/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinSubmixListener.h"

#include "Odin.h"

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

    FAudioDeviceHandle AudioDevice = GEngine->GetActiveAudioDevice();

    int32 samplerate = AudioDevice->SampleRate;
    if (samplerate != OdinSampleRate) {
        UE_LOG(Odin, Warning, TEXT("Creating resampler. Samplerate of %d mismatch %d"),
               AudioDevice->SampleRate, OdinSampleRate);
        // resampler_handle = odin_resampler_create(samplerate, OdinSampleRate, OdinChannels);
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

    Audio::TSampleBuffer<float> buffer(AudioData, InNumSamples, InNumChannels, InSampleRate);
    if (buffer.GetNumChannels() != OdinChannels)
        buffer.MixBufferToChannels(OdinChannels);

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
