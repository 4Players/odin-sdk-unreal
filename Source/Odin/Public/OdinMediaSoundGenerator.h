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

    /**
     * Set the OdinMediaStream to be used by the OdinMediaSoundGenerator for reading voice data.
     *
     * @param streamHandle - The handle to the OdinMediaStream to set. Must not be 0.
     */
    void SetOdinStream(OdinMediaStreamHandle streamHandle);

    // Called by the sound system when a new buffer is required.
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

    /**
     * Add a listener to the Audio generated by the OdinMediaSoundGenerator.
     *
     * @param InAudioBufferListener - Pointer to an object implementing the IAudioBufferListener
     * interface.
     */
    void AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener);
    /**
     * @param InAudioBufferListener The audio buffer listener to remove.
     */
    void RemoveAudioBufferListener(IAudioBufferListener* InAudioBufferListener);

  private:
    OdinMediaStreamHandle stream_handle_ = 0;

    TArray<IAudioBufferListener*> AudioBufferListeners;
};
