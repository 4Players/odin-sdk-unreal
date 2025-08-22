/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "OdinMediaBase.h"
#include "UObject/Object.h"
#include "odin_sdk.h"
#include "OdinPlaybackStreamReader.h"
#include <memory>

#include "Odin.h"
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
    UOdinPlaybackMedia(OdinMediaStreamHandle streamHandle, UOdinRoom* room);

    /**
     * Sets the room for the playback media object to the provided room pointer.
     *
     * @param room Pointer to the UOdinRoom object to set as the room for the playback media.
     */
    void SetRoom(UOdinRoom* room)
    {
        this->Room = room;
    }

    /**
     * Retrieves the internal ID of the media from the Odin SDK.
     *
     * @return The internal media ID that uniquely identifies the media.
     */
    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Get Output Media ID",
                      ToolTip = "Get the internal ID of an output media", Category = "Odin|Debug"))
    int32 GetMediaId();

    virtual void SetMediaHandle(OdinMediaStreamHandle handle) override;

    /**
     * @brief Get the peer ID associated with the media stream.
     *
     * @return The peer ID of the media stream.
     */
    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Get Output Media Peer ID",
                      ToolTip = "Get the peer ID of an output media", Category = "Odin|Debug"))
    int64 GetPeerId();

    /**
     * Retrieves the statistics of the audio stream including the total packets, processed packets,
     * packets arrived too early, packets arrived too late, dropped packets, invalid packets,
     * repeated packets, and lost packets.
     *
     * @return FOdinAudioStreamStats struct containing the audio stream statistics.
     */
    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Get Output Media Audio Stats",
                      ToolTip     = "Get statistics for an output media",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                      Category = "Odin|Debug"))
    FOdinAudioStreamStats AudioStreamStats();

    [[deprecated("Please use GetPlaybackStreamReader() to access the "
                 "FOdinPlaybackStreamReader::ReadData function.")]]
    OdinReturnCode ReadData(int32& RefReaderIndex, float* OutAudio, int32 NumSamples);

    TSharedPtr<FOdinPlaybackStreamReader, ESPMode::ThreadSafe> GetPlaybackStreamReader() const;

    virtual void AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener) override;
    virtual void RemoveAudioBufferListener(IAudioBufferListener* AudioBufferListener) override;

  protected:
    virtual void BeginDestroy() override;

    UPROPERTY(BlueprintReadOnly, Category = "Odin | Playback")
    UOdinRoom* Room;

    /**
     * The Audio Buffer Capacity in seconds.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Odin | Playback")
    float AudioBufferCapacity = 0.1f;

    TSharedPtr<FOdinPlaybackStreamReader, ESPMode::ThreadSafe> PlaybackStreamReader;
};