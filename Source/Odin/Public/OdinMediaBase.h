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

    void AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener);
    void RemoveAudioBufferListener(IAudioBufferListener* AudioBufferListener);

    virtual int32 GetSampleRate() const;
    virtual int32 GetNumChannels() const;

  protected:
    inline void SetMediaHandle(OdinMediaStreamHandle handle)
    {
        this->stream_handle_ = handle;
    }

    TSet<IAudioBufferListener*> AudioBufferListeners;

  private:
    OdinMediaStreamHandle stream_handle_ = 0;

    friend class UOdinPlaybackMedia;
    friend class UOdinCaptureMedia;
};
