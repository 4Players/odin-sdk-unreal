/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinMediaSoundGenerator.h"

#include "Odin.h"
#include "OdinFunctionLibrary.h"
#include "Components/SynthComponent.h"
#include "odin_sdk.h"

int32 OdinMediaSoundGenerator::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(OdinMediaSoundGenerator::OnGenerateAudio)
    if (!PlaybackStreamReader.IsValid()) {
        return NumSamples;
    }

    const OdinReturnCode& ReturnCode =
        PlaybackStreamReader->ReadData(PlaybackStreamReadIndex, OutAudio, NumSamples);
    if (odin_is_error(ReturnCode)) {
        const FString FormattedError = UOdinFunctionLibrary::FormatError(ReturnCode, false);
        if (FormattedError == "media is invalid") {
            UE_LOG(
                Odin, Verbose,
                TEXT(
                    "Notification while reading data from Odin in "
                    "OdinMediaSoundGenerator::OnGenerateAudio, "
                    "Message: %s, could be due to media being removed. This message is expected to "
                    "appear a few times during normal usage."),
                *FormattedError);
            PlaybackStreamReader.Reset();
            bWasMediaStreamInvalid = true;
        } else {
            UE_LOG(Odin, Error,
                   TEXT("Error while reading data from Odin in "
                        "OdinMediaSoundGenerator::OnGenerateAudio, Message: %s"),
                   *FormattedError);
        }

        return NumSamples;
    }

    int32 ReadSamples = static_cast<int32>(ReturnCode);
    if (ReadSamples > NumSamples || ReadSamples < 0) {
        UE_LOG(Odin, Verbose,
               TEXT("Notification while reading data from Odin in "
                    "UOdinSynthComponent::OnGenerateAudio, "
                    "number of read samples returned by Odin is larger than requested number of "
                    "samples."));
        return NumSamples;
    }
    return ReturnCode;
}

void OdinMediaSoundGenerator::SetOdinStream(OdinMediaStreamHandle NewStreamHandle)
{
    if (NewStreamHandle != 0) {
        odin_audio_reset(NewStreamHandle);
    }

    UE_LOG(Odin, Warning,
           TEXT("OdinMediaSoundGenerator::SetOdinStream was called. This is being deprecated, "
                "please provide an FOdinPlaybackStreamReader pointer instead. Creating new "
                "FOdinPlaybackStreamReader."));
    bWasMediaStreamInvalid = false;

    // use default values, 20ms audio frames and 1 second of audio buffer capacity.
    constexpr int32 Capacity = ODIN_DEFAULT_CHANNEL_COUNT * (ODIN_DEFAULT_SAMPLE_RATE * 0.02f);
    SetStreamReader(
        MakeShared<FOdinPlaybackStreamReader, ESPMode::ThreadSafe>(NewStreamHandle, Capacity));
}

void OdinMediaSoundGenerator::SetStreamReader(
    const TSharedPtr<FOdinPlaybackStreamReader, ESPMode::ThreadSafe>& StreamReader)
{
    PlaybackStreamReader = StreamReader;
    if (StreamReader.IsValid()) {
        bWasMediaStreamInvalid  = false;
        PlaybackStreamReadIndex = StreamReader->GetLatestReadIndex();
    }
}

#if ENGINE_MAJOR_VERSION >= 5
bool OdinMediaSoundGenerator::IsFinished() const
{
    return bWasMediaStreamInvalid;
}
#endif

void OdinMediaSoundGenerator::AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    if (PlaybackStreamReader.IsValid()) {
        PlaybackStreamReader->AddAudioBufferListener(InAudioBufferListener);
    }
}

void OdinMediaSoundGenerator::RemoveAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    if (PlaybackStreamReader.IsValid()) {
        PlaybackStreamReader->RemoveAudioBufferListener(InAudioBufferListener);
    }
}