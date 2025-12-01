/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"

#include "OdinNativeBlueprint.h"
#include "OdinNativeRpc.h"

class ODIN_API SendRpcTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<SendRpcTask>;

    OdinRoom* RoomHandle;

    FString Data;

    FOdinNativeRoomSendRpcResponsePin Response;
    FOdinNativeRoomSendRpcError       OnError;
    FOdinNativeRoomSendRpcSuccess     OnSuccess;

    SendRpcTask(OdinRoom* roomHandle, FString data, FOdinNativeRoomSendRpcResponsePin response, FOdinNativeRoomSendRpcError onError,
                FOdinNativeRoomSendRpcSuccess onSuccess)
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
        RETURN_QUICK_DECLARE_CYCLE_STAT(SendRpcTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API ChangeSelfRpcTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<ChangeSelfRpcTask>;

    OdinRoom* RoomHandle;

    FOdinChangeSelf Payload;

    FOdinNativeRoomSendRpcResponsePin Response;
    FOdinNativeRoomSendRpcError       OnError;
    FOdinNativeRoomSendRpcSuccess     OnSuccess;

    ChangeSelfRpcTask(OdinRoom* roomHandle, FOdinChangeSelf payload, FOdinNativeRoomSendRpcResponsePin response, FOdinNativeRoomSendRpcError onError,
                      FOdinNativeRoomSendRpcSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Payload(payload)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(AddMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API SetChannelMasksRpcTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<SetChannelMasksRpcTask>;

    OdinRoom* RoomHandle;

    FOdinSetChannelMasks Payload;

    FOdinNativeRoomSendRpcResponsePin Response;
    FOdinNativeRoomSendRpcError       OnError;
    FOdinNativeRoomSendRpcSuccess     OnSuccess;

    SetChannelMasksRpcTask(OdinRoom* roomHandle, FOdinSetChannelMasks payload, FOdinNativeRoomSendRpcResponsePin response, FOdinNativeRoomSendRpcError onError,
                           FOdinNativeRoomSendRpcSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Payload(payload)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(AddMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API SendMessageRpcTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<SendMessageRpcTask>;

    OdinRoom* RoomHandle;

    FOdinSendMessage Payload;

    FOdinNativeRoomSendRpcResponsePin Response;
    FOdinNativeRoomSendRpcError       OnError;
    FOdinNativeRoomSendRpcSuccess     OnSuccess;

    SendMessageRpcTask(OdinRoom* roomHandle, FOdinSendMessage payload, FOdinNativeRoomSendRpcResponsePin response, FOdinNativeRoomSendRpcError onError,
                       FOdinNativeRoomSendRpcSuccess onSuccess)
        : RoomHandle(roomHandle)
        , Payload(payload)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(AddMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API DecoderPopTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<SendMessageRpcTask>;

    TWeakObjectPtr<UOdinDecoder> Decoder;

    FOdinNativeDecoderPopResponsePin Response;
    FOdinNativeDecoderPopError       OnError;
    FOdinNativeDecoderPopSuccess     OnSuccess;

    DecoderPopTask(TWeakObjectPtr<UOdinDecoder> decoder, FOdinNativeDecoderPopResponsePin response, FOdinNativeDecoderPopError onError,
                   FOdinNativeDecoderPopSuccess onSuccess)
        : Decoder(decoder)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(AddMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};

class ODIN_API EncoderPopTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<SendMessageRpcTask>;

    TWeakObjectPtr<UOdinEncoder> Encoder;

    FOdinNativeDecoderPopResponsePin Response;
    FOdinNativeDecoderPopError       OnError;
    FOdinNativeDecoderPopSuccess     OnSuccess;

    EncoderPopTask(TWeakObjectPtr<UOdinEncoder> encoder, FOdinNativeEncoderPopResponsePin response, FOdinNativeEncoderPopError onError,
                   FOdinNativeEncoderPopSuccess onSuccess)
        : Encoder(encoder)
        , Response(response)
        , OnError(onError)
        , OnSuccess(onSuccess)
    {
    }

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(AddMediaTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};