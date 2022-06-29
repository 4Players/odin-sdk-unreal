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

UOdinRoom *UOdinRoom::ConstructRoom(UObject                *WorldContextObject,
                                    const FOdinApmSettings &InitialAPMSettings)
{
    auto room = NewObject<UOdinRoom>();
    room->UpdateAPMConfig(InitialAPMSettings);
    return room;
}

void UOdinRoom::SetPositionScale(float Scale)
{
    (new FAutoDeleteAsyncTask<UpdateScalingTask>(this->room_handle_, Scale))->StartBackgroundTask();
}

void UOdinRoom::UpdateAPMConfig(const FOdinApmSettings &apm_config)
{
    this->current_apm_settings_              = apm_config;
    auto odin_apm_config                     = OdinApmConfig{};
    odin_apm_config.voice_activity_detection = apm_config.bVoiceActivityDetection;
    odin_apm_config.voice_activity_detection_attack_probability = apm_config.fVadAttackProbability;
    odin_apm_config.voice_activity_detection_release_probability =
        apm_config.fVadReleaseProbability;
    odin_apm_config.volume_gate                  = apm_config.bEnableVolumeGate;
    odin_apm_config.volume_gate_attack_loudness  = apm_config.fVolumeGateAttackLoudness;
    odin_apm_config.volume_gate_release_loudness = apm_config.fVolumeGateReleaseLoudness;
    odin_apm_config.echo_canceller               = false;
    odin_apm_config.high_pass_filter             = apm_config.bHighPassFilter;
    odin_apm_config.pre_amplifier                = apm_config.bPreAmplifier;
    odin_apm_config.transient_suppressor         = apm_config.bTransientSuppresor;

    switch (apm_config.noise_suppression_level) {
        case EOdinNoiseSuppressionLevel::None: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_None;
        } break;
        case EOdinNoiseSuppressionLevel::Low: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_Low;
        } break;
        case EOdinNoiseSuppressionLevel::Moderate: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_Moderate;
        } break;
        case EOdinNoiseSuppressionLevel::High: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_High;
        } break;
        case EOdinNoiseSuppressionLevel::VeryHigh: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_VeryHigh;
        } break;
    }
    odin_room_configure_apm(this->room_handle_, odin_apm_config);
}

void UOdinRoom::Destroy()
{
    {
        FScopeLock lock(&this->capture_medias_cs_);
        for (auto media : this->capture_medias_) {
            media->Reset();
        }
        this->capture_medias_.Empty();
    }
    (new FAutoDeleteAsyncTask<DestroyRoomTask>(this->room_handle_))->StartBackgroundTask();
    this->room_handle_ = 0;
}

void UOdinRoom::BindCaptureMedia(UOdinCaptureMedia *media)
{
    FScopeLock lock(&this->capture_medias_cs_);
    this->capture_medias_.Add(media);
}

void UOdinRoom::UnbindCaptureMedia(UOdinCaptureMedia *media)
{
    FScopeLock lock(&this->capture_medias_cs_);
    this->capture_medias_.Remove(media);
}

// void UOdinRoom::RemoveMedia(int32 mediaId) {}

void UOdinRoom::HandleOdinEvent(const struct OdinEvent *event)
{
    switch (event->tag) {
        case OdinEventTag::OdinEvent_Joined: {
            auto          own_peer_id = event->joined.own_peer_id;
            TArray<uint8> user_data{event->joined.room_user_data,
                                    (int)event->joined.room_user_data_len};

            {
                FString       roomId       = UTF8_TO_TCHAR(event->joined.room_id);
                FString       roomCustomer = UTF8_TO_TCHAR(event->joined.customer);
                FScopeLock lock(&joined_callbacks_cs_);
                for (auto &callback : this->joined_callbacks_) {
                    callback(roomId, roomCustomer, user_data, own_peer_id);
                }
                this->joined_callbacks_.Reset();
            }
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onRoomJoined.Broadcast(own_peer_id, user_data, this); }, TStatId(),
                nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerJoined: {
            auto          peer_id = event->peer_joined.peer_id;
            TArray<uint8> user_data{event->peer_joined.peer_user_data,
                                    (int)event->peer_joined.peer_user_data_len};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onPeerJoined.Broadcast(peer_id, user_data, this); }, TStatId(),
                nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerLeft: {
            auto peer_id = event->peer_left.peer_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onPeerLeft.Broadcast(peer_id, this); }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_PeerUserDataChanged: {
            auto          peer_id = event->peer_user_data_changed.peer_id;
            TArray<uint8> user_data{event->peer_user_data_changed.peer_user_data,
                                    (int)event->peer_user_data_changed.peer_user_data_len};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onPeerUserDataChanged.Broadcast(peer_id, user_data, this); },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MediaAdded: {
            auto stream  = event->media_added.stream;
            auto peer_id = event->media_added.peer_id;
            // uint16_t media_id = 0;
            // odin_media_stream_media_id(stream, &media_id);
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    const auto obj = UOdinJsonObject::ConstructJsonObject(GetWorld());
                    auto       n   = NewObject<UOdinPlaybackMedia>();
                    n->SetMedia(stream);
                    n->SetRoom(this);
                    // medias_.Add(media_id, n);
                    this->onMediaAdded.Broadcast(peer_id, n, obj, this);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MediaRemoved: {
            auto media_id = event->media_removed.media_id;
            auto peer_id  = event->media_removed.peer_id;
            // auto playback_media = medias_.Get(media_id);
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onMediaRemoved.Broadcast(peer_id, media_id, this); }, TStatId(),
                nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MediaActiveStateChanged: {
            auto peer_id  = event->media_active_state_changed.peer_id;
            auto media_id = event->media_active_state_changed.media_id;
            auto active   = event->media_active_state_changed.active;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    this->onMediaActiveStateChanged.Broadcast(peer_id, media_id, active, this);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEventTag::OdinEvent_MessageReceived: {
            auto          peer_id = event->message_received.peer_id;
            TArray<uint8> data{event->message_received.data, (int)event->message_received.data_len};
            ;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { this->onMessageReceived.Broadcast(peer_id, data, this); }, TStatId(),
                nullptr, ENamedThreads::GameThread);
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
                [=]() { this->onConnectionStateChanged.Broadcast(state, this); }, TStatId(),
                nullptr, ENamedThreads::GameThread);
        } break;
    }
}
