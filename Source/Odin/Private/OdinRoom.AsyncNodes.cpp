/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinRoom.h"
#include "OdinRoom.AsyncTasks.h"
#include "odin_sdk.h"
#include "Odin.h"
#include "OdinRegistrationSubsystem.h"
#include "Async/AsyncWork.h"
#include "GenericPlatform/GenericPlatform.h"
#include "Async/Async.h"

UOdinRoomJoin* UOdinRoomJoin::JoinRoom(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                       const FString url, const FString token,
                                       const TArray<uint8>& initial_peer_user_data,
                                       FVector initial_position, const FOdinRoomJoinError& onError,
                                       const FOdinRoomJoinSuccess& onSuccess)
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
    return action;
}

void UOdinRoomJoin::Activate()
{
    if (Room && UOdinRegistrationSubsystem::GlobalIsRoomValid(Room->RoomHandle())) {
        FScopeLock lock(&Room->joined_callbacks_cs_);
        Room->joined_callbacks_.Reset();
        const FOdinRoomJoinSuccess OnSuccessCallback = OnSuccess;
        Room->joined_callbacks_.Add(
            [=](auto roomId, auto roomCustomer, auto roomUserData, auto ownPeerId, auto ownUserId) {
                FFunctionGraphTask::CreateAndDispatchWhenReady(
                    [=]() {
                        OnSuccessCallback.ExecuteIfBound(roomId, roomUserData, roomCustomer,
                                                         ownPeerId, ownUserId);
                    },
                    TStatId(), nullptr, ENamedThreads::GameThread);
            });
        UE_LOG(Odin, Verbose,
               TEXT("Starting OdinRoomJoin Task for Room %llu, Url %s, Initial Position: %s"),
               this->Room->RoomHandle(), *this->Url, *this->InitialPosition.ToString());
        (new FAutoDeleteAsyncTask<JoinRoomTask>(this->Room->RoomHandle(), this->Url, this->Token,
                                                this->InitialPeerUserData, this->InitialPosition,
                                                this->OnResponse, this->OnError, this->OnSuccess))
            ->StartBackgroundTask();
    } else {
        UE_LOG(Odin, Error,
               TEXT("Provided invalid UOdinRoom pointer while trying to call "
                    "UOdinRoomJoin::JoinRoom. Please provide a valid UOdinRoom."));
        if (OnError.IsBound())
            OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
        if (OnResponse.IsBound())
            OnResponse.Broadcast(false);
    }
    this->SetReadyToDestroy();
}

UOdinRoomAddMedia* UOdinRoomAddMedia::AddMedia(UObject*                        WorldContextObject,
                                               UPARAM(ref) UOdinRoom*&         room,
                                               UPARAM(ref) UOdinCaptureMedia*& captureMedia,
                                               const FOdinRoomAddMediaError&   onError,
                                               const FOdinRoomAddMediaSuccess& onSuccess)
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
    bool bIsError = false;
    if (!Room.IsValid() || !UOdinRegistrationSubsystem::GlobalIsRoomValid(Room->RoomHandle())) {
        UE_LOG(Odin, Error,
               TEXT("Provided invalid UOdinRoom pointer while trying to call "
                    "UOdinRoomAddMedia::AddMedia. Please provide a valid Odin room "
                    "pointer."));
        bIsError = true;
    }

    if (!CaptureMedia.IsValid()) {
        UE_LOG(Odin, Error,
               TEXT("Provided invalid UOdinCaptureMedia pointer while trying to call "
                    "UOdinRoomAddMedia::AddMedia. Please provide a valid Capture Media."));
        bIsError = true;
    }

    if (!bIsError) {
        const OdinRoomHandle room_handle = Room.IsValid() ? Room->RoomHandle() : 0;
        // Start the add media task in another thread to not block the game thread
        (new FAutoDeleteAsyncTask<AddMediaTask>(room_handle, CaptureMedia, OnResponse, OnError,
                                                OnSuccess))
            ->StartBackgroundTask();
    } else {
        if (OnError.IsBound()) {
            OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
        }
        if (OnResponse.IsBound()) {
            OnResponse.Broadcast(false);
        }
    }

    this->SetReadyToDestroy();
}

