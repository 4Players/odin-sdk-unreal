/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#include "OdinMediaSoundGenerator.h"

#include "OdinCore/include/odin.h"

OdinMediaSoundGenerator::OdinMediaSoundGenerator() = default;

int32 OdinMediaSoundGenerator::OnGenerateAudio(float *OutAudio, int32 NumSamples)
{
    if (stream_handle_ == 0) {
        return NumSamples;
    }

    auto read = odin_audio_read_data(stream_handle_, OutAudio, NumSamples, OdinChannelLayout_Mono);
    if (odin_is_error(read)) {
        return NumSamples;
    } else {
        return read;
    }
}

void OdinMediaSoundGenerator::SetOdinStream(OdinMediaStreamHandle streamHandle)
{
    stream_handle_ = streamHandle;
}

void OdinMediaSoundGenerator::OnBeginGenerate() {}

void OdinMediaSoundGenerator::OnEndGenerate() {}