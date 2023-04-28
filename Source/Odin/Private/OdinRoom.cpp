/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#include "OdinRoom.h"

#include "OdinCore/include/odin.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"
#include "Engine/World.h"

#include "OdinRoom.AsyncTasks.h"
#include "Odin.h"

UOdinRoom::UOdinRoom(const class FObjectInitializer& PCIP)
    : Super(PCIP)
{

    this->room_handle_ = odin_room_create();
    odin_room_set_event_callback(
        this->room_handle_,
        [](OdinRoomHandle roomHandle, const struct OdinEvent* event, void* user_data) {
            UOdinRoom* room = reinterpret_cast<UOdinRoom*>(user_data);
            if (room && room->IsValidLowLevel()) {
                room->HandleOdinEvent(event);
            }
        },
        this);
}

UOdinRoom::~UOdinRoom()
{
    odin_room_set_event_callback(this->room_handle_, nullptr, nullptr);
}

void UOdinRoom::BeginDestroy()
{
    Super::BeginDestroy();
    this->Destroy();
}

void UOdinRoom::FinishDestroy()
{
    Super::FinishDestroy();
    if (this->room_handle_) {
        odin_room_set_event_callback(room_handle_, nullptr, nullptr);
    }
}

UOdinRoom* UOdinRoom::ConstructRoom(UObject*                WorldContextObject,
                                    const FOdinApmSettings& InitialAPMSettings)
{
    auto room = NewObject<UOdinRoom>();
    room->UpdateAPMConfig(InitialAPMSettings);
    return room;
}

void UOdinRoom::SetPositionScale(float Scale)
{
    (new FAutoDeleteAsyncTask<UpdateScalingTask>(this->room_handle_, Scale))->StartBackgroundTask();
}

FOdinConnectionStats UOdinRoom::ConnectionStats()
{
    OdinConnectionStats stats  = OdinConnectionStats();
    auto                result = odin_room_connection_stats(this->room_handle_, &stats);

    if (odin_is_error(result)) {
        UE_LOG(LogTemp, Warning, TEXT("odin_room_connection_stats result: %d"), result);
    } else {
        FOdinConnectionStats RoomStats;
        RoomStats.udp_tx_datagrams  = stats.udp_tx_datagrams;
        RoomStats.udp_tx_acks       = stats.udp_tx_acks;
        RoomStats.udp_tx_bytes      = stats.udp_tx_bytes;
        RoomStats.udp_rx_datagrams  = stats.udp_rx_datagrams;
        RoomStats.udp_rx_acks       = stats.udp_rx_acks;
        RoomStats.udp_rx_bytes      = stats.udp_rx_bytes;
        RoomStats.cwnd              = stats.cwnd;
        RoomStats.congestion_events = stats.congestion_events;
        RoomStats.rtt               = stats.rtt;

        return RoomStats;
    }

    return {};
}

void UOdinRoom::UpdateAPMConfig(FOdinApmSettings apm_config)
{
    this->current_apm_settings_ = apm_config;
    auto odin_apm_config = OdinApmConfig{};
    odin_apm_config.voice_activity_detection = apm_config.bVoiceActivityDetection;
    odin_apm_config.voice_activity_detection_attack_probability = apm_config.fVadAttackProbability;
    odin_apm_config.voice_activity_detection_release_probability =
        apm_config.fVadReleaseProbability;
    odin_apm_config.volume_gate                  = apm_config.bEnableVolumeGate;
    odin_apm_config.volume_gate_attack_loudness  = apm_config.fVolumeGateAttackLoudness;
    odin_apm_config.volume_gate_release_loudness = apm_config.fVolumeGateReleaseLoudness;
    odin_apm_config.echo_canceller               = apm_config.bEchoCanceller;
    odin_apm_config.high_pass_filter             = apm_config.bHighPassFilter;
    odin_apm_config.pre_amplifier                = apm_config.bPreAmplifier;
    odin_apm_config.transient_suppressor         = apm_config.bTransientSuppresor;
    odin_apm_config.gain_controller              = apm_config.bGainController;

    if (odin_apm_config.echo_canceller) {
        if (!submix_listener_) {
            submix_listener_ = NewObject<UOdinSubmixListener>();
            submix_listener_->SetRoom(this->room_handle_);
        }
        submix_listener_->StartSubmixListener();
    } else if (submix_listener_) {
        submix_listener_->StopSubmixListener();
    }

    switch (apm_config.noise_suppression_level) {
        case OdinNS_None: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_None;
        }
        break;
        case OdinNS_Low: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_Low;
        }
        break;
        case OdinNS_Moderate: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_Moderate;
        }
        break;
        case OdinNS_High: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_High;
        }
        break;
        case OdinNS_VeryHigh: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_VeryHigh;
        }
        break;
    }
    odin_room_configure_apm(this->room_handle_, odin_apm_config);
}

void UOdinRoom::Destroy()
{
    {
        FScopeLock lock(&this->capture_medias_cs_);
        for (auto media : this->capture_medias_) {
            if (nullptr != media) {
                media->Reset();
            }
        }
        this->capture_medias_.Empty();
    }
    (new FAutoDeleteAsyncTask<DestroyRoomTask>(this->room_handle_))->StartBackgroundTask();

    {
        FScopeLock lock(&this->medias_cs_);
        
        this->medias_.Empty();
    }
    
    
    onPeerJoined.Clear();
    onPeerLeft.Clear();
    onPeerUserDataChanged.Clear();
    onMediaAdded.Clear();
    onMediaRemoved.Clear();
    onMediaActiveStateChanged.Clear();
    onRoomJoined.Clear();
    onRoomUserDataChanged.Clear();
    onConnectionStateChanged.Clear();
    onMessageReceived.Clear();
}

void UOdinRoom::BindCaptureMedia(UOdinCaptureMedia* media)
{
    {
        FScopeLock lock(&this->capture_medias_cs_);
        this->capture_medias_.Add(media);
    }
    {
        FScopeLock lock(&this->medias_cs_);
        this->medias_.Add(media->GetMediaHandle(), media);
    }
}

void UOdinRoom::UnbindCaptureMedia(UOdinCaptureMedia* media)
{
    {
        FScopeLock lock(&this->capture_medias_cs_);
        this->capture_medias_.Remove(media);
    }

    {
        FScopeLock lock(&this->medias_cs_);
        this->medias_.Remove(media->GetMediaHandle());
    }
}

void UOdinRoom::HandleOdinEvent(const struct OdinEvent* event)
{
    switch (event->tag) {
        case OdinEvent_Joined: {
            auto          own_peer_id = event->joined.own_peer_id;
            TArray<uint8> user_data{event->joined.room_user_data,
                                    static_cast<int>(event->joined.room_user_data_len)};

            {
                FString roomId       = UTF8_TO_TCHAR(event->joined.room_id);
                FString roomCustomer = UTF8_TO_TCHAR(event->joined.customer);
                FString own_user_id  = UTF8_TO_TCHAR(event->joined.own_user_id);
                if (IsValidLowLevel() && joined_callbacks_cs_.TryLock()) {
                    for (auto& callback : this->joined_callbacks_) {

                        callback(roomId, roomCustomer, user_data, own_peer_id, own_user_id);
                    }
                    this->joined_callbacks_.Reset();
                    joined_callbacks_cs_.Unlock();
                }
            }
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    if (IsValidLowLevel()) {
                        this->onRoomJoined.Broadcast(own_peer_id, user_data, this);
                    }

                }, TStatId(),
                nullptr, ENamedThreads::GameThread);
        }
        break;
        case OdinEvent_PeerJoined: {
            auto          peer_id = event->peer_joined.peer_id;
            FString       user_id = UTF8_TO_TCHAR(event->peer_joined.user_id);
            TArray<uint8> user_data{event->peer_joined.peer_user_data,
                                    static_cast<int>(event->peer_joined.peer_user_data_len)};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    if (IsValidLowLevel()) {
                        this->onPeerJoined.Broadcast(peer_id, user_id, user_data, this);
                    }
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
        break;
        case OdinEvent_PeerLeft: {
            auto peer_id = event->peer_left.peer_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    if (IsValidLowLevel()) {
                        this->onPeerLeft.Broadcast(peer_id, this);

                    }
                }, TStatId(), nullptr,
                ENamedThreads::GameThread);
        }
        break;
        case OdinEvent_PeerUserDataChanged: {
            auto          peer_id = event->peer_user_data_changed.peer_id;
            TArray<uint8> user_data{event->peer_user_data_changed.peer_user_data,
                                    static_cast<int>(event->peer_user_data_changed.
                                                            peer_user_data_len)};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    if (IsValidLowLevel()) {
                        this->onPeerUserDataChanged.Broadcast(peer_id, user_data, this);

                    }
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
        break;
        case OdinEvent_MediaAdded: {
            auto media_handle = event->media_added.media_handle;
            auto peer_id      = event->media_added.peer_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    if (IsValidLowLevel()) {
                        const auto obj = UOdinJsonObject::ConstructJsonObject(GetWorld());
                        auto       playback_media = NewObject<UOdinPlaybackMedia>();
                        playback_media->SetMediaHandle(media_handle);
                        playback_media->SetRoom(this);
                        {
                            FScopeLock lock(&this->medias_cs_);
                            medias_.Add(media_handle, playback_media);
                        }
                        this->onMediaAdded.Broadcast(peer_id, playback_media, obj, this);

                    }

                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
        break;
        case OdinEvent_MediaRemoved: {
            auto            media_handle = event->media_removed.media_handle;
            auto            peer_id      = event->media_removed.peer_id;
            UOdinMediaBase* base_media   = nullptr;

            bool bFoundMedia;
            {
                FScopeLock lock(&this->medias_cs_);
                bFoundMedia = medias_.RemoveAndCopyValue(media_handle, base_media);
            }
            if (bFoundMedia) {
                auto playback_media = Cast<UOdinPlaybackMedia>(base_media);
                UE_LOG(Odin, Warning, TEXT("Removed media"));
                FFunctionGraphTask::CreateAndDispatchWhenReady(
                    [=]() {
                        if (IsValidLowLevel()) {
                            this->onMediaRemoved.Broadcast(peer_id, playback_media, this);

                        }
                    },
                    TStatId(), nullptr, ENamedThreads::GameThread);
            }
        }
        break;
        case OdinEvent_MediaActiveStateChanged: {
            auto   peer_id      = event->media_active_state_changed.peer_id;
            auto   media_handle = event->media_active_state_changed.media_handle;
            auto   active       = event->media_active_state_changed.active;
            auto** media        = medias_.Find(media_handle);
            if (media) {
                FFunctionGraphTask::CreateAndDispatchWhenReady(
                    [=]() {
                        if (IsValidLowLevel()) {
                            this->onMediaActiveStateChanged.Broadcast(peer_id, *media, active,
                                this);
                        }
                    },
                    TStatId(), nullptr, ENamedThreads::GameThread);
            }
        }
        break;
        case OdinEvent_MessageReceived: {
            auto          peer_id = event->message_received.peer_id;
            TArray<uint8> data{event->message_received.data,
                               static_cast<int>(event->message_received.data_len)};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    if (IsValidLowLevel()) {
                        this->onMessageReceived.Broadcast(peer_id, data, this);
                    }
                }, TStatId(),
                nullptr, ENamedThreads::GameThread);
        }
        break;
        case OdinEvent_RoomConnectionStateChanged: {
            EOdinRoomConnectionState state;
            switch (event->room_connection_state_changed.state) {
                case OdinRoomConnectionState_Connected: {
                    state = Connected;
                }
                break;
                case OdinRoomConnectionState_Connecting: {
                    state = Connecting;
                }
                break;
                case OdinRoomConnectionState_Disconnecting: {
                    state = Disconnecting;
                }
                break;
                case OdinRoomConnectionState_Disconnected: {
                    state = Disconnected;
                }
                break;
            }

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() {
                    if (IsValidLowLevel()) {
                        this->onConnectionStateChanged.Broadcast(state, this);
                    }
                }, TStatId(),
                nullptr, ENamedThreads::GameThread);
        }
        break;
    }
}
