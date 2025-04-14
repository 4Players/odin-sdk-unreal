/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinPlaybackMedia.h"
#include "OdinRoom.h"

#include "odin_sdk.h"

FOdinAudioRingBuffer::FOdinAudioRingBuffer(const int32 InCapacity)
    : Capacity(InCapacity)
    , WriteIndex(0)
{
    Buffer.Init(0, Capacity);
}

int32 FOdinAudioRingBuffer::Write(const float* InData, const int32 NumSamples)
{
    if (NumSamples > Capacity) {
        UE_LOG(Odin, Error,
               TEXT("Tried Writing more Samples than Capacity in Buffer from FAudioRingBuffer, "
                    "aborting Write."));
        return WriteIndex.GetValue();
    }

    FScopeLock WriteLock(&BufferSection);
    int32      CurrentWriteIndex = WriteIndex.GetValue();

    int32 NumFirstCopy = FMath::Min(NumSamples, Capacity - CurrentWriteIndex);
    FMemory::Memcpy(Buffer.GetData() + CurrentWriteIndex, InData, sizeof(float) * NumFirstCopy);

    if (NumFirstCopy < NumSamples) {
        int32 NumSecondCopy = NumSamples - NumFirstCopy;
        FMemory::Memcpy(Buffer.GetData(), InData + NumFirstCopy, sizeof(float) * NumSecondCopy);
    }

    WriteIndex.Set((WriteIndex.GetValue() + NumSamples) % Capacity);
    return WriteIndex.GetValue();
}

int32 FOdinAudioRingBuffer::Read(const int32 ReaderIndex, float* OutData, const int32 NumSamples)
{
    if (NumSamples > Capacity) {
        UE_LOG(Odin, Error,
               TEXT("Tried Reading more Samples than available in Buffer from FAudioRingBuffer, "
                    "aborting Write."));
        return ReaderIndex;
    }

    if (ReaderIndex >= Capacity) {
        UE_LOG(Odin, Error,
               TEXT("Invalid ReaderIndex when trying to read data from FAudioRingBuffer"));
        return ReaderIndex;
    }

    FScopeLock ReadLock(&BufferSection);

    int32 NumFirstCopy = FMath::Min(NumSamples, Capacity - ReaderIndex);
    FMemory::Memcpy(OutData, Buffer.GetData() + ReaderIndex, sizeof(float) * NumFirstCopy);

    if (NumFirstCopy < Capacity) {
        int32 NumSecondCopy = NumSamples - NumFirstCopy;
        FMemory::Memcpy(OutData + NumFirstCopy, Buffer.GetData(), sizeof(float) * NumSecondCopy);
    }
    return (ReaderIndex + NumSamples) % Capacity;
}

int32 FOdinAudioRingBuffer::GetAvailableSamples(const int32 ReaderIndex) const
{
    if (ReaderIndex >= Capacity) {
        UE_LOG(Odin, Error, TEXT("Invalid ReaderIndex when trying to get available sample count."));
        return 0;
    }
    if (WriteIndex.GetValue() >= ReaderIndex) {
        return WriteIndex.GetValue() - ReaderIndex;
    }
    return Capacity - (ReaderIndex - WriteIndex.GetValue());
}

UOdinPlaybackMedia::UOdinPlaybackMedia()
{
    MultipleAccessCacheBuffer = MakeShared<FOdinAudioRingBuffer, ESPMode::ThreadSafe>(1024 * 4);
}

UOdinPlaybackMedia::UOdinPlaybackMedia(OdinMediaStreamHandle streamHandle, UOdinRoom* Room)
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

OdinReturnCode UOdinPlaybackMedia::ReadData(int32& RefReaderIndex, float* OutAudio,
                                            int32 NumSamples)
{
    int32 AvailableSamples = MultipleAccessCacheBuffer->GetAvailableSamples(RefReaderIndex);
    if (AvailableSamples < NumSamples) {
        CachedReturnCode = odin_audio_read_data(GetMediaHandle(), OutAudio, NumSamples);
        if (!odin_is_error(CachedReturnCode)) {
            RefReaderIndex = MultipleAccessCacheBuffer->Write(OutAudio, NumSamples);
        }
    } else {
        RefReaderIndex = MultipleAccessCacheBuffer->Read(RefReaderIndex, OutAudio, NumSamples);
    }
    return CachedReturnCode;
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
