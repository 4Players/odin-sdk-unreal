/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinNative/OdinNativeBlueprint.AsyncTask.h"

#include "OdinVoice.h"

void SendRpcTask::DoWork()
{
    auto ret = odin_room_send_rpc(RoomHandle, TCHAR_TO_UTF8(*Data));

    if (ret != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Send Rpc Task Error (odin_room_send_rpc)", ret);
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, error = (EOdinError)(ret + OdinUtility::EODIN_ERROR_OFFSET)]() {
                if (OnError.IsBound())
                    OnError.Execute(error);
                if (Response.IsBound())
                    Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response, rpc = Data]() {
                if (OnSuccess.IsBound())
                    OnSuccess.Execute(rpc);
                if (Response.IsBound())
                    Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void ChangeSelfRpcTask::DoWork()
{
    FString json = Payload.AsJson();
    auto    ret  = odin_room_send_rpc(RoomHandle, TCHAR_TO_UTF8(*json));

    if (ret != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Send ChangeSelf Task Error (odin_room_send_rpc)", ret);
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, error = (EOdinError)(ret + OdinUtility::EODIN_ERROR_OFFSET)]() {
                OnError.ExecuteIfBound(error);
                Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        // clean up media
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response, result = json]() {
                OnSuccess.ExecuteIfBound(result);
                Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void SetChannelMasksRpcTask::DoWork()
{
    FString json = Payload.AsJson();
    auto    ret  = odin_room_send_rpc(RoomHandle, TCHAR_TO_UTF8(*json));

    if (ret != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Send SetAudioMask Task Error (odin_room_send_rpc)", ret);
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, error = (EOdinError)(ret + OdinUtility::EODIN_ERROR_OFFSET)]() {
                OnError.ExecuteIfBound(error);
                Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        // clean up media
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response, result = json]() {
                OnSuccess.ExecuteIfBound(result);
                Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void SendMessageRpcTask::DoWork()
{
    FString json = Payload.AsJson();
    auto    ret  = odin_room_send_rpc(RoomHandle, TCHAR_TO_UTF8(*json));

    if (ret != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Send Message Task Error (odin_room_send_rpc)", ret);
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnError = OnError, Response = Response, error = (EOdinError)(ret + OdinUtility::EODIN_ERROR_OFFSET)]() {
                OnError.ExecuteIfBound(error);
                Response.Broadcast(false);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    } else {
        // clean up media
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [OnSuccess = OnSuccess, Response = Response, result = json]() {
                OnSuccess.ExecuteIfBound(result);
                Response.Broadcast(true);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}