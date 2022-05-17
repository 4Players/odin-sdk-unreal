#pragma once

#include "OdinLibrary/include/odin.h"

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include <memory>

#include "OdinPlaybackMedia.generated.h"

class UOdinRoom;

UCLASS(BlueprintType, ClassGroup = Odin)
class UOdinPlaybackMedia : public UObject
{
    GENERATED_BODY()

    friend class UOdinRoom;

  public:
    UOdinPlaybackMedia();
    UOdinPlaybackMedia(OdinMediaStreamHandle streamHandle, UOdinRoom *room);

    OdinMediaStreamHandle GetMediaHandle() const
    {
        return this->stream_handle_;
    }

    void SetMedia(OdinMediaStreamHandle streamHandle)
    {
        this->stream_handle_ = streamHandle;
    }

    void SetRoom(UOdinRoom *room)
    {
        this->Room = room;
    }

    UFUNCTION(BlueprintCallable)
    int32 GetMediaId();

  protected:
    void BeginDestroy() override;

    OdinMediaStreamHandle stream_handle_ = 0;

    UPROPERTY(BlueprintReadOnly)
    UOdinRoom *Room;
};