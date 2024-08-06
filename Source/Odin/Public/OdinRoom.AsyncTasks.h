/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "odin_sdk.h"
#include "Async/AsyncWork.h"
#include "OdinRoom.h"

#include <functional>

class ODIN_API JoinRoomTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<JoinRoomTask>;

    OdinRoomHandle RoomHandle;
    FString        Url;
    FString        RoomToken;
    TArray<uint8>  InitialPeerUserData;
    FVector        InitialPosition;

    FJoinRoomResponsePin Response;
    FOdinRoomJoinError   OnError;
    FOdinRoomJoinSuccess OnSuccess;

    JoinRoomTask(OdinRoomHandle roomHandle, FString url, FString room_token,
                 TArray<uint8> initial_peer_user_data, FVector initial_position,
                 FJoinRoomResponsePin response, FOdinRoomJoinError onError,
                 FOdinRoomJoinSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Url(url)
        , RoomToken(room_token)
        , InitialPeerUserData(initial_peer_user_data)
        , InitialPosition(initial_position)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(JoinRoomTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API AddMediaTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<AddMediaTask>;

    FAddMediaResponsePin              Response;
    FOdinRoomAddMediaError            OnError;
    FOdinRoomAddMediaSuccess          OnSuccess;
    OdinRoomHandle                    RoomHandle;
    TWeakObjectPtr<UOdinCaptureMedia> CaptureMedia;

    AddMediaTask(OdinRoomHandle RoomHandle, TWeakObjectPtr<UOdinCaptureMedia> CaptureMedia,
                 FAddMediaResponsePin response, FOdinRoomAddMediaError onError,
                 FOdinRoomAddMediaSuccess onSuccess)
        : Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
        , RoomHandle(RoomHandle)
        , CaptureMedia(CaptureMedia)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(AddMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API RemoveMediaTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<RemoveMediaTask>;

    TWeakObjectPtr<UOdinCaptureMedia> Media;

    OdinRoomHandle              RoomHandle;
    FRemoveMediaResponsePin     Response;
    FOdinRoomRemoveMediaError   OnError;
    FOdinRoomRemoveMediaSuccess OnSuccess;

    RemoveMediaTask(OdinRoomHandle RoomHandle, TWeakObjectPtr<UOdinCaptureMedia> media,
                    FRemoveMediaResponsePin response, FOdinRoomRemoveMediaError onError,
                    FOdinRoomRemoveMediaSuccess onSuccess)
        : Media(media)
        , RoomHandle(RoomHandle)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(RemoveMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API PauseMediaTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<PauseMediaTask>;

    TWeakObjectPtr<UOdinPlaybackMedia> Media;

    FPauseMediaResponsePin     Response;
    FOdinRoomPauseMediaError   OnError;
    FOdinRoomPauseMediaSuccess OnSuccess;

    PauseMediaTask(TWeakObjectPtr<UOdinPlaybackMedia> media, FPauseMediaResponsePin response,
                   FOdinRoomPauseMediaError onError, FOdinRoomPauseMediaSuccess onSuccess)
        : Media(media)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(PauseMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API ResumeMediaTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<ResumeMediaTask>;

    TWeakObjectPtr<UOdinPlaybackMedia> Media;

    FResumeMediaResponsePin     Response;
    FOdinRoomResumeMediaError   OnError;
    FOdinRoomResumeMediaSuccess OnSuccess;

    ResumeMediaTask(TWeakObjectPtr<UOdinPlaybackMedia> media, FResumeMediaResponsePin response,
                    FOdinRoomResumeMediaError onError, FOdinRoomResumeMediaSuccess onSuccess)
        : Media(media)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(ResumeMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API UpdatePositionTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<UpdatePositionTask>;

    OdinRoomHandle RoomHandle;

    FVector Position;

    FUpdatePositionResponsePin     Response;
    FOdinRoomUpdatePositionError   OnError;
    FOdinRoomUpdatePositionSuccess OnSuccess;

    UpdatePositionTask(OdinRoomHandle roomHandle, FVector position,
                       FUpdatePositionResponsePin response, FOdinRoomUpdatePositionError onError,
                       FOdinRoomUpdatePositionSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Position(position)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UpdatePositionTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API UpdateScalingTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<UpdateScalingTask>;

    OdinRoomHandle RoomHandle;

    float Scale;

    UpdateScalingTask(OdinRoomHandle roomHandle, float scale)
        : RoomHandle(roomHandle)
        , Scale(scale)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UpdateScalingTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API UpdatePeerUserDataTask : public FNonAbandonableTask
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

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UpdatePeerUserDataTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API SendMessageTask : public FNonAbandonableTask
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

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(SendMessageTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API DestroyRoomTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<DestroyRoomTask>;

    OdinRoomHandle roomHandle;

    DestroyRoomTask(OdinRoomHandle roomHandle)
        : roomHandle(roomHandle)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(DestroyRoomTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};