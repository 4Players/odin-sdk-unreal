/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

#include "DSP/Dsp.h"
#include "Sound/SoundGenerator.h"

class OdinMediaSoundGenerator : public ISoundGenerator
{
  public:
    OdinMediaSoundGenerator();

    void SetOdinStream(OdinMediaStreamHandle streamHandle);

    // Called when a new buffer is required.
    int32 OnGenerateAudio(float *OutAudio, int32 NumSamples) override final;

    // Returns the number of samples to render per callback.
    int32 GetDesiredNumSamplesToRenderPerCallback() const override final
    {
        return 1920; // 20ms 48kHz stereo
    }

    // Optional. Called on audio generator thread right when the generator begins generating.
    void OnBeginGenerate() override final;

    // Optional. Called on audio generator thread right when the generator ends generating.
    void OnEndGenerate() override final;

  private:
    OdinMediaStreamHandle stream_handle_ = 0;
};
