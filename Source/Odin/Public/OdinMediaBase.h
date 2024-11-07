/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "odin_sdk.h"

#include <memory>

#include "OdinMediaBase.generated.h"

class UOdinRoom;

UCLASS(ClassGroup = Odin)
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

  protected:
    inline void SetMediaHandle(OdinMediaStreamHandle handle)
    {
        this->stream_handle_ = handle;
    }

  private:
    OdinMediaStreamHandle stream_handle_ = 0;

    friend class UOdinPlaybackMedia;
    friend class UOdinCaptureMedia;
};