UOdinRoomPauseMedia* UOdinRoomPauseMedia::PauseMedia(UObject* WorldContextObject,
                                                     UPARAM(ref) UOdinPlaybackMedia*& playbackMedia,
                                                     const FOdinRoomPauseMediaError&  onError,
                                                     const FOdinRoomPauseMediaSuccess& onSuccess)
{
    auto action           = NewObject<UOdinRoomPauseMedia>();
    action->PlaybackMedia = playbackMedia;
    action->OnError       = onError;
    action->OnSuccess     = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinRoomPauseMedia::Activate()
{
    if (!PlaybackMedia.IsValid()) {
        UE_LOG(Odin, Error,
               TEXT("UOdinRoomPauseMedia Error: Provided PlaybackMedia reference is invalid."));
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = OnResponse]() {
                if (OnError.IsBound())
                    OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
                if (Response.IsBound())
                    Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        // start background thread for pausing media
        (new FAutoDeleteAsyncTask<PauseMediaTask>(PlaybackMedia, OnResponse, OnError, OnSuccess))
            ->StartBackgroundTask();
    }
    this->SetReadyToDestroy();
}

UOdinRoomResumeMedia* UOdinRoomResumeMedia::ResumeMedia(
    UObject* WorldContextObject, UPARAM(ref) UOdinPlaybackMedia*& playbackMedia,
    const FOdinRoomResumeMediaError& onError, const FOdinRoomResumeMediaSuccess& onSuccess)
{
    auto action           = NewObject<UOdinRoomResumeMedia>();
    action->PlaybackMedia = playbackMedia;
    action->OnError       = onError;
    action->OnSuccess     = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinRoomResumeMedia::Activate()
{
    if (!PlaybackMedia.IsValid()) {
        UE_LOG(Odin, Error,
               TEXT("UOdinRoomResumeMedia Error: Provided PlaybackMedia reference is invalid."));
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = OnResponse]() {
                if (OnError.IsBound())
                    OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
                if (Response.IsBound())
                    Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        // start background thread for pausing media
        (new FAutoDeleteAsyncTask<ResumeMediaTask>(PlaybackMedia, OnResponse, OnError, OnSuccess))
            ->StartBackgroundTask();
    }

    this->SetReadyToDestroy();
}

UOdinRoomRemoveMedia* UOdinRoomRemoveMedia::RemoveMedia(
    UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, UOdinCaptureMedia* captureMedia,
    const FOdinRoomRemoveMediaError& onError, const FOdinRoomRemoveMediaSuccess& onSuccess)
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
    if (Room.IsValid() && UOdinRegistrationSubsystem::GlobalIsRoomValid(Room->RoomHandle())) {
        OdinRoomHandle Handle = Room->RoomHandle();
        (new FAutoDeleteAsyncTask<RemoveMediaTask>(Handle, CaptureMedia, OnResponse, OnError,
                                                   OnSuccess))
            ->StartBackgroundTask();
    } else {
        UE_LOG(Odin, Error,
               TEXT("UOdinRoomRemoveMedia Error: The provided Odin Room Handle is invalid."));
        if (OnError.IsBound())
            OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
        if (OnResponse.IsBound())
            OnResponse.Broadcast(false);
    }
    this->SetReadyToDestroy();
}

UOdinRoomUpdatePosition* UOdinRoomUpdatePosition::UpdatePosition(
    UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, FVector position,
    const FOdinRoomUpdatePositionError& onError, const FOdinRoomUpdatePositionSuccess& onSuccess)
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

    if (Room.IsValid() && UOdinRegistrationSubsystem::GlobalIsRoomValid(Room->RoomHandle())) {
        (new FAutoDeleteAsyncTask<UpdatePositionTask>(this->Room->RoomHandle(), this->Position,
                                                      this->OnResponse, this->OnError,
                                                      this->OnSuccess))
            ->StartBackgroundTask();
    } else {
        UE_LOG(Odin, Error,
               TEXT("UOdinRoomUpdatePosition Error: The provided Odin Room Handle is invalid."));
        if (OnError.IsBound())
            OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
        if (OnResponse.IsBound())
            OnResponse.Broadcast(false);
    }
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
    if (Room.IsValid() && UOdinRegistrationSubsystem::GlobalIsRoomValid(Room->RoomHandle())) {
        (new FAutoDeleteAsyncTask<UpdatePeerUserDataTask>(this->Room->room_handle_, this->Data,
                                                          this->OnResponse, this->OnError,
                                                          this->OnSuccess))
            ->StartBackgroundTask();
    } else {
        UE_LOG(Odin, Error,
               TEXT("UOdinRoomUpdatePosition Error: The provided Odin Room Handle is invalid."));
        if (OnError.IsBound())
            OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
        if (OnResponse.IsBound())
            OnResponse.Broadcast(false);
    }
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
    if (Room.IsValid() && UOdinRegistrationSubsystem::GlobalIsRoomValid(Room->RoomHandle())) {
        (new FAutoDeleteAsyncTask<SendMessageTask>(this->Room->room_handle_, this->Targets,
                                                   this->Data, this->OnResponse, this->OnError,
                                                   this->OnSuccess))
            ->StartBackgroundTask();
    } else {
        UE_LOG(Odin, Error,
               TEXT("UOdinRoomUpdatePosition Error: The provided Odin Room Handle is invalid."));
        if (OnError.IsBound())
            OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
        if (OnResponse.IsBound())
            OnResponse.Broadcast(false);
    }
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