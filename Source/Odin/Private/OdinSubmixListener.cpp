/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinSubmixListener.h"
#include "Odin.h"
#include "Sound/SoundSubmix.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "ISubmixBufferListener.h"
#endif
#include "AudioDevice.h"
#include "OdinInitializationSubsystem.h"
#include "Engine/GameInstance.h"

using namespace Audio;

UOdinSubmixListener::UOdinSubmixListener(const class FObjectInitializer& PCIP)
    : Super(PCIP)
    , CurrentRoomHandle(0)
{
    resampler_handle = 0;
}

void UOdinSubmixListener::StartSubmixListener()
{
    if (IsListening()) {
        UE_LOG(Odin, Error, TEXT("StartSubmixListener failed, already listening."))
        return;
    }
    if (!GetWorld() || !GEngine) {
        UE_LOG(Odin, Error, TEXT("StartSubmixListener failed, invalid World or Engine."))
        return;
    }

    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    if (!GameInstance) {
        UE_LOG(Odin, Error, TEXT("StartSubmixListener failed, GameInstance is invalid."));
        return;
    }

    UOdinInitializationSubsystem* OdinInitializationSubsystem =
        GameInstance->GetSubsystem<UOdinInitializationSubsystem>();
    if (!OdinInitializationSubsystem) {
        UE_LOG(Odin, Error,
               TEXT("StartSubmixListener failed, UOdinInitializationSubsystem is invalid."));
        return;
    }

    int32 OdinSampleRate = OdinInitializationSubsystem->GetSampleRate();
    int32 OdinChannels   = OdinInitializationSubsystem->GetChannelCount();

    UE_LOG(Odin, Log, TEXT("Starting Submix Listener with OdinSampleRate %d and OdinChannels %d"),
           OdinSampleRate, OdinChannels);

    FAudioDeviceHandle AudioDevice = GEngine->GetActiveAudioDevice();
    const int32        SampleRate  = AudioDevice->SampleRate;
    if (SampleRate != OdinSampleRate) {
        UE_LOG(Odin, Warning,
               TEXT("Detected difference in sample rate: %d In Sample Rate and %d Odin Sample "
                    "Rate. Echo Cancellation will not work correctly!"),
               SampleRate, OdinSampleRate);
    }

    FOnSubmixBufferListenerError ErrorDelegate =
        FOnSubmixBufferListenerError::CreateUObject(this, &UOdinSubmixListener::StopSubmixListener);

    SubmixBufferListener = MakeShareable(new FOdinSubmixBufferListenerImplementation());
    SubmixBufferListener->Initialize(CurrentRoomHandle, OdinSampleRate, OdinChannels,
                                     ErrorDelegate);
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
    AudioDevice->RegisterSubmixBufferListener(SubmixBufferListener.ToSharedRef(),
                                              AudioDevice->GetMainSubmixObject());
#else
    AudioDevice->RegisterSubmixBufferListener(SubmixBufferListener.Get());
#endif
}

void UOdinSubmixListener::StopSubmixListener()
{
    if (!IsListening() || !GEngine)
        return;

    FAudioDeviceHandle AudioDevice = GEngine->GetActiveAudioDevice();
    if (!AudioDevice.IsValid())
        return;

    if (!SubmixBufferListener.IsValid())
        return;

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
    AudioDevice->UnregisterSubmixBufferListener(SubmixBufferListener.ToSharedRef(),
                                                AudioDevice->GetMainSubmixObject());
#else
    AudioDevice->UnregisterSubmixBufferListener(SubmixBufferListener.Get());
#endif
    SubmixBufferListener->Stop();
}

void UOdinSubmixListener::SetRoom(OdinRoomHandle room)
{
    CurrentRoomHandle = room;
}

void UOdinSubmixListener::BeginDestroy()
{
    UObject::BeginDestroy();
    if (IsListening())
        StopSubmixListener();

    if (resampler_handle != 0)
        odin_resampler_destroy(resampler_handle);
}

bool UOdinSubmixListener::IsListening() const
{
    return SubmixBufferListener.IsValid() && SubmixBufferListener->IsInitialized();
}

FOdinSubmixBufferListenerImplementation::FOdinSubmixBufferListenerImplementation()
    : CurrentRoomHandle(0)
{
}

FOdinSubmixBufferListenerImplementation::~FOdinSubmixBufferListenerImplementation() {}

void FOdinSubmixBufferListenerImplementation::Initialize(
    OdinRoomHandle Handle, int32 SampleRate, int32 Channels,
    const FOnSubmixBufferListenerError& Callback)
{
    CurrentRoomHandle = Handle;
    OdinSampleRate    = SampleRate;
    OdinChannels      = Channels;
    ErrorCallback     = Callback;
    bInitialized      = true;
}

void FOdinSubmixBufferListenerImplementation::Stop()
{
    bInitialized = false;
}

bool FOdinSubmixBufferListenerImplementation::IsInitialized() const
{
    return bInitialized;
}

void FOdinSubmixBufferListenerImplementation::OnNewSubmixBuffer(
    const USoundSubmix* OwningSubmix, float* AudioData, int32 InNumSamples, int32 InNumChannels,
    const int32 InSampleRate, double InAudioClock)
{
    if (!bInitialized)
        return;

    FScopeLock Lock(&SubmixCS);

    UE_LOG(Odin, VeryVerbose,
           TEXT("In Channels: %d In SampleRate: %d In Num Samples: %d In Audio Clock: %f"),
           InNumChannels, InSampleRate, InNumSamples, InAudioClock);

    TSampleBuffer<float> buffer(AudioData, InNumSamples, InNumChannels, InSampleRate);
    if (buffer.GetNumChannels() != OdinChannels) {
        UE_LOG(Odin, VeryVerbose,
               TEXT("Due to differences in Channel Count, remixing buffer from %d Channels to %d "
                    "OdinChannels"),
               InNumChannels, OdinChannels);
        buffer.MixBufferToChannels(OdinChannels);
    }

    if (InSampleRate != OdinSampleRate) {
        UE_LOG(Odin, Verbose,
               TEXT("InSampleRate %d !== OdinSampleRate %d - Echo Cancellation will not work "
                    "correctly!"),
               InSampleRate, OdinSampleRate);
    }

    float*         pbuffer = buffer.GetArrayView().GetData();
    OdinReturnCode result =
        odin_audio_process_reverse(CurrentRoomHandle, pbuffer, buffer.GetNumSamples());

    if (odin_is_error(result)) {
        UE_LOG(Odin, Verbose, TEXT("odin_audio_process_reverse result: %d"), result);
        UE_LOG(Odin, Verbose, TEXT("OnNewSubmixBuffer on %s "),
               *OwningSubmix->GetFName().ToString());

        ErrorCallback.ExecuteIfBound();
    }
}