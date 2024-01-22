/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinMediaSoundGenerator.h"
#include "Components/SynthComponent.h"
#include "odin_sdk.h"

OdinMediaSoundGenerator::OdinMediaSoundGenerator() = default;

int32 OdinMediaSoundGenerator::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    if (stream_handle_ == 0) {
        return NumSamples;
    }

    auto read = odin_audio_read_data(stream_handle_, OutAudio, NumSamples);
    if (odin_is_error(read)) {
        return NumSamples;
    }
    for (IAudioBufferListener* AudioBufferListener : AudioBufferListeners) {
        AudioBufferListener->OnGeneratedBuffer(OutAudio, NumSamples, 2);
    }
    return read;
}

void OdinMediaSoundGenerator::SetOdinStream(OdinMediaStreamHandle streamHandle)
{
    if (streamHandle != 0) {
        odin_audio_reset(streamHandle);
    }

    stream_handle_ = streamHandle;
}

void OdinMediaSoundGenerator::OnBeginGenerate() {}

void OdinMediaSoundGenerator::OnEndGenerate() {}

void OdinMediaSoundGenerator::AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    AudioBufferListeners.AddUnique(InAudioBufferListener);
}

void OdinMediaSoundGenerator::RemoveAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    AudioBufferListeners.Remove(InAudioBufferListener);
}
