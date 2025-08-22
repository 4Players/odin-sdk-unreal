/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinRoom.AsyncTasks.h"

#include "Odin.h"
#include "OdinRegistrationSubsystem.h"
#include "GenericPlatform/GenericPlatform.h"
#include "Async/Async.h"

void JoinRoomTask::DoWork()
{
    // Make sure the room handle is registered
    if (!UOdinRegistrationSubsystem::GlobalIsRoomValid(RoomHandle)) {
        UE_LOG(Odin, Error,
               TEXT("JoinRoomTask::DoWork: Trying to join room by handle that is not registered. "
                    "Aborting join room."));
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response]() {
                if (OnError.IsBound()) {
                    OnError.Execute(ODIN_GENERAL_CLIENT_ERROR_CODE);
                }
                if (Response.IsBound()) {
                    Response.Broadcast(false);
                }
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
        return;
    }

    // Update the peer's initial user data
    auto update_user_data_result = odin_room_update_peer_user_data(
        RoomHandle, InitialPeerUserData.GetData(), InitialPeerUserData.Num());
    if (odin_is_error(update_user_data_result)) {
        FOdinModule::LogErrorCode(TEXT("Call to odin_room_update_peer_user_data returned error: "),
                                  update_user_data_result);
    }
    // Update the peer's initial room position
    auto update_position_result = odin_room_update_position(RoomHandle, InitialPosition.X,
                                                            InitialPosition.Y, InitialPosition.Z);
    if (odin_is_error(update_position_result)) {
        FOdinModule::LogErrorCode(TEXT("Call to odin_room_update_position returned error: "),
                                  update_position_result);
    }

    // Join the room
    auto join_room_result = odin_room_join(RoomHandle, StringCast<ANSICHAR>(*Url).Get(),
                                           StringCast<ANSICHAR>(*RoomToken).Get());

    if (odin_is_error(join_room_result)) {
        FOdinModule::LogErrorCode(TEXT("Call to odin_room_join returned error: "),
                                  join_room_result);

        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, join_room_result]() {
                if (OnError.IsBound()) {
                    OnError.Execute(join_room_result);
                }
                if (Response.IsBound()) {
                    Response.Broadcast(false);
                }
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {

        // OnSuccess is handled in UOdinRoom::HandleEvent
        // See also, UOdinRoomJoin in OdinRoom.AsyncNodes.cpp
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response]() {
                if (Response.IsBound()) {
                    Response.Broadcast(true);
                }
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void AddMediaTask::DoWork()
{
    OdinMediaStreamHandle MediaHandle = CaptureMedia.IsValid() ? CaptureMedia->GetMediaHandle() : 0;
    // call to native lib. Can take quite a while, that's why we're doing it on a background task
    OdinReturnCode Result = odin_room_add_media(RoomHandle, MediaHandle);

    // Sync to the Game Thread to
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [RoomHandle = RoomHandle, CaptureMedia = CaptureMedia, Response = Response,
         OnError = OnError, OnSuccess = OnSuccess, Result]() {
            const bool bIsOdinError =
                odin_is_error(Result) || !UOdinRegistrationSubsystem::GlobalIsRoomValid(RoomHandle);
            UOdinRegistrationSubsystem* OdinSubsystem = UOdinRegistrationSubsystem::Get();
            if (!bIsOdinError) {
                TWeakObjectPtr<UOdinRoom> OdinRoom = OdinSubsystem->GetRoomByHandle(RoomHandle);
                if (OdinRoom.IsValid() && CaptureMedia.IsValid()) {
                    OdinRoom->BindCaptureMedia(CaptureMedia.Get());
                }
            }

            if (bIsOdinError) {
                FOdinModule::LogErrorCode(TEXT("AddMediaTask Error"), Result);
                if (OnError.IsBound()) {
                    OnError.Execute(Result);
                }
            } else {
                if (OnSuccess.IsBound()) {
                    OnSuccess.Execute(Result);
                }
            }
            if (Response.IsBound()) {
                Response.Broadcast(!bIsOdinError);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

void RemoveMediaTask::DoWork()
{
    // TODO: refactor to prevent potential sound generator errors
    OdinReturnCode              result        = ODIN_GENERAL_CLIENT_ERROR_CODE;
    UOdinRegistrationSubsystem* OdinSubsystem = UOdinRegistrationSubsystem::Get();
    TWeakObjectPtr<UOdinRoom>   Room          = nullptr;
    if (OdinSubsystem) {
        Room = OdinSubsystem->GetRoomByHandle(RoomHandle);
    }

    if (Room.IsValid() && Media.IsValid()) {
        Room->UnbindCaptureMedia(Media.Get());
        result = Media->ResetOdinStream();
    }

    if (odin_is_error(result)) {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, result]() {
                if (OnError.IsBound())
                    OnError.Execute(result);
                if (Response.IsBound())
                    Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response]() {
                if (OnSuccess.IsBound())
                    OnSuccess.Execute();
                if (Response.IsBound())
                    Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void PauseMediaTask::DoWork()
{
    if (!Media.IsValid())
        return;

    OdinMediaStreamHandle MediaHandle = Media.IsValid() ? Media->GetMediaHandle() : 0;

    auto result = odin_media_stream_pause(MediaHandle);

    if (odin_is_error(result)) {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, result]() {
                if (OnError.IsBound())
                    OnError.Execute(result);
                if (Response.IsBound())
                    Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response]() {
                if (OnSuccess.IsBound())
                    OnSuccess.Execute();
                if (Response.IsBound())
                    Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void ResumeMediaTask::DoWork()
{
    if (!Media.IsValid())
        return;

    OdinMediaStreamHandle media_handle = Media.IsValid() ? Media->GetMediaHandle() : 0;

    auto result = odin_media_stream_resume(media_handle);

    if (odin_is_error(result)) {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, result]() {
                if (OnError.IsBound())
                    OnError.Execute(result);
                if (Response.IsBound())
                    Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response]() {
                if (OnSuccess.IsBound())
                    OnSuccess.Execute();
                if (Response.IsBound())
                    Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void UpdatePositionTask::DoWork()
{
    auto result = odin_room_update_position(RoomHandle, Position.X, Position.Y, Position.Z);

    if (odin_is_error(result)) {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, result]() {
                if (OnError.IsBound())
                    OnError.Execute(result);
                if (Response.IsBound())
                    Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response]() {
                if (OnSuccess.IsBound())
                    OnSuccess.Execute();
                if (Response.IsBound())
                    Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void UpdateScalingTask::DoWork()
{
    OdinReturnCode Result = ODIN_GENERAL_CLIENT_ERROR_CODE;
    if (UOdinRegistrationSubsystem::GlobalIsRoomValid(RoomHandle)) {
        Result = odin_room_set_position_scale(RoomHandle, Scale);
    }

    if (odin_is_error(Result)) {
        FOdinModule::LogErrorCode(TEXT("UpdateScaling Task Error"), Result);
    }
}

void UpdatePeerUserDataTask::DoWork()
{
    OdinReturnCode Result = ODIN_GENERAL_CLIENT_ERROR_CODE;
    if (UOdinRegistrationSubsystem::GlobalIsRoomValid(RoomHandle)) {
        Result = odin_room_update_peer_user_data(RoomHandle, Data.GetData(), Data.Num());
    }

    if (odin_is_error(Result)) {
        FOdinModule::LogErrorCode(TEXT("Update Peer User Data Task Error"), Result);
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, Result]() {
                if (OnError.IsBound())
                    OnError.Execute(Result);
                if (Response.IsBound())
                    Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response]() {
                if (OnSuccess.IsBound())
                    OnSuccess.Execute();
                if (Response.IsBound())
                    Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void SendMessageTask::DoWork()
{
    OdinReturnCode Result = ODIN_GENERAL_CLIENT_ERROR_CODE;
    if (UOdinRegistrationSubsystem::GlobalIsRoomValid(RoomHandle)) {
        // Todo: Check if this cast here is dangerous or not
        Result = odin_room_send_message(RoomHandle, (const uint64_t*)Targets.GetData(),
                                        Targets.Num(), Data.GetData(), Data.Num());
    }

    if (odin_is_error(Result)) {
        FOdinModule::LogErrorCode(TEXT("Send Message Task Error"), Result);
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, Result]() {
                if (OnError.IsBound())
                    OnError.Execute(Result);
                if (Response.IsBound()) {
                    Response.Broadcast(false);
                }
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response]() {
                if (OnSuccess.IsBound())
                    OnSuccess.Execute();
                if (Response.IsBound()) {
                    Response.Broadcast(true);
                }
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void DestroyRoomTask::DoWork()
{
    if (roomHandle > 0) {
        UE_LOG(Odin, Verbose, TEXT("DestroyRoomTask::DoWork(): Closing Odin Room with Handle %lld"),
               roomHandle);
        OdinReturnCode ReturnCode = odin_room_destroy(roomHandle);
        if (odin_is_error(ReturnCode)) {
            FOdinModule::LogReturnCode(
                TEXT("Destroy Room Task: odin_room_destroy was not successful."), ReturnCode);
        } else {
            UE_LOG(Odin, Verbose,
                   TEXT("DestroyRoomTask::DoWork(): Successfully closed room with Handle %lld"),
                   roomHandle);
        }
        ReturnCode = odin_room_set_event_callback(roomHandle, nullptr, nullptr);
        if (odin_is_error(ReturnCode)) {
            FOdinModule::LogReturnCode(
                TEXT("Destroy Room Task: odin_room_set_event_callback was not successful."),
                ReturnCode);
        } else {
            UE_LOG(Odin, Verbose,
                   TEXT("DestroyRoomTask::DoWork(): Successfully unset event callbacks for room "
                        "with Handle %lld"),
                   roomHandle);
        }
        if (UOdinRegistrationSubsystem* OdinSubsystem = UOdinRegistrationSubsystem::Get()) {
            OdinSubsystem->DeregisterRoom(roomHandle);
        }
    } else {
        UE_LOG(Odin, Log,
               TEXT("DestroyRoomTask::DoWork(): Aborted closing room, room handle is already "
                    "invalid."));
    }
}
