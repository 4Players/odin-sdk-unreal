/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "odin_sdk.h"

#include "DSP/Dsp.h"
#include "Sound/SoundGenerator.h"

class IAudioBufferListener;

class ODIN_API OdinMediaSoundGenerator : public ISoundGenerator
{
  public:
    OdinMediaSoundGenerator();

    void SetOdinStream(OdinMediaStreamHandle streamHandle);

    // Called when a new buffer is required.
    int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override final;

    // Returns the number of samples to render per callback.
    int32 GetDesiredNumSamplesToRenderPerCallback() const override final
    {
        return 1920; // 20ms 48kHz stereo
    }

    // Optional. Called on audio generator thread right when the generator begins generating.
    void OnBeginGenerate() override final;

    // Optional. Called on audio generator thread right when the generator ends generating.
    void OnEndGenerate() override final;

    void AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener);
    void RemoveAudioBufferListener(IAudioBufferListener* InAudioBufferListener);

  private:
    OdinMediaStreamHandle stream_handle_ = 0;

    TArray<IAudioBufferListener*> AudioBufferListeners;
};
