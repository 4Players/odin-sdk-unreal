#include "OdinRoom.h"

#include "OdinLibrary/include/odin.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"

#include "OdinRoom.AsyncTasks.h"

UOdinRoom::UOdinRoom(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{

    this->room_handle_ = odin_room_create();
    odin_room_set_event_callback(
        this->room_handle_,
        [](OdinRoomHandle roomHandle, const struct OdinEvent *event, void *user_data) {
            reinterpret_cast<UOdinRoom *>(user_data)->HandleOdinEvent(event);
        },
        this);
}

void UOdinRoom::BeginDestroy()
{
    this->Destroy();
    Super::BeginDestroy();
}

UOdinRoom *UOdinRoom::ConstructRoom(UObject *WorldContextObject)
{
    return NewObject<UOdinRoom>();
}

void UOdinRoom::SetPositionScale(float Scale)
{
    (new FAutoDeleteAsyncTask<UpdateScalingTask>(this->room_handle_, Scale))->StartBackgroundTask();
}

void UOdinRoom::Destroy()
{
    for (auto media : this->capture_medias_) {
        media->Reset();
    }
    this->capture_medias_.Empty();
    (new FAutoDeleteAsyncTask<DestroyRoomTask>(this->room_handle_))->StartBackgroundTask();
    this->room_handle_ = 0;
}

// void UOdinRoom::RemoveMedia(int32 mediaId) {}

void UOdinRoom::HandleOdinEvent(const struct OdinEvent *event)
{
    switch (event->tag) {
        case OdinEventTag::OdinEvent_Joined: {
            auto          own_peer_id = event->joined.own_peer_id;
            TArray<uint8> user_data{event->joined.room_user_data,
                                    (int)event->joined.room_user_data_len};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onRoomJoined.Broadcast(own_peer_id, user_data); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerJoined: {
            auto          peer_id = event->peer_joined.peer_id;
            TArray<uint8> user_data{event->peer_joined.peer_user_data,
                                    (int)event->peer_joined.peer_user_data_len};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onPeerJoined.Broadcast(peer_id, user_data); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerLeft: {
            auto peer_id = event->peer_left.peer_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onPeerLeft.Broadcast(peer_id); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerUserDataChanged: {
            auto          peer_id = event->peer_user_data_changed.peer_id;
            TArray<uint8> user_data{event->peer_user_data_changed.peer_user_data,
                                    (int)event->peer_user_data_changed.peer_user_data_len};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onPeerUserDataChanged.Broadcast(peer_id, user_data); }, TStatId(),
                nullptr, ENamedThreads::GameThread);
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
                    this->onMediaAdded.Broadcast(peer_id, n, obj);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MediaRemoved: {
            auto media_id = event->media_removed.media_id;
            auto peer_id  = event->media_removed.peer_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onMediaRemoved.Broadcast(peer_id, media_id); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MediaActiveStateChanged: {
            auto peer_id  = event->media_active_state_changed.peer_id;
            auto media_id = event->media_active_state_changed.media_id;
            auto active   = event->media_active_state_changed.active;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onMediaActiveStateChanged.Broadcast(peer_id, media_id, active); },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MessageReceived: {
            auto          peer_id = event->message_received.peer_id;
            TArray<uint8> data{event->message_received.data, (int)event->message_received.data_len};
            ;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onMessageReceived.Broadcast(peer_id, data); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_RoomConnectionStateChanged: {
            EOdinRoomConnectionState state;
            switch (event->room_connection_state_changed.state) {
                case OdinRoomConnectionState::OdinRoomConnectionState_Connected: {
                    state = EOdinRoomConnectionState::Connected;
                } break;
                case OdinRoomConnectionState::OdinRoomConnectionState_Connecting: {
                    state = EOdinRoomConnectionState::Connecting;
                } break;
                case OdinRoomConnectionState::OdinRoomConnectionState_Disconnected: {
                    state = EOdinRoomConnectionState::Disconnected;
                } break;
            }

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onConnectionStateChanged.Broadcast(state); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
    }
}
