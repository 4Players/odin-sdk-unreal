/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinMediaSoundGenerator.h"

#include "Odin.h"
#include "OdinFunctionLibrary.h"
#include "Components/SynthComponent.h"
#include "odin_sdk.h"

OdinMediaSoundGenerator::OdinMediaSoundGenerator() = default;

int32 OdinMediaSoundGenerator::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    if (stream_handle_ == 0) {
        return 0;
    }

    // Will return the number of read samples, if successful, error code otherwise
    OdinReturnCode ReadResult = odin_audio_read_data(stream_handle_, OutAudio, NumSamples);
    if (odin_is_error(ReadResult)) {
        const FString FormattedError = UOdinFunctionLibrary::FormatError(ReadResult, false);
        UE_LOG(Odin, Verbose,
               TEXT("Error while reading data from Odin in UOdinSynthComponent::OnGenerateAudio, "
                    "Error Message: %s, could be due to media being removed."),
               *FormattedError);
        return 0;
    }

    if (ReadResult > static_cast<uint32>(NumSamples)) {
        UE_LOG(Odin, Verbose,
               TEXT("Error while reading data from Odin in UOdinSynthComponent::OnGenerateAudio, "
                    "number of read samples returned by Odin is larger than requested number of "
                    "samples."));
        return 0;
    }

    for (IAudioBufferListener* AudioBufferListener : AudioBufferListeners) {
        AudioBufferListener->OnGeneratedBuffer(OutAudio, NumSamples, 2);
    }
    return ReadResult;
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
