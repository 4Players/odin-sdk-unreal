/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinMediaBase.h"

#include "Odin.h"
#include "Components/SynthComponent.h"

UOdinMediaBase::UOdinMediaBase(const class FObjectInitializer& PCIP)
    : Super(PCIP)
{
}

void UOdinMediaBase::AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    if (InAudioBufferListener) {
        FScopeLock BufferListenerAccessLock(&AudioBufferListenerSection);
        AudioBufferListeners.Add(InAudioBufferListener);
    }
}

void UOdinMediaBase::RemoveAudioBufferListener(IAudioBufferListener* AudioBufferListener)
{
    if (AudioBufferListeners.Contains(AudioBufferListener)) {
        FScopeLock BufferListenerAccessLock(&AudioBufferListenerSection);

        AudioBufferListeners.Remove(AudioBufferListener);
    }
}

int32 UOdinMediaBase::GetSampleRate() const
{
    return ODIN_DEFAULT_SAMPLE_RATE;
}

int32 UOdinMediaBase::GetNumChannels() const
{
    return ODIN_DEFAULT_SAMPLE_RATE;
}

void UOdinMediaBase::SetMediaHandle(OdinMediaStreamHandle handle)
{
    this->stream_handle_ = handle;
}

TArray<IAudioBufferListener*> UOdinMediaBase::GetAudioBufferListeners() const
{
    FScopeLock BufferListenerAccessLock(&AudioBufferListenerSection);
    return AudioBufferListeners.Array();
}