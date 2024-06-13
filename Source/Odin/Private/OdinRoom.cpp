/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinRoom.h"

#include "odin_sdk.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"
#include "Engine/World.h"

#include "Odin.h"
#include "OdinFunctionLibrary.h"
#include "OdinRoom.AsyncTasks.h"
#include "OdinSubsystem.h"

UOdinRoom::UOdinRoom(const class FObjectInitializer& PCIP)
    : Super(PCIP)
    , room_handle_(0)
    , submix_listener_(nullptr)
{
}

void UOdinRoom::BeginDestroy()
{
    Super::BeginDestroy();
    this->CleanUp();
    DeregisterRoomFromSubsystem();
}

void UOdinRoom::FinishDestroy()
{
    odin_room_destroy(room_handle_);
    Super::FinishDestroy();
}

void UOdinRoom::Destroy()
{
    CleanUp();
}

bool UOdinRoom::IsConnected() const
{
    UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get();
    if (OdinSubsystem) {

        const bool bIsRegistered = OdinSubsystem->IsRoomRegistered(room_handle_);
        const bool bIsConnected =
            LastRoomConnectionStateChangedData.State == EOdinRoomConnectionState::Connected;
        return bIsRegistered && bIsConnected;
    }
    return false;
}

void UOdinRoom::CleanUp()
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

    {
        FScopeLock lock(&this->medias_cs_);
        this->medias_.Empty();
    }

    // UE_LOG(Odin, Log, TEXT("Pre room close, room handle: %lld"), room_handle_);
    OdinReturnCode ReturnCode = odin_room_close(room_handle_);
    if (odin_is_error(ReturnCode)) {
        FString FormattedCloseError = UOdinFunctionLibrary::FormatError(ReturnCode, false);
        UE_LOG(Odin, Error, TEXT("Error while closing Odin room: %s"), *FormattedCloseError);
    }
    // UE_LOG(Odin, Log, TEXT("Post room close, pre set event callback, room handle: %lld"),
    // room_handle_); odin_room_set_event_callback(room_handle_, nullptr, nullptr); UE_LOG(Odin,
    // Log, TEXT("Post set event callback, pre room destroy, room handle: %lld"), room_handle_);
    // odin_room_destroy(room_handle_);
    // UE_LOG(Odin, Log, TEXT("Post room destroy callback, room handle: %lld"), room_handle_);
}

void UOdinRoom::DeregisterRoomFromSubsystem()
{
    UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get();
    if (OdinSubsystem) {
        OdinSubsystem->DeregisterRoom(this->room_handle_);
    }
}

UOdinRoom* UOdinRoom::ConstructRoom(UObject*                WorldContextObject,
                                    const FOdinApmSettings& InitialAPMSettings)
{
    if (!GEngine) {
        UE_LOG(Odin, Error,
               TEXT("Aborted Odin Room Construction due to invalid GEngine reference."))
        return nullptr;
    }

    UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get();
    if (!OdinSubsystem) {
        UE_LOG(Odin, Error,
               TEXT("Aborted Odin Room Construction due to invalid Odin Subsystem reference."))
        return nullptr;
    }

    auto room          = NewObject<UOdinRoom>(WorldContextObject);
    room->room_handle_ = odin_room_create();
    OdinSubsystem->RegisterRoom(room->room_handle_, room);
    odin_room_set_event_callback(
        room->room_handle_,
        [](OdinRoomHandle Handle, const struct OdinEvent* Event, void* UserData) {
            if (nullptr != Event) {
                HandleOdinEvent(Handle, *Event);
            }
        },
        nullptr);

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
        UE_LOG(Odin, Warning, TEXT("odin_room_connection_stats result: %d"), result);
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
    this->current_apm_settings_              = apm_config;
    auto odin_apm_config                     = OdinApmConfig{};
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
        if (submix_listener_ == nullptr) {
            submix_listener_ = NewObject<UOdinSubmixListener>(this);
            submix_listener_->SetRoom(this->room_handle_);
        }
        if (!submix_listener_->IsListening())
            submix_listener_->StartSubmixListener();
    } else if (submix_listener_ != nullptr && submix_listener_->IsListening()) {
        submix_listener_->StopSubmixListener();
    }

    switch (apm_config.noise_suppression_level) {
        case OdinNS_None: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_None;
        } break;
        case OdinNS_Low: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_Low;
        } break;
        case OdinNS_Moderate: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_Moderate;
        } break;
        case OdinNS_High: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_High;
        } break;
        case OdinNS_VeryHigh: {
            odin_apm_config.noise_suppression_level = OdinNoiseSuppressionLevel_VeryHigh;
        } break;
        default:;
    }
    odin_room_configure_apm(this->room_handle_, odin_apm_config);
}

void UOdinRoom::UpdateAPMStreamDelay(int64 DelayInMs)
{
    odin_audio_set_stream_delay(this->room_handle_, DelayInMs);
}

void UOdinRoom::BindCaptureMedia(UOdinCaptureMedia* media)
{
    if (!media) {
        return;
    }

    media->SetRoom(this);
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
    if (!media) {
        return;
    }

    media->RemoveRoom();
    {
        FScopeLock lock(&this->capture_medias_cs_);
        this->capture_medias_.Remove(media);
    }

    {
        FScopeLock lock(&this->medias_cs_);
        this->medias_.Remove(media->GetMediaHandle());
    }
}

void UOdinRoom::HandleOdinEvent(OdinRoomHandle RoomHandle, const OdinEvent event)
{
    UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get();
    if (!OdinSubsystem) {
        UE_LOG(Odin, Error,
               TEXT("Aborting HandleOdinEvent due to invalid OdinSubsystem reference."));
        return;
    }

    TWeakObjectPtr<UOdinRoom> WeakOdinRoom = OdinSubsystem->GetRoomByHandle(RoomHandle);
    if (!UOdinFunctionLibrary::Check(WeakOdinRoom, "HandleOdinEvent")) {
        return;
    }

    switch (event.tag) {
        case OdinEvent_Joined: {
            auto          own_peer_id = event.joined.own_peer_id;
            TArray<uint8> user_data{event.joined.room_user_data,
                                    static_cast<int>(event.joined.room_user_data_len)};

            FString roomId       = UTF8_TO_TCHAR(event.joined.room_id);
            FString roomCustomer = UTF8_TO_TCHAR(event.joined.customer);
            FString own_user_id  = UTF8_TO_TCHAR(event.joined.own_user_id);

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [roomId, roomCustomer, own_user_id, own_peer_id, user_data, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom, "OdinEvent_Joined")) {
                        return;
                    }

                    if (WeakOdinRoom->joined_callbacks_cs_.TryLock()) {
                        for (auto& callback : WeakOdinRoom->joined_callbacks_) {
                            callback(roomId, roomCustomer, user_data, own_peer_id, own_user_id);
                        }
                        WeakOdinRoom->joined_callbacks_.Reset();
                        WeakOdinRoom->joined_callbacks_cs_.Unlock();
                    }

                    WeakOdinRoom->onRoomJoined.Broadcast(own_peer_id, user_data,
                                                         WeakOdinRoom.Get());
                },
                TStatId(), nullptr, ENamedThreads::GameThread);

        } break;
        case OdinEvent_PeerJoined: {
            auto          peer_id = event.peer_joined.peer_id;
            FString       user_id = UTF8_TO_TCHAR(event.peer_joined.user_id);
            TArray<uint8> user_data{event.peer_joined.peer_user_data,
                                    static_cast<int>(event.peer_joined.peer_user_data_len)};

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [peer_id, user_id, user_data, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom, "OdinEvent_PeerJoined")) {
                        return;
                    }

                    WeakOdinRoom->onPeerJoined.Broadcast(peer_id, user_id, user_data,
                                                         WeakOdinRoom.Get());
                },
                TStatId(), nullptr, ENamedThreads::GameThread);

        } break;
        case OdinEvent_PeerLeft: {
            auto peer_id = event.peer_left.peer_id;

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [peer_id, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom, "OdinEvent_PeerLeft")) {
                        return;
                    }

                    WeakOdinRoom->onPeerLeft.Broadcast(peer_id, WeakOdinRoom.Get());
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEvent_PeerUserDataChanged: {
            auto          peer_id = event.peer_user_data_changed.peer_id;
            TArray<uint8> user_data{
                event.peer_user_data_changed.peer_user_data,
                static_cast<int>(event.peer_user_data_changed.peer_user_data_len)};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [peer_id, user_data, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom,
                                                     "OdinEvent_PeerUserDataChanged")) {
                        return;
                    }

                    WeakOdinRoom->onPeerUserDataChanged.Broadcast(peer_id, user_data,
                                                                  WeakOdinRoom.Get());
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEvent_RoomUserDataChanged: {
            auto          room_data_changed = event.room_user_data_changed;
            TArray<uint8> room_data{room_data_changed.room_user_data,
                                    static_cast<int>(room_data_changed.room_user_data_len)};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [room_data_changed, room_data, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom,
                                                     "OdinEvent_RoomUserDataChanged")) {
                        return;
                    }

                    WeakOdinRoom->onRoomUserDataChanged.Broadcast(room_data, WeakOdinRoom.Get());
                },
                TStatId(), nullptr, ENamedThreads::GameThread);

        } break;
        case OdinEvent_MediaAdded: {
            auto media_handle = event.media_added.media_handle;
            auto peer_id      = event.media_added.peer_id;
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [media_handle, peer_id, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom, "OdinEvent_MediaAdded")) {
                        return;
                    }

                    UOdinRoom* Room = WeakOdinRoom.Get();

                    auto       playback_media = NewObject<UOdinPlaybackMedia>();
                    const auto obj            = UOdinJsonObject::ConstructJsonObject(Room);
                    playback_media->SetMediaHandle(media_handle);
                    playback_media->SetRoom(Room);
                    WeakOdinRoom->medias_.Add(media_handle, playback_media);
                    WeakOdinRoom->onMediaAdded.Broadcast(peer_id, playback_media, obj, Room);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);

        } break;
        case OdinEvent_MediaRemoved: {
            auto media_handle = event.media_removed.media_handle;
            auto peer_id      = event.media_removed.peer_id;

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [media_handle, peer_id, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom, "OdinEvent_MediaRemoved")) {
                        return;
                    }

                    UOdinMediaBase* base_media = nullptr;
                    {
                        FScopeLock lock(&WeakOdinRoom->medias_cs_);
                        if (WeakOdinRoom->medias_.Contains(media_handle)) {
                            if (WeakOdinRoom->medias_.RemoveAndCopyValue(media_handle, base_media)
                                && base_media != nullptr) {
                                auto playback_media = Cast<UOdinPlaybackMedia>(base_media);
                                WeakOdinRoom->onMediaRemoved.Broadcast(peer_id, playback_media,
                                                                       WeakOdinRoom.Get());
                            }
                        }
                    }
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEvent_MediaActiveStateChanged: {
            auto peer_id      = event.media_active_state_changed.peer_id;
            auto media_handle = event.media_active_state_changed.media_handle;
            auto active       = event.media_active_state_changed.active;

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [peer_id, media_handle, active, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom,
                                                     "OdinEvent_MediaActiveStateChanged")) {
                        return;
                    }

                    if (!WeakOdinRoom->medias_.Contains(media_handle)) {
                        return;
                    }
                    TWeakObjectPtr<UOdinMediaBase> media =
                        *WeakOdinRoom->medias_.Find(media_handle);
                    if (media.IsValid()) {
                        WeakOdinRoom->onMediaActiveStateChanged.Broadcast(
                            peer_id, media.Get(), active, WeakOdinRoom.Get());
                    }
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEvent_MessageReceived: {
            auto          peer_id = event.message_received.peer_id;
            TArray<uint8> data{event.message_received.data,
                               static_cast<int>(event.message_received.data_len)};
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [peer_id, data, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom, "OdinEvent_MessageReceived")) {
                        return;
                    }

                    WeakOdinRoom->onMessageReceived.Broadcast(peer_id, data, WeakOdinRoom.Get());
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } break;
        case OdinEvent_RoomConnectionStateChanged: {
            FRoomConnectionStateChangedData Data =
                FRoomConnectionStateChangedData::FromOdinEventData(
                    event.room_connection_state_changed);
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [Data, WeakOdinRoom]() {
                    if (!UOdinFunctionLibrary::Check(WeakOdinRoom,
                                                     "OdinEvent_RoomConnectionStateChanged")) {
                        return;
                    }
                    WeakOdinRoom->LastRoomConnectionStateChangedData = Data;

                    if (WeakOdinRoom->onConnectionStateChanged_DEPRECATED.IsBound()) {
                        WeakOdinRoom->onConnectionStateChanged_DEPRECATED.Broadcast(
                            Data.State, WeakOdinRoom.Get());
                    }

                    if (WeakOdinRoom->onRoomConnectionStateChanged.IsBound()) {
                        WeakOdinRoom->onRoomConnectionStateChanged.Broadcast(Data,
                                                                             WeakOdinRoom.Get());
                    }

                    // Handle connection timeouts and reconnects by creating new input stream
                    if (Data.Reason == EOdinRoomConnectionStateChangeReason::ConnectionLost) {
                        if (Data.State == EOdinRoomConnectionState::Connected) {
                            FScopeLock lock(&WeakOdinRoom->capture_medias_cs_);
                            for (auto media : WeakOdinRoom->capture_medias_) {
                                if (IsValid(media)) {
                                    media->Reconnect();
                                }
                            }
                        }
                    }

                    if (Data.State == EOdinRoomConnectionState::Disconnected) {
                        WeakOdinRoom->ConditionalBeginDestroy();
                    }
                },
                TStatId(), nullptr, ENamedThreads::GameThread);

        } break;

        default:;
    }
}