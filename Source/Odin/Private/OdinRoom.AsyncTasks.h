/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"

// #include "OdinRoom.AsyncTasks.generated.h"
#include "OdinRoom.h"

#include <functional>

class JoinRoomTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<JoinRoomTask>;

    OdinRoomHandle RoomHandle;
    FString        Url;
    FString        RoomToken;
    TArray<uint8>  InitialPeerUserData;
    FVector2D      InitialPosition;

    FJoinRoomResponsePin Response;
    FOdinRoomJoinError   OnError;
    FOdinRoomJoinSuccess OnSuccess;

    JoinRoomTask(OdinRoomHandle roomHandle, FString url, FString room_token,
                 TArray<uint8> initial_peer_user_data, FVector2D initial_position,
                 FJoinRoomResponsePin response, FOdinRoomJoinError onError,
                 FOdinRoomJoinSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Url(url)
        , RoomToken(room_token)
        , InitialPeerUserData(initial_peer_user_data)
        , InitialPosition(initial_position)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork()
    {
        auto update_user_data_result =
            odin_room_update_user_data(RoomHandle, OdinUserDataTarget::OdinUserDataTarget_Peer,
                                       InitialPeerUserData.GetData(), InitialPeerUserData.Num());
        if (odin_is_error(update_user_data_result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, update_user_data_result]() {
                    OnError.ExecuteIfBound(update_user_data_result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
            return;
        }

        auto update_position_result =
            odin_room_update_position(RoomHandle, InitialPosition.X, InitialPosition.Y);
        if (odin_is_error(update_user_data_result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, update_position_result]() {
                    OnError.ExecuteIfBound(update_position_result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
            return;
        }

        auto join_room_result =
            odin_room_join(RoomHandle, TCHAR_TO_UTF8(*Url), TCHAR_TO_UTF8(*RoomToken));

        if (odin_is_error(join_room_result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, join_room_result]() {
                    OnError.ExecuteIfBound(join_room_result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } else {
            // OnSuccess is handled in UOdinRoom::HandleEvent
            // See also, UOdinRoomJoin in OdinRoom.AsyncNodes.cpp
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnSuccess = OnSuccess, Response = Response]() { Response.Broadcast(true); },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(JoinRoomTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class AddMediaTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<AddMediaTask>;

    UOdinCaptureMedia *Media;
    UOdinRoom         *Room;

    FAddMediaResponsePin     Response;
    FOdinRoomAddMediaError   OnError;
    FOdinRoomAddMediaSuccess OnSuccess;

    AddMediaTask(UOdinRoom *room, UOdinCaptureMedia *media, FAddMediaResponsePin response,
                 FOdinRoomAddMediaError onError, FOdinRoomAddMediaSuccess onSuccess)
        : Media(media)
        , Room(room)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork()
    {
        OdinRoomHandle room_handle = Room ? Room->RoomHandle() : 0;
        OdinMediaStreamHandle media_handle = Media ? Media->GetMediaHandle() : 0;
        
        auto result = odin_room_add_media(room_handle, media_handle);

        if (odin_is_error(result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, result]() {
                    OnError.ExecuteIfBound(result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } else {
            Room->BindCaptureMedia(Media);

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnSuccess = OnSuccess, Response = Response, result]() {
                    OnSuccess.ExecuteIfBound(result);
                    Response.Broadcast(true);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(AddMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class RemoveMediaTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<RemoveMediaTask>;

    UOdinCaptureMedia *Media;
    UOdinRoom         *Room;

    FRemoveMediaResponsePin     Response;
    FOdinRoomRemoveMediaError   OnError;
    FOdinRoomRemoveMediaSuccess OnSuccess;

    RemoveMediaTask(UOdinRoom *room, UOdinCaptureMedia *media, FRemoveMediaResponsePin response,
                    FOdinRoomRemoveMediaError onError, FOdinRoomRemoveMediaSuccess onSuccess)
        : Media(media)
        , Room(room)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork()
    {
        //TODO(Nico) refactor error check this tmp to prevent audio_generator errors
        OdinReturnCode result = -1;
        if (Room && Media) {
            Room->UnbindCaptureMedia(Media);
            result = Media->ResetOdinStream();
        } else {
            UE_LOG(Odin, Error, TEXT("RemoveMediaTask invalid pins for room/media input."));
        }

        if (odin_is_error(result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, result]() {
                    OnError.ExecuteIfBound(result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } else {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnSuccess = OnSuccess, Response = Response, result]() {
                    OnSuccess.ExecuteIfBound();
                    Response.Broadcast(true);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(AddMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class UpdatePositionTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<UpdatePositionTask>;

    OdinRoomHandle RoomHandle;

    FVector2D Position;

    FUpdatePositionResponsePin     Response;
    FOdinRoomUpdatePositionError   OnError;
    FOdinRoomUpdatePositionSuccess OnSuccess;

    UpdatePositionTask(OdinRoomHandle roomHandle, FVector2D position,
                       FUpdatePositionResponsePin response, FOdinRoomUpdatePositionError onError,
                       FOdinRoomUpdatePositionSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Position(position)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork()
    {
        auto result = odin_room_update_position(RoomHandle, Position.X, Position.Y);

        if (odin_is_error(result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, result]() {
                    OnError.ExecuteIfBound(result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);

        } else {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnSuccess = OnSuccess, Response = Response]() {
                    OnSuccess.ExecuteIfBound();
                    Response.Broadcast(true);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UpdatePositionTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class UpdateScalingTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<UpdateScalingTask>;

    OdinRoomHandle RoomHandle;

    float Scale;

    UpdateScalingTask(OdinRoomHandle roomHandle, float scale)
        : RoomHandle(roomHandle)
        , Scale(scale)
    {
    }

    void DoWork()
    {
        auto result = odin_room_set_position_scale(RoomHandle, Scale);
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UpdateScalingTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class UpdatePeerUserDataTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<UpdatePeerUserDataTask>;

    OdinRoomHandle RoomHandle;

    TArray<uint8> Data;

    FUpdatePeerUserDataResponsePin     Response;
    FOdinRoomUpdatePeerUserDataError   OnError;
    FOdinRoomUpdatePeerUserDataSuccess OnSuccess;

    UpdatePeerUserDataTask(OdinRoomHandle roomHandle, TArray<uint8> data,
                           FUpdatePeerUserDataResponsePin     response,
                           FOdinRoomUpdatePeerUserDataError   onError,
                           FOdinRoomUpdatePeerUserDataSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Data(data)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork()
    {
        auto result = odin_room_update_user_data(
            RoomHandle, OdinUserDataTarget::OdinUserDataTarget_Peer, Data.GetData(), Data.Num());

        if (odin_is_error(result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, result]() {
                    OnError.ExecuteIfBound(result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } else {

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnSuccess = OnSuccess, Response = Response]() {
                    OnSuccess.ExecuteIfBound();
                    Response.Broadcast(true);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UpdatePeerUserDataTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class UpdateRoomUserDataTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<UpdateRoomUserDataTask>;

    OdinRoomHandle RoomHandle;

    TArray<uint8> Data;

    FUpdateRoomUserDataResponsePin     Response;
    FOdinRoomUpdateRoomUserDataError   OnError;
    FOdinRoomUpdateRoomUserDataSuccess OnSuccess;

    UpdateRoomUserDataTask(OdinRoomHandle roomHandle, TArray<uint8> data,
                           FUpdateRoomUserDataResponsePin     response,
                           FOdinRoomUpdateRoomUserDataError   onError,
                           FOdinRoomUpdateRoomUserDataSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Data(data)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork()
    {
        auto result = odin_room_update_user_data(
            RoomHandle, OdinUserDataTarget::OdinUserDataTarget_Room, Data.GetData(), Data.Num());

        if (odin_is_error(result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, result]() {
                    OnError.ExecuteIfBound(result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } else {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnSuccess = OnSuccess, Response = Response]() {
                    OnSuccess.ExecuteIfBound();
                    Response.Broadcast(true);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UpdateRoomUserDataTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class SendMessageTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<SendMessageTask>;

    OdinRoomHandle RoomHandle;

    TArray<int64> Targets;
    TArray<uint8> Data;

    FSendMessageResponsePin     Response;
    FOdinRoomSendMessageError   OnError;
    FOdinRoomSendMessageSuccess OnSuccess;

    SendMessageTask(OdinRoomHandle roomHandle, TArray<int64> targets, TArray<uint8> data,
                    FSendMessageResponsePin response, FOdinRoomSendMessageError onError,
                    FOdinRoomSendMessageSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Targets(targets)
        , Data(data)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork()
    {
        auto result = odin_room_send_message(RoomHandle, (const uint64_t *)Targets.GetData(),
                                             Targets.Num(), Data.GetData(), Data.Num());

        if (odin_is_error(result)) {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnError = OnError, Response = Response, result]() {
                    OnError.ExecuteIfBound(result);
                    Response.Broadcast(false);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        } else {
            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [OnSuccess = OnSuccess, Response = Response]() {
                    OnSuccess.ExecuteIfBound();
                    Response.Broadcast(true);
                },
                TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(SendMessageTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class DestroyRoomTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<DestroyRoomTask>;

    OdinRoomHandle roomHandle;

    DestroyRoomTask(OdinRoomHandle roomHandle)
        : roomHandle(roomHandle)
    {
    }

    void DoWork()
    {
        odin_room_close(roomHandle);
        odin_room_set_event_callback(roomHandle, nullptr, nullptr);
        odin_room_destroy(roomHandle);
        roomHandle = 0;
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(DestroyRoomTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};