/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinRoom.h"

#include "OdinRoom.AsyncTasks.h"

#include "OdinCore/include/odin.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"

UOdinRoomJoin *UOdinRoomJoin::JoinRoom(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room,
                                       const FString url, const FString token,
                                       const TArray<uint8>        &initial_peer_user_data,
                                       FVector2D                   initial_position,
                                       const FOdinRoomJoinError   &onError,
                                       const FOdinRoomJoinSuccess &onSuccess)
{
    auto action                 = NewObject<UOdinRoomJoin>();
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

UOdinRoomAddMedia *UOdinRoomAddMedia::AddMedia(UObject                        *WorldContextObject,
                                               UPARAM(ref) UOdinRoom         *&room,
                                               UPARAM(ref) UOdinCaptureMedia *&captureMedia,
                                               const FOdinRoomAddMediaError   &onError,
                                               const FOdinRoomAddMediaSuccess &onSuccess)
{
    auto action          = NewObject<UOdinRoomAddMedia>();
    action->Room         = room;
    action->CaptureMedia = captureMedia;
    action->OnError      = onError;
    action->OnSuccess    = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinRoomAddMedia::Activate()
{
    // (new FAutoDeleteAsyncTask<AddMediaTask>(this->Room.Get(), this->CaptureMedia.Get(),
    // this->OnResponse,
    //                                         this->OnError, this->OnSuccess))
    //     ->StartBackgroundTask();
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [=]() {
            if (!(Room.IsValid() && CaptureMedia.IsValid()))
                return;

            OdinRoomHandle        room_handle = Room.IsValid() ? Room->RoomHandle() : 0;
            OdinMediaStreamHandle media_handle =
                CaptureMedia.IsValid() ? CaptureMedia->GetMediaHandle() : 0;

            auto result = odin_room_add_media(room_handle, media_handle);

            if (odin_is_error(result)) {

                OnError.ExecuteIfBound(result);
                OnResponse.Broadcast(false);

            } else {
                Room->BindCaptureMedia(CaptureMedia.Get());

                OnSuccess.ExecuteIfBound(result);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinRoomRemoveMedia *UOdinRoomRemoveMedia::RemoveMedia(
    UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, UOdinCaptureMedia *captureMedia,
    const FOdinRoomRemoveMediaError &onError, const FOdinRoomRemoveMediaSuccess &onSuccess)
{
    auto action          = NewObject<UOdinRoomRemoveMedia>();
    action->Room         = room;
    action->CaptureMedia = captureMedia;
    action->OnError      = onError;
    action->OnSuccess    = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinRoomRemoveMedia::Activate()
{
    // (new FAutoDeleteAsyncTask<RemoveMediaTask>(this->Room.Get(), this->CaptureMedia.Get(),
    // this->OnResponse,
    //                                            this->OnError, this->OnSuccess))
    //     ->StartBackgroundTask();

    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [=]() {
            OdinReturnCode result = -1;
            if (Room.IsValid() && CaptureMedia.IsValid()) {
                Room->UnbindCaptureMedia(CaptureMedia.Get());
                result = CaptureMedia->ResetOdinStream();
            }

            if (odin_is_error(result)) {
                OnError.ExecuteIfBound(result);
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound();
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
    this->SetReadyToDestroy();
}

UOdinRoomUpdatePosition *UOdinRoomUpdatePosition::UpdatePosition(
    UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, FVector2D position,
    const FOdinRoomUpdatePositionError &onError, const FOdinRoomUpdatePositionSuccess &onSuccess)
{
    auto action       = NewObject<UOdinRoomUpdatePosition>();
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
UOdinRoomUpdatePeerUserData *UOdinRoomUpdatePeerUserData::UpdatePeerUserData(
    UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, const TArray<uint8> &data,
    const FOdinRoomUpdatePeerUserDataError   &onError,
    const FOdinRoomUpdatePeerUserDataSuccess &onSuccess)
{
    auto action       = NewObject<UOdinRoomUpdatePeerUserData>();
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
/// UOdinRoomUpdatePeerUserData
/// </summary>
/// <param name="WorldContextObject"></param>
/// <param name="room"></param>
/// <returns></returns>
UOdinRoomUpdateRoomUserData *UOdinRoomUpdateRoomUserData::UpdateRoomUserData(
    UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, const TArray<uint8> &data,
    const FOdinRoomUpdateRoomUserDataError   &onError,
    const FOdinRoomUpdateRoomUserDataSuccess &onSuccess)
{
    auto action       = NewObject<UOdinRoomUpdateRoomUserData>();
    action->Room      = room;
    action->Data      = data;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);

    return action;
}

void UOdinRoomUpdateRoomUserData::Activate()
{
    (new FAutoDeleteAsyncTask<UpdateRoomUserDataTask>(
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
UOdinRoomSendMessage *
UOdinRoomSendMessage::SendMessage(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room,
                                  const TArray<int64> &targets, const TArray<uint8> &data,
                                  const FOdinRoomSendMessageError   &onError,
                                  const FOdinRoomSendMessageSuccess &onSuccess)
{
    auto action       = NewObject<UOdinRoomSendMessage>();
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
