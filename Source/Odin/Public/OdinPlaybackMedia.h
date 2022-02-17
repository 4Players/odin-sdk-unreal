#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include <memory>

#include "OdinPlaybackMedia.generated.h"

class UOdinRoom;

UCLASS(BlueprintType, Blueprintable)
class UOdinPlaybackMedia : public UObject
{
    GENERATED_BODY()

  public:
    UOdinPlaybackMedia();
    UOdinPlaybackMedia(struct OdinMediaStream *streamm, UOdinRoom *Room);

    struct OdinMediaStream *GetMedia()
    {
        return this->stream_;
    }

    void SetMedia(struct OdinMediaStream *stream)
    {
        this->stream_ = stream;
    }

    void SetSharedRoom(std::shared_ptr<struct OdinRoom> sharedRoom) {
        this->sharedRoom_ = sharedRoom;
    }

    void SetRoom(UOdinRoom *room)
    {
        this->Room = room;
    }

  protected:
    void BeginDestroy() override;

    struct OdinMediaStream *stream_ = nullptr;

    UPROPERTY(BlueprintReadOnly)
    UOdinRoom *Room;

    std::shared_ptr<struct OdinRoom> sharedRoom_;
};