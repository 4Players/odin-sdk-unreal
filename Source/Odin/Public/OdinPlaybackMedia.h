/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

#include "CoreMinimal.h"
#include "OdinMediaBase.h"
#include "UObject/Object.h"

#include <memory>

#include "OdinPlaybackMedia.generated.h"

class UOdinRoom;

UCLASS(BlueprintType, ClassGroup = Odin)
class ODIN_API UOdinPlaybackMedia : public UOdinMediaBase
{
    GENERATED_BODY()

    friend class UOdinRoom;

  public:
    UOdinPlaybackMedia();
    UOdinPlaybackMedia(OdinMediaStreamHandle streamHandle, UOdinRoom *room);

    void SetRoom(UOdinRoom *room)
    {
        this->Room = room;
    }

    UFUNCTION(BlueprintCallable,
              meta = (DeprecatedFunction,
                      DeprecationMessage =
                          "Media IDs are deprecated! Use media objects/handles instead."))
    int32 GetMediaId();

  protected:
    void BeginDestroy() override;

    UPROPERTY(BlueprintReadOnly, Category = "Room")
    UOdinRoom *Room;
};