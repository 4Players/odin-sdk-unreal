/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinRoom.h"
#include "OdinFunctionLibrary.h"

#include "OdinRoom.AsyncTasks.h"

#include "odin_sdk.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"

UOdinRoomJoin* UOdinRoomJoin::JoinRoom(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                       const FString url, const FString token,
                                       const TArray<uint8>& initial_peer_user_data,
                                       FVector initial_position, const FOdinRoomJoinError& onError,
                                       const FOdinRoomJoinSuccess& onSuccess)
{
    auto action                 = NewObject<UOdinRoomJoin>(WorldContextObject);
    action->Room                = room;
    action->Url                 = url;
    action->Token               = token;
    action->InitialPeerUserData = initial_peer_user_data;
    action->InitialPosition     = initial_position;
    action->OnError             = onError;
    action->OnSuccess           = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);

    FScopeLock lock(&room->joined_callbacks_cs_);
    room->joined_callbacks_.Reset();
    room->joined_callbacks_.Add([=](auto roomId, auto roomCustomer, auto roomUserData,
                                    auto ownPeerId, auto ownUserId) {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [=]() {
                onSuccess.ExecuteIfBound(roomId, roomUserData, roomCustomer, ownPeerId, ownUserId);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    });
    return action;
}

void UOdinRoomJoin::Activate()
{
    (new FAutoDeleteAsyncTask<JoinRoomTask>(this->Room->room_handle_, this->Url, this->Token,
                                            this->InitialPeerUserData, this->InitialPosition,
                                            this->OnResponse, this->OnError, this->OnSuccess))
        ->StartBackgroundTask();
    this->SetReadyToDestroy();
}

UOdinRoomAddMedia* UOdinRoomAddMedia::AddMedia(UObject*                        WorldContextObject,
                                               UPARAM(ref) UOdinRoom*&         room,
                                               UPARAM(ref) UOdinCaptureMedia*& captureMedia,
                                               const FOdinRoomAddMediaError&   onError,
                                               const FOdinRoomAddMediaSuccess& onSuccess)
{
    auto action          = NewObject<UOdinRoomAddMedia>(WorldContextObject);
    action->Room         = room;
    action->CaptureMedia = captureMedia;
    action->OnError      = onError;
    action->OnSuccess    = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinRoomAddMedia::Activate()
{
    TWeakObjectPtr<UOdinRoomAddMedia> WeakThisPtr = this;
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [WeakThisPtr]() {
            if (UOdinRoomAddMedia* This = WeakThisPtr.Get()) {
                if (!(This->Room.IsValid() && This->CaptureMedia.IsValid())) {
                    return;
                }

                OdinRoomHandle room_handle = This->Room.IsValid() ? This->Room->RoomHandle() : 0;
                OdinMediaStreamHandle media_handle =
                    This->CaptureMedia.IsValid() ? This->CaptureMedia->GetMediaHandle() : 0;

                auto result = odin_room_add_media(room_handle, media_handle);

                bool IsError = odin_is_error(result);
                if (IsError) {
                    This->OnError.ExecuteIfBound(result);
                } else {
                    This->Room->BindCaptureMedia(This->CaptureMedia.Get());
                    This->OnSuccess.ExecuteIfBound(result);
                }

                if (This->OnResponse.IsBound()) {
                    This->OnResponse.Broadcast(!IsError);
                }
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinRoomPauseMedia* UOdinRoomPauseMedia::PauseMedia(UObject* WorldContextObject,
                                                     UPARAM(ref) UOdinPlaybackMedia*& playbackMedia,
                                                     const FOdinRoomPauseMediaError&  onError,
                                                     const FOdinRoomPauseMediaSuccess& onSuccess)
{
    auto action           = NewObject<UOdinRoomPauseMedia>(WorldContextObject);
    action->PlaybackMedia = playbackMedia;
    action->OnError       = onError;
    action->OnSuccess     = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinRoomPauseMedia::Activate()
{

    TWeakObjectPtr<UOdinRoomPauseMedia> WeakThis = this;
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [WeakThis]() {
            if (UOdinRoomPauseMedia* This = WeakThis.Get()) {
                if (!This->PlaybackMedia.IsValid()) {
                    return;
                }

                OdinMediaStreamHandle media_handle =
                    This->PlaybackMedia.IsValid() ? This->PlaybackMedia->GetMediaHandle() : 0;

                auto result = odin_media_stream_pause(media_handle);

                bool IsError = odin_is_error(result);
                if (IsError) {
                    This->OnError.ExecuteIfBound(result);

                } else {
                    This->OnSuccess.ExecuteIfBound();
                }

                if (This->OnResponse.IsBound()) {
                    This->OnResponse.Broadcast(!IsError);
                }
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinRoomResumeMedia* UOdinRoomResumeMedia::ResumeMedia(
    UObject* WorldContextObject, UPARAM(ref) UOdinPlaybackMedia*& playbackMedia,
    const FOdinRoomResumeMediaError& onError, const FOdinRoomResumeMediaSuccess& onSuccess)
{
    auto action           = NewObject<UOdinRoomResumeMedia>(WorldContextObject);
    action->PlaybackMedia = playbackMedia;
    action->OnError       = onError;
    action->OnSuccess     = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinRoomResumeMedia::Activate()
{
    TWeakObjectPtr<UOdinRoomResumeMedia> WeakThis = this;
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [WeakThis]() {
            if (UOdinRoomResumeMedia* This = WeakThis.Get()) {
                if (!This->PlaybackMedia.IsValid()) {
                    return;
                }

                OdinMediaStreamHandle media_handle =
                    This->PlaybackMedia.IsValid() ? This->PlaybackMedia->GetMediaHandle() : 0;

                auto result = odin_media_stream_resume(media_handle);

                bool IsError = odin_is_error(result);
                if (IsError) {
                    This->OnError.ExecuteIfBound(result);

                } else {
                    This->OnSuccess.ExecuteIfBound();
                }
                if (This->OnResponse.IsBound()) {
                    This->OnResponse.Broadcast(!IsError);
                }
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinRoomRemoveMedia* UOdinRoomRemoveMedia::RemoveMedia(
    UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, UOdinCaptureMedia* captureMedia,
    const FOdinRoomRemoveMediaError& onError, const FOdinRoomRemoveMediaSuccess& onSuccess)
{
    auto action          = NewObject<UOdinRoomRemoveMedia>(WorldContextObject);
    action->Room         = room;
    action->CaptureMedia = captureMedia;
    action->OnError      = onError;
    action->OnSuccess    = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinRoomRemoveMedia::Activate()
{
    TWeakObjectPtr<UOdinRoomRemoveMedia> WeakThis = this;
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [WeakThis]() {
            if (UOdinRoomRemoveMedia* This = WeakThis.Get()) {
                OdinReturnCode result = 1 << 30;
                if (!This->Room.IsValid()) {
                    UE_LOG(Odin, Error, TEXT("OdinRoomRemoveMedia: The Room reference is invalid."))
                }

                if (!This->CaptureMedia.IsValid()) {
                    UE_LOG(Odin, Error,
                           TEXT("OdinRoomRemoveMedia: The CaptureMedia reference is invalid."))
                }

                if (This->Room.IsValid() && This->CaptureMedia.IsValid()) {
                    This->Room->UnbindCaptureMedia(This->CaptureMedia.Get());
                    result = This->CaptureMedia->ResetOdinStream();
                }
                bool IsError = odin_is_error(result);
                if (IsError) {
                    This->OnError.ExecuteIfBound(result);
                } else {
                    This->OnSuccess.ExecuteIfBound();
                }
                if (This->OnResponse.IsBound()) {
                    This->OnResponse.Broadcast(!IsError);
                }
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
    this->SetReadyToDestroy();
}

UOdinRoomUpdatePosition* UOdinRoomUpdatePosition::UpdatePosition(
    UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, FVector position,
    const FOdinRoomUpdatePositionError& onError, const FOdinRoomUpdatePositionSuccess& onSuccess)
{
    auto action       = NewObject<UOdinRoomUpdatePosition>(WorldContextObject);
    action->Room      = room;
    action->Position  = position;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);

    return action;
}

void UOdinRoomUpdatePosition::Activate()
{
    (new FAutoDeleteAsyncTask<UpdatePositionTask>(this->Room->room_handle_, this->Position,
                                                  this->OnResponse, this->OnError, this->OnSuccess))
        ->StartBackgroundTask();
    this->SetReadyToDestroy();
}

/// <summary>
/// UOdinRoomUpdatePeerUserData
/// </summary>
/// <param name="WorldContextObject"></param>
/// <param name="room"></param>
/// <returns></returns>
UOdinRoomUpdatePeerUserData* UOdinRoomUpdatePeerUserData::UpdatePeerUserData(
    UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, const TArray<uint8>& data,
    const FOdinRoomUpdatePeerUserDataError&   onError,
    const FOdinRoomUpdatePeerUserDataSuccess& onSuccess)
{
    auto action       = NewObject<UOdinRoomUpdatePeerUserData>(WorldContextObject);
    action->Room      = room;
    action->Data      = data;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);

    return action;
}

void UOdinRoomUpdatePeerUserData::Activate()
{
    (new FAutoDeleteAsyncTask<UpdatePeerUserDataTask>(
         this->Room->room_handle_, this->Data, this->OnResponse, this->OnError, this->OnSuccess))
        ->StartBackgroundTask();
    this->SetReadyToDestroy();
}

/// <summary>
/// UOdinRoomSendMessage
/// </summary>
/// <param name="WorldContextObject"></param>
/// <param name="room"></param>
/// <returns></returns>
UOdinRoomSendMessage*
UOdinRoomSendMessage::SendMessage(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                  const TArray<int64>& targets, const TArray<uint8>& data,
                                  const FOdinRoomSendMessageError&   onError,
                                  const FOdinRoomSendMessageSuccess& onSuccess)
{
    auto action       = NewObject<UOdinRoomSendMessage>(WorldContextObject);
    action->Room      = room;
    action->Data      = data;
    action->Targets   = targets;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);

    return action;
}

void UOdinRoomSendMessage::Activate()
{
    (new FAutoDeleteAsyncTask<SendMessageTask>(this->Room->room_handle_, this->Targets, this->Data,
                                               this->OnResponse, this->OnError, this->OnSuccess))
        ->StartBackgroundTask();
    this->SetReadyToDestroy();
}

FRoomConnectionStateChangedData
FRoomConnectionStateChangedData::FromOdinEventData(OdinEvent_RoomConnectionStateChangedData data)
{
    FRoomConnectionStateChangedData UnrealData;
    switch (data.reason) {
        case OdinRoomConnectionStateChangeReason_ClientRequested:
            UnrealData.Reason = EOdinRoomConnectionStateChangeReason::ClientRequested;
            break;
        case OdinRoomConnectionStateChangeReason_ConnectionLost:
            UnrealData.Reason = EOdinRoomConnectionStateChangeReason::ConnectionLost;
            break;
        case OdinRoomConnectionStateChangeReason_ServerRequested:
            UnrealData.Reason = EOdinRoomConnectionStateChangeReason::ServerRequested;
            break;
        default:
            UnrealData.Reason = EOdinRoomConnectionStateChangeReason::ClientRequested;
            break;
    }
    switch (data.state) {
        case OdinRoomConnectionState_Connected:
            UnrealData.State = EOdinRoomConnectionState::Connected;
            break;
        case OdinRoomConnectionState_Connecting:
            UnrealData.State = EOdinRoomConnectionState::Connecting;
            break;
        case OdinRoomConnectionState_Disconnecting:
            UnrealData.State = EOdinRoomConnectionState::Disconnecting;
            break;
        case OdinRoomConnectionState_Disconnected:
            UnrealData.State = EOdinRoomConnectionState::Disconnected;
            break;
        default:
            UnrealData.State = EOdinRoomConnectionState::Disconnected;
            break;
    }

    return UnrealData;
}