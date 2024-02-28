/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinSubmixListener.h"
#include "Odin.h"
#include "Sound/SoundSubmix.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "ISubmixBufferListener.h"
#endif
#include "AudioDevice.h"

using namespace Audio;

UOdinSubmixListener::UOdinSubmixListener(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
    resampler_handle = 0;
}

UOdinSubmixListener::~UOdinSubmixListener()
{
    if (bInitialized)
        StopSubmixListener();

    if (resampler_handle != 0)
        odin_resampler_destroy(resampler_handle);
}

void UOdinSubmixListener::StartSubmixListener()
{
    if (bInitialized || !GEngine)
        return;

    const FOdinModule OdinModule = FModuleManager::GetModuleChecked<FOdinModule>("Odin");
    OdinSampleRate               = OdinModule.GetSampleRate();
    OdinChannels                 = OdinModule.GetChannelCount();

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

    AudioDevice->RegisterSubmixBufferListener(this);
    bInitialized = true;
}

void UOdinSubmixListener::StopSubmixListener()
{
    if (!bInitialized || !GEngine)
        return;

    FAudioDeviceHandle AudioDevice = GEngine->GetActiveAudioDevice();
    AudioDevice->UnregisterSubmixBufferListener(this);
    bInitialized = false;
}

void UOdinSubmixListener::SetRoom(OdinRoomHandle room)
{
    current_room_handle = room;
}

void UOdinSubmixListener::OnNewSubmixBuffer(const USoundSubmix *OwningSubmix, float *AudioData,
                                            int32 InNumSamples, int32 InNumChannels,
                                            const int32 InSampleRate, double InAudioClock)
{
    if (!bInitialized)
        return;

    FScopeLock Lock(&submix_cs_);

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

    float         *pbuffer = buffer.GetArrayView().GetData();
    OdinReturnCode result =
        odin_audio_process_reverse(current_room_handle, pbuffer, buffer.GetNumSamples());

    if (odin_is_error(result)) {
        UE_LOG(Odin, Verbose, TEXT("odin_audio_process_reverse result: %d"), result);
        UE_LOG(Odin, Verbose, TEXT("OnNewSubmixBuffer on %s "),
               *OwningSubmix->GetFName().ToString());

        StopSubmixListener();
    }
}
