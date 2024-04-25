/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "OdinMediaBase.h"
#include "UObject/Object.h"
#include "odin_sdk.h"

#include <memory>

#include "OdinPlaybackMedia.generated.h"

class UOdinRoom;

USTRUCT(BlueprintType)
struct ODIN_API FOdinAudioStreamStats {
    GENERATED_BODY()

    /**
     * The total number of packets seen by the medias jitter buffer.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Packets total"))
    int32 packets_total = 0;
    /**
     * The number of packets processed by the medias jitter buffer.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Packets processed"))
    int32 packets_processed = 0;
    /**
     * The number of packets dropped because they seemed to arrive too early.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats",
              meta = (DisplayName = "Packets arrived too early"))
    int32 packets_arrived_too_early = 0;
    /**
     * The number of packets dropped because they seemed to arrive too late.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats",
              meta = (DisplayName = "Packets arrived too late"))
    int32 packets_arrived_too_late = 0;
    /**
     * The number of packets dropped due to a jitter buffer reset.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Packets dropped"))
    int32 packets_dropped = 0;
    /**
     * The number of packets marked as invalid.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Packets invalid"))
    int32 packets_invalid = 0;
    /**
     * The number of packets marked as duplicates.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Packets repeated"))
    int32 packets_repeated = 0;
    /**
     * The number of packets marked as lost during transmission.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Packets lost"))
    int32 packets_lost = 0;
};

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
              meta = (DisplayName = "Get Output Media ID",
                      ToolTip = "Get the internal ID of an output media", Category = "Odin|Debug"))
    int32 GetMediaId();

    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Get Output Media Peer ID",
                      ToolTip = "Get the peer ID of an output media", Category = "Odin|Debug"))
    int64 GetPeerId();

    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Get Output Media Audio Stats",
                      ToolTip     = "Get statistics for an output media",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                      Category = "Odin|Debug"))
    FOdinAudioStreamStats AudioStreamStats();

  protected:
    void BeginDestroy() override;

    UPROPERTY(BlueprintReadOnly, Category = "Room")
    UOdinRoom *Room;
};
