#include "OdinPlaybackMedia.h"
#include "OdinRoom.h"

#include "OdinLibrary/include/odin.h"

UOdinPlaybackMedia::UOdinPlaybackMedia() {}

UOdinPlaybackMedia::UOdinPlaybackMedia(struct OdinMediaStream *stream, UOdinRoom *Room)
    : UOdinPlaybackMedia()
{
    this->stream_ = stream;
    this->Room    = Room;
}

void UOdinPlaybackMedia::BeginDestroy()
{
    if (this->stream_) {
        odin_media_stream_destroy(this->stream_);
        this->stream_ = nullptr;
    }
    this->Room = nullptr;
    Super::BeginDestroy();
}
