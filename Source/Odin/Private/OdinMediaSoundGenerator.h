#pragma once

#include "DSP/Dsp.h"
#include "Sound/SoundGenerator.h"

class OdinMediaSoundGenerator : public ISoundGenerator
{
  public:
    OdinMediaSoundGenerator();

    void SetOdinStream(OdinMediaStream *stream);

    // Called when a new buffer is required.
    int32 OnGenerateAudio(float *OutAudio, int32 NumSamples) override final;

    // Returns the number of samples to render per callback
    int32 GetDesiredNumSamplesToRenderPerCallback() const override final
    {
        return 960;
    }

    // Optional. Called on audio generator thread right when the generator begins generating.
    void OnBeginGenerate() override final;

    // Optional. Called on audio generator thread right when the generator ends generating.
    void OnEndGenerate() override final;

  private:
    struct OdinMediaStream *stream_ = nullptr;
};