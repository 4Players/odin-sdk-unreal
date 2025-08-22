/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "odin_sdk.h"

#include <memory>

#include "OdinMediaBase.generated.h"

class IAudioBufferListener;
class UOdinRoom;

UCLASS(ClassGroup = Odin, NotBlueprintable, BlueprintType, Abstract)
class ODIN_API UOdinMediaBase : public UObject
{
    GENERATED_UCLASS_BODY()

  public:
    /**
     * @brief Returns the Odin media stream handle associated with this object.
     *
     * @return The Odin media stream handle.
     */
    OdinMediaStreamHandle GetMediaHandle() const
    {
        return this->stream_handle_;
    }

    virtual void AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener);
    virtual void RemoveAudioBufferListener(IAudioBufferListener* AudioBufferListener);

    virtual int32 GetSampleRate() const;
    virtual int32 GetNumChannels() const;

  protected:
    virtual void SetMediaHandle(OdinMediaStreamHandle handle);

    TArray<IAudioBufferListener*> GetAudioBufferListeners() const;

  private:
    TSet<IAudioBufferListener*> AudioBufferListeners;
    mutable FCriticalSection    AudioBufferListenerSection;

    OdinMediaStreamHandle stream_handle_ = 0;

    friend class UOdinPlaybackMedia;
    friend class UOdinCaptureMedia;
};
