#include "OdinRoom.h"

#include "OdinRoom.AsyncTasks.h"

#include "OdinLibrary/include/odin.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"

UOdinRoomJoin *UOdinRoomJoin::JoinRoom(UObject *WorldContextObject, UOdinRoom *room,
                                       const FString url, const FString token,
                                       const TArray<uint8> &user_data, FOdinRoomJoinError onError,
                                       const FOdinRoomJoinSuccess &onSuccess)
{
    auto action       = NewObject<UOdinRoomJoin>();
    action->Room      = room;
    action->Url       = url;
    action->Token     = token;
    action->UserData  = user_data;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);

    FScopeLock lock(&room->joined_callbacks_cs_);
    room->joined_callbacks_.Reset();
    room->joined_callbacks_.Add(
        [=](auto roomId, auto roomCustomer, auto roomUserData, auto ownPeerId) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [=]() { onSuccess.ExecuteIfBound(roomId, roomUserData, roomCustomer, ownPeerId); },
                TStatId(), nullptr, ENamedThreads::GameThread);
        });
    return action;
}

void UOdinRoomJoin::Activate()
{
    (new FAutoDeleteAsyncTask<JoinRoomTask>(this->Room->room_handle_, this->Url, this->Token,
                                            this->UserData, this->OnResponse, this->OnError,
                                            this->OnSuccess))
        ->StartBackgroundTask();
}

UOdinRoomAddMedia *UOdinRoomAddMedia::AddMedia(UObject *WorldContextObject, UOdinRoom *room,
                                               UOdinCaptureMedia              *captureMedia,
                                               FOdinRoomAddMediaError          onError,
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
    (new FAutoDeleteAsyncTask<AddMediaTask>(this->Room->room_handle_, this->CaptureMedia,
                                            this->OnResponse, this->OnError, this->OnSuccess))
        ->StartBackgroundTask();
}

UOdinRoomUpdatePosition *
UOdinRoomUpdatePosition::UpdatePosition(UObject *WorldContextObject, UOdinRoom *room,
                                        FVector2D position, FOdinRoomUpdatePositionError onError,
                                        const FOdinRoomUpdatePositionSuccess &onSuccess)
{
    auto action       = NewObject<UOdinRoomUpdatePosition>();
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
}

/// <summary>
/// UOdinRoomUpdatePeerUserData
/// </summary>
/// <param name="WorldContextObject"></param>
/// <param name="room"></param>
/// <returns></returns>
UOdinRoomUpdatePeerUserData *UOdinRoomUpdatePeerUserData::UpdatePeerUserData(
    UObject *WorldContextObject, UOdinRoom *room, const TArray<uint8> &data,
    FOdinRoomUpdatePeerUserDataError onError, const FOdinRoomUpdatePeerUserDataSuccess &onSuccess)
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
}

/// <summary>
/// UOdinRoomUpdatePeerUserData
/// </summary>
/// <param name="WorldContextObject"></param>
/// <param name="room"></param>
/// <returns></returns>
UOdinRoomUpdateRoomUserData *UOdinRoomUpdateRoomUserData::UpdateRoomUserData(
    UObject *WorldContextObject, UOdinRoom *room, const TArray<uint8> &data,
    FOdinRoomUpdateRoomUserDataError onError, const FOdinRoomUpdateRoomUserDataSuccess &onSuccess)
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
}

/// <summary>
/// UOdinRoomSendMessage
/// </summary>
/// <param name="WorldContextObject"></param>
/// <param name="room"></param>
/// <returns></returns>
UOdinRoomSendMessage *
UOdinRoomSendMessage::SendMessage(UObject *WorldContextObject, UOdinRoom *room,
                                  const TArray<int64> &targets, const TArray<uint8> &data,
                                  FOdinRoomSendMessageError          onError,
                                  const FOdinRoomSendMessageSuccess &onSuccess)
{
    auto action       = NewObject<UOdinRoomSendMessage>();
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
}