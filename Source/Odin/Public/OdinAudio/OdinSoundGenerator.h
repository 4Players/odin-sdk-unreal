/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "DSP/Dsp.h"
#include "OdinNative/OdinNativeHandle.h"
#include "HAL/ThreadSafeBool.h"
#include "Sound/SoundGenerator.h"

class IAudioBufferListener;
class UOdinDecoder;

class ODIN_API FOdinSoundGenerator : public ISoundGenerator
{
  public:
    FOdinSoundGenerator();
    ~FOdinSoundGenerator();

    void SetOdinDecoder(UOdinDecoder* InDecoder);
    void Close();
    void AddAudioBufferListener(const TWeakPtr<IAudioBufferListener>& InAudioBufferListener);
    void RemoveAudioBufferListener(const TWeakPtr<IAudioBufferListener>& InAudioBufferListener);

    virtual int32 GetDesiredNumSamplesToRenderPerCallback() const override;

    // Called on audio generator thread right when the generator begins generating.
    virtual void OnBeginGenerate() override;

    // Called when a new buffer is required.
    virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

    // Called on audio generator thread right when the generator ends generating.
    virtual void OnEndGenerate() override;

    virtual bool IsFinished() const override;

  private:
    TWeakObjectPtr<UOdinHandle>            OdinDecoderHandle;
    TArray<TWeakPtr<IAudioBufferListener>> AudioBufferListeners;
    FCriticalSection                       CriticalSectionAudioBufferListeners;

    OdinDecoder*     NativeDecoderHandle;
    FCriticalSection NativeHandleAccessSection;

    FThreadSafeBool bIsFinished;

    int32 SampleRate   = 48000;
    int32 ChannelCount = 1;
};