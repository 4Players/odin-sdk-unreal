#include "OdinRoom.h"

#include "OdinLibrary/include/odin.h"

#include "Async/AsyncWork.h"

class JoinRoomTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<JoinRoomTask>;

    std::shared_ptr<struct OdinRoom> room;
    FString                          Url;
    FString                          RoomToken;

    JoinRoomTask(std::shared_ptr<struct OdinRoom> room, FString url, FString room_token)
        : room(room)
        , Url(url)
        , RoomToken(room_token)
    {
    }

    void DoWork()
    {
        auto join_room_result =
            odin_room_join(room.get(), TCHAR_TO_UTF8(*Url), TCHAR_TO_UTF8(*RoomToken));

        char buf[200] = {0};
        odin_error_format(join_room_result, buf, sizeof(buf));

        (void)(buf);
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(JoinRoomTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class DestroyRoomTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<DestroyRoomTask>;

    OdinRoom *room;

    DestroyRoomTask(OdinRoom *room)
        : room(room)
    {
    }

    void DoWork()
    {
        odin_room_destroy(room);
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(DestroyRoomTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

UOdinRoom::UOdinRoom(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{

    this->room_ = std::shared_ptr<OdinRoom>{odin_room_create(), [](struct OdinRoom *Obj) {
                                                auto *MyTask =
                                                    new FAutoDeleteAsyncTask<DestroyRoomTask>(Obj);
                                                MyTask->StartBackgroundTask();
                                            }};

    odin_room_set_event_callback(
        this->room_.get(),
        [](struct OdinRoom *room, const struct OdinEvent *event, void *user_data) {
            reinterpret_cast<UOdinRoom *>(user_data)->HandleOdinEvent(event);
        },
        this);
}

void UOdinRoom::BeginDestroy()
{
    for (auto media : this->capture_medias_) {
        media->Reset();
    }
    this->capture_medias_.Empty();
    this->room_ = nullptr;
    Super::BeginDestroy();
}

UOdinRoom *UOdinRoom::ConstructRoom(UObject *WorldContextObject, const FString &url,
                                    const FString &token)
{
    auto room = NewObject<UOdinRoom>();
    room->SetInitParams(url, token);
    return room;
}

void UOdinRoom::OdinRoom_AddMedia(UOdinCaptureMedia *capture_media)
{
    auto result   = odin_room_add_media(this->room_.get(), capture_media->GetMedia());
    char buf[200] = {0};
    odin_error_format(result, buf, sizeof(buf));

    this->capture_medias_.Add(capture_media);
}

void UOdinRoom::OdinRoom_Join(const TArray<uint8> &user_data)
{
    auto *MyTask = new FAutoDeleteAsyncTask<JoinRoomTask>(room_, Url, RoomId);
    MyTask->StartBackgroundTask();
}

void UOdinRoom::SetInitParams(const FString &url, const FString &room_id)
{
    this->Url    = url;
    this->RoomId = room_id;
}

void UOdinRoom::HandleOdinEvent(const struct OdinEvent *event)
{
    switch (event->tag) {
        case OdinEventTag::OdinEvent_Joined: {
            auto own_peer_id   = event->joined.own_peer_id;
            auto user_data = BytesToString(event->joined.room_user_data,
                                           event->joined.room_user_data_len);
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    const auto obj = UOdinJsonObject::ConstructJsonObject(GetWorld());
                    obj->DecodeJson(user_data);
                    this->onRoomJoined.Broadcast(own_peer_id, obj);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerJoined: {
            auto peer_id   = event->peer_joined.peer_id;
            auto user_data = BytesToString(event->peer_joined.peer_user_data,
                                           event->peer_joined.peer_user_data_len);
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    const auto obj = UOdinJsonObject::ConstructJsonObject(GetWorld());
                    obj->DecodeJson(user_data);
                    this->onPeerJoined.Broadcast(peer_id, obj);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerLeft: {
            auto peer_id = event->peer_left.peer_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onPeerLeft.Broadcast(peer_id); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerUserDataChanged: {
            auto peer_id   = event->peer_user_data_changed.peer_id;
            auto user_data = BytesToString(event->peer_joined.peer_user_data,
                                           event->peer_joined.peer_user_data_len);
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    const auto obj = UOdinJsonObject::ConstructJsonObject(GetWorld());
                    obj->DecodeJson(user_data);
                    this->onPeerUserDataChanged.Broadcast(peer_id, obj);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MediaAdded: {
            auto stream  = event->media_added.stream;
            auto peer_id = event->media_added.peer_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    const auto obj = UOdinJsonObject::ConstructJsonObject(GetWorld());
                    auto       n   = NewObject<UOdinPlaybackMedia>();
                    n->SetMedia(stream);
                    n->SetRoom(this);
                    n->SetSharedRoom(this->room_);
                    this->onMediaAdded.Broadcast(peer_id, n, obj);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MediaRemoved: {
            auto media_id = event->media_removed.media_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onMediaRemoved.Broadcast(media_id); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MediaActiveStateChanged: {
            auto peer_id  = event->media_active_state_changed.peer_id;
            auto media_id = event->media_active_state_changed.media_id;
            auto active   = event->media_active_state_changed.active;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onMediaActiveStateChanged.Broadcast(peer_id, media_id, active); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
    }
}