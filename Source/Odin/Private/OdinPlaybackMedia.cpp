/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#include "OdinPlaybackMedia.h"
#include "OdinRoom.h"

#include "OdinCore/include/odin.h"

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

void UOdinPlaybackMedia::BeginDestroy()
{
    if (this->stream_handle_) {
        odin_media_stream_destroy(this->stream_handle_);
        this->stream_handle_ = 0;
    }
    this->Room = nullptr;
    Super::BeginDestroy();
}
