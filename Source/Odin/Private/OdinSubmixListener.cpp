/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinSubmixListener.h"
#include "Odin.h"
#include "Sound/SoundSubmix.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "AudioDeviceHandle.h"
#else
#include "AudioDeviceManager.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "ISubmixBufferListener.h"
#endif

#include "AudioDevice.h"
#include "OdinInitializationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

using namespace Audio;

UOdinSubmixListener::UOdinSubmixListener(const class FObjectInitializer& PCIP)
    : Super(PCIP)
    , CurrentRoomHandle(0)
{
}

void UOdinSubmixListener::StartSubmixListener()
{
    UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: StartSubmixListener Called."))

    if (!GetWorld()) {
        UE_LOG(Odin, Error, TEXT("OdinSubmixListener: StartSubmixListener failed, invalid World."));
        return;
    }

    if (IsListening()) {
        UE_LOG(Odin, Verbose,
               TEXT("OdinSubmixListener, StartSubmixListener: Already listening, restarting submix "
                    "listener"));
        StopSubmixListener();
    }

    const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    if (!GameInstance) {
        UE_LOG(Odin, Error,
               TEXT("OdinSubmixListener: StartSubmixListener failed, GameInstance is invalid."));
        return;
    }

    const UOdinInitializationSubsystem* OdinInitializationSubsystem =
        GameInstance->GetSubsystem<UOdinInitializationSubsystem>();
    if (!OdinInitializationSubsystem) {
        UE_LOG(Odin, Error,
               TEXT("OdinSubmixListener: StartSubmixListener failed, UOdinInitializationSubsystem "
                    "is invalid."));
        return;
    }

    const int32 OdinSampleRate = OdinInitializationSubsystem->GetSampleRate();
    const int32 OdinChannels   = OdinInitializationSubsystem->GetChannelCount();

    UE_LOG(Odin, Verbose,
           TEXT("OdinSubmixListener: Starting Submix Listener with OdinSampleRate %d and "
                "OdinChannels %d"),
           OdinSampleRate, OdinChannels);

    FAudioDeviceHandle AudioDeviceHandle = GetWorld()->GetAudioDevice();
    UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: Retrieved Audio Device with Handle Id: %d"),
           AudioDeviceHandle.GetDeviceID());
    if (!AudioDeviceHandle.IsValid()) {
        UE_LOG(Odin, Error,
               TEXT("OdinSubmixListener: StartSubmixListener failed, no Active AudioDevice "
                    "available."));
        return;
    }
    const int32 SampleRate = AudioDeviceHandle->SampleRate;
    if (SampleRate != OdinSampleRate) {
        UE_LOG(Odin, Warning,
               TEXT("OdinSubmixListener: Detected difference in sample rate: %d In Sample Rate and "
                    "%d Odin Sample "
                    "Rate. Echo Cancellation will not work correctly!"),
               SampleRate, OdinSampleRate);
    }

    const FOnSubmixBufferListenerError ErrorDelegate =
        FOnSubmixBufferListenerError::CreateUObject(this, &UOdinSubmixListener::StopSubmixListener);

    if (!SubmixBufferListener.IsValid())
        SubmixBufferListener = MakeShared<FOdinSubmixBufferListenerImplementation>();

    SubmixBufferListener->Initialize(CurrentRoomHandle, OdinSampleRate, OdinChannels,
                                     ErrorDelegate);
    UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: Registering SubmixBufferListener Called."))
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
    ConnectedSubmix = &AudioDeviceHandle->GetMainSubmixObject();
    AudioDeviceHandle->RegisterSubmixBufferListener(SubmixBufferListener.ToSharedRef(),
                                                    *ConnectedSubmix.Get());
#else
    AudioDeviceHandle->RegisterSubmixBufferListener(SubmixBufferListener.Get());
#endif
}

void UOdinSubmixListener::StopSubmixListener()
{
    if (!GetWorld() || !IsListening() || !SubmixBufferListener.IsValid())
        return;

    FAudioDeviceHandle AudioDeviceHandle = GetWorld()->GetAudioDevice();
    if (!AudioDeviceHandle.IsValid()) {
        return;
    }

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
    if (!ConnectedSubmix.IsValid()) {
        UE_LOG(
            Odin, Error,
            TEXT("UOdinSubmixListener: StopSubmixListener failed, Connected Submix is invalid."));
        return;
    }
    AudioDeviceHandle->UnregisterSubmixBufferListener(SubmixBufferListener.ToSharedRef(),
                                                      *ConnectedSubmix.Get());
#else
    AudioDeviceHandle->UnregisterSubmixBufferListener(SubmixBufferListener.Get());
#endif
    SubmixBufferListener->StopListener();
    UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: UnregisterSubmixBufferListener Called."))
}

void UOdinSubmixListener::SetRoom(OdinRoomHandle room)
{
    CurrentRoomHandle = room;
}

void UOdinSubmixListener::BeginDestroy()
{
    Super::BeginDestroy();
    if (IsListening())
        StopSubmixListener();
}

bool UOdinSubmixListener::IsListening() const
{
    return SubmixBufferListener.IsValid() && SubmixBufferListener->IsInitialized();
}

void UOdinSubmixListener::SetRecordSubmixOutput(bool bNewActive)
{
    if (SubmixBufferListener.IsValid()) {
        SubmixBufferListener->SetRecordSubmixOutput(bNewActive);
    }
}

void UOdinSubmixListener::StopRecording()
{
    if (SubmixBufferListener.IsValid()) {
        SubmixBufferListener->StopSubmixRecording();
    }
}

FOdinSubmixBufferListenerImplementation::FOdinSubmixBufferListenerImplementation()
    : CurrentRoomHandle(0)
    , bRecordSubmixOutput(false)
{
}

FOdinSubmixBufferListenerImplementation::~FOdinSubmixBufferListenerImplementation() {}

void FOdinSubmixBufferListenerImplementation::Initialize(
    OdinRoomHandle Handle, int32 SampleRate, int32 Channels,
    const FOnSubmixBufferListenerError& Callback, bool ShouldRecordSubmixOutput)
{
    CurrentRoomHandle   = Handle;
    OdinSampleRate      = SampleRate;
    OdinChannels        = Channels;
    ErrorCallback       = Callback;
    bInitialized        = true;
    bRecordSubmixOutput = ShouldRecordSubmixOutput;

    SavedBuffer.Reset();
}

void FOdinSubmixBufferListenerImplementation::StopListener()
{
    bInitialized = false;
    StopSubmixRecording();
}

bool FOdinSubmixBufferListenerImplementation::IsInitialized() const
{
    return bInitialized;
}

void FOdinSubmixBufferListenerImplementation::SetRecordSubmixOutput(bool bShouldRecord)
{
    bRecordSubmixOutput = bShouldRecord;
}

void FOdinSubmixBufferListenerImplementation::StopSubmixRecording()
{
    if (bRecordSubmixOutput) {
        const FString       FileName = "SubmixOutput";
        FString             FilePath = "";
        FSoundWavePCMWriter Writer;
        Writer.BeginWriteToWavFile(SavedBuffer, FileName, FilePath, [FileName]() {
            UE_LOG(Odin, Log,
                   TEXT("FOdinSubmixBufferListenerImplementation: Successfully wrote SubMix Output "
                        "to /Saved/BouncedWavFiles/%s.wav"),
                   *FileName);
        });
        bRecordSubmixOutput = false;
    }
}

void FOdinSubmixBufferListenerImplementation::OnNewSubmixBuffer(
    const USoundSubmix* OwningSubmix, float* AudioData, int32 InNumSamples, int32 InNumChannels,
    const int32 InSampleRate, double InAudioClock)
{
    if (!IsInitialized())
        return;

    UE_LOG(Odin, VeryVerbose,
           TEXT("OdinSubmixListener: In Channels: %d In SampleRate: %d In Num Samples: %d In Audio "
                "Clock: %f"),
           InNumChannels, InSampleRate, InNumSamples, InAudioClock);

    TSampleBuffer<float> buffer(AudioData, InNumSamples, InNumChannels, InSampleRate);
    if (buffer.GetNumChannels() != OdinChannels) {
        UE_LOG(Odin, VeryVerbose,
               TEXT("OdinSubmixListener: Due to differences in Channel Count, remixing buffer from "
                    "%d Channels to %d "
                    "OdinChannels"),
               InNumChannels, OdinChannels);
        buffer.MixBufferToChannels(OdinChannels);
    }

    if (InSampleRate != OdinSampleRate) {
        UE_LOG(Odin, Verbose,
               TEXT("OdinSubmixListener: InSampleRate %d !== OdinSampleRate %d - Echo Cancellation "
                    "will not work "
                    "correctly!"),
               InSampleRate, OdinSampleRate);
    }

    float*         pbuffer = buffer.GetArrayView().GetData();
    OdinReturnCode result =
        odin_audio_process_reverse(CurrentRoomHandle, pbuffer, buffer.GetNumSamples());

    if (bRecordSubmixOutput && InNumSamples > 0) {
        if (SavedBuffer.GetNumChannels() != buffer.GetNumChannels()
            || SavedBuffer.GetSampleRate() != buffer.GetSampleRate()) {
            SavedBuffer.Append(buffer.GetData(), buffer.GetNumSamples(), buffer.GetNumChannels(),
                               buffer.GetSampleRate());
        } else {
            SavedBuffer.Append(buffer.GetData(), buffer.GetNumSamples());
        }
    }

    if (odin_is_error(result)) {
        UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: din_audio_process_reverse result: %d"),
               result);
        UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: OnNewSubmixBuffer on %s "),
               *OwningSubmix->GetFName().ToString());

        ErrorCallback.ExecuteIfBound();
    }
}
