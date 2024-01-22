/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinPlaybackMedia.h"
#include "OdinRoom.h"

#include "odin_sdk.h"

UOdinPlaybackMedia::UOdinPlaybackMedia() {}

UOdinPlaybackMedia::UOdinPlaybackMedia(OdinMediaStreamHandle streamHandle, UOdinRoom *Room)
    : UOdinPlaybackMedia()
{
    this->stream_handle_ = streamHandle;
    this->Room           = Room;
}

int32 UOdinPlaybackMedia::GetMediaId()
{
    uint16_t media_id;
    odin_media_stream_media_id(stream_handle_, &media_id);
    return media_id;
}

int64 UOdinPlaybackMedia::GetPeerId()
{
    uint64_t peer_id;
    odin_media_stream_peer_id(stream_handle_, &peer_id);
    return peer_id;
}

FOdinAudioStreamStats UOdinPlaybackMedia::AudioStreamStats()
{
    OdinAudioStreamStats stats  = OdinAudioStreamStats();
    auto                 result = odin_audio_stats(this->stream_handle_, &stats);

    if (odin_is_error(result)) {
        UE_LOG(LogTemp, Warning, TEXT("odin_audio_stats result: %d"), result);
    } else {
        FOdinAudioStreamStats audio_stats;
        audio_stats.packets_total             = stats.packets_total;
        audio_stats.packets_processed         = stats.packets_processed;
        audio_stats.packets_arrived_too_early = stats.packets_arrived_too_early;
        audio_stats.packets_arrived_too_late  = stats.packets_arrived_too_late;
        audio_stats.packets_dropped           = stats.packets_dropped;
        audio_stats.packets_invalid           = stats.packets_invalid;
        audio_stats.packets_repeated          = stats.packets_repeated;
        audio_stats.packets_lost              = stats.packets_lost;

        return audio_stats;
    }

    return {};
}

void UOdinPlaybackMedia::BeginDestroy()
{
    Super::BeginDestroy();
    if (this->stream_handle_) {
        odin_media_stream_destroy(this->stream_handle_);
        this->stream_handle_ = 0;
    }
    Room = nullptr;
}
