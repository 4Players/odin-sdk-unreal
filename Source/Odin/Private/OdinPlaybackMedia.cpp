/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinPlaybackMedia.h"
#include "OdinRoom.h"
#include "odin_sdk.h"

UOdinPlaybackMedia::UOdinPlaybackMedia() {}

UOdinPlaybackMedia::UOdinPlaybackMedia(OdinMediaStreamHandle streamHandle, UOdinRoom* Room)
    : UOdinPlaybackMedia()
{
    UOdinPlaybackMedia::SetMediaHandle(streamHandle);
    SetRoom(Room);
}

int32 UOdinPlaybackMedia::GetMediaId()
{
    uint16_t media_id;
    odin_media_stream_media_id(stream_handle_, &media_id);
    return media_id;
}

void UOdinPlaybackMedia::SetMediaHandle(OdinMediaStreamHandle handle)
{
    Super::SetMediaHandle(handle);
    const int32 Capacity =
        ODIN_DEFAULT_CHANNEL_COUNT * ODIN_DEFAULT_SAMPLE_RATE * AudioBufferCapacity;
    PlaybackStreamReader =
        MakeShared<FOdinPlaybackStreamReader, ESPMode::ThreadSafe>(handle, Capacity);
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
        UE_LOG(Odin, Warning, TEXT("odin_audio_stats result: %d"), result);
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

OdinReturnCode UOdinPlaybackMedia::ReadData(int32& RefReaderIndex, float* OutAudio,
                                            int32 NumSamples)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinPlaybackMedia::ReadData)
    if (PlaybackStreamReader.IsValid()) {
        return PlaybackStreamReader->ReadData(RefReaderIndex, OutAudio, NumSamples);
    }
    return 0;
}

TSharedPtr<FOdinPlaybackStreamReader, ESPMode::ThreadSafe>
UOdinPlaybackMedia::GetPlaybackStreamReader() const
{
    return PlaybackStreamReader;
}

void UOdinPlaybackMedia::AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    if (PlaybackStreamReader.IsValid()) {
        PlaybackStreamReader->AddAudioBufferListener(InAudioBufferListener);
    }
}

void UOdinPlaybackMedia::RemoveAudioBufferListener(IAudioBufferListener* AudioBufferListener)
{
    if (PlaybackStreamReader.IsValid()) {
        PlaybackStreamReader->RemoveAudioBufferListener(AudioBufferListener);
    }
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
