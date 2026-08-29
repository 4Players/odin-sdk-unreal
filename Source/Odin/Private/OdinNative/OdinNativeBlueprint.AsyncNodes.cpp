/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinNative/OdinUtils.h"

#include "OdinCore/include/odin.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"

#include "OdinAudio/OdinDecoder.h"
#include "OdinAudio/OdinEncoder.h"
#include "OdinAudio/OdinPipeline.h"
#include "OdinRoom.h"
#include "OdinSocket.h"
#include "OdinSubsystem.h"
#include "OdinTokenGenerator.h"

#include "Engine/World.h"

FOdinPosition::FOdinPosition() {}

FOdinPosition::FOdinPosition(const FVector& In)
    : x(In.X)
    , y(In.Y)
    , z(In.Z)
{
}

UOdinNativeInitialize* UOdinNativeInitialize::Initialize(UObject* WorldContextObject, const FString version, const FOdinNativeInitializeError& onError,
                                                         const FOdinNativeInitializeSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeInitialize>();
    action->Version   = version;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeInitialize::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (Version != ODIN_VERSION) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_UNSUPPORTED_VERSION + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }

            auto result = odin_initialize(StringCast<ANSICHAR>(*Version).Get());

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(Version);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeShutdown* UOdinNativeShutdown::Shutdown(UObject* WorldContextObject, const FOdinNativeShutdownError& onError,
                                                   const FOdinNativeShutdownSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeShutdown>();
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeShutdown::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            odin_shutdown();
            OnSuccess.ExecuteIfBound();
            OnResponse.Broadcast(true);
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeErrorGetLastError* UOdinNativeErrorGetLastError::ErrorGetLastError(UObject* WorldContextObject, const FOdinNativeErrorGetLastErrorError& onError,
                                                                              const FOdinNativeErrorGetLastErrorSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeErrorGetLastError>();
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeErrorGetLastError::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            const char* lastError = odin_error_get_last_error();
            FString     result    = lastError ? FString(UTF8_TO_TCHAR(lastError)) : FString();

            this->LastError = result;

            if (result.IsEmpty()) {
                OnError.ExecuteIfBound("");
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(result);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeErrorResetLastError* UOdinNativeErrorResetLastError::ErrorResetLastError(UObject*                                     WorldContextObject,
                                                                                    const FOdinNativeErrorResetLastErrorError&   onError,
                                                                                    const FOdinNativeErrorResetLastErrorSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeErrorResetLastError>();
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeErrorResetLastError::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            odin_error_reset_last_error();
            OnSuccess.ExecuteIfBound();
            OnResponse.Broadcast(true);
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeDecoderIsSilent* UOdinNativeDecoderIsSilent::DecoderIsSilent(UObject* WorldContextObject, UPARAM(ref) UOdinDecoder*& decoder, const bool& bIsSilent,
                                                                        const FOdinNativeDecoderIsSilentError&   onError,
                                                                        const FOdinNativeDecoderIsSilentSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeDecoderIsSilent>();
    action->Decoder   = decoder;
    action->Silent    = bIsSilent;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeDecoderIsSilent::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Decoder)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            this->Silent = odin_decoder_is_silent(this->Decoder->GetNativeHandle());
            OnSuccess.ExecuteIfBound(this->Silent);
            OnResponse.Broadcast(true);
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeDecoderPush* UOdinNativeDecoderPush::DecoderPush(UObject* WorldContextObject, UPARAM(ref) UOdinDecoder*& decoder, const TArray<uint8>& datagram,
                                                            const FOdinNativeDecoderPushError& onError, const FOdinNativeDecoderPushSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeDecoderPush>();
    action->Decoder   = decoder;
    action->Datagram  = datagram;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeDecoderPush::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Decoder)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto size   = this->Datagram.Num();
            auto result = odin_decoder_push(this->Decoder->GetNativeHandle(), this->Datagram.GetData(), size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(size);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeDecoderPop* UOdinNativeDecoderPop::DecoderPop(UObject* WorldContextObject, UPARAM(ref) UOdinDecoder*& decoder, const TArray<float>& buffer,
                                                         const FOdinNativeDecoderPopError& onError, const FOdinNativeDecoderPopSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeDecoderPop>();
    action->Decoder   = decoder;
    action->Samples   = buffer;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeDecoderPop::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Decoder)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            const uint32_t buffer_count = this->Samples.Num();
            float*         buffer       = this->Samples.GetData();

            auto result = odin_decoder_pop(this->Decoder->GetNativeHandle(), buffer, buffer_count, &this->IsSilent);

            if (result != OdinError::ODIN_ERROR_SUCCESS && result != OdinError::ODIN_ERROR_NO_DATA) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                if (result == OdinError::ODIN_ERROR_NO_DATA)
                    this->Samples.Empty(buffer_count);

                OnSuccess.ExecuteIfBound(this->Samples, this->IsSilent);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeDecoderGetPipeline* UOdinNativeDecoderGetPipeline::DecoderGetPipeline(UObject* WorldContextObject, UPARAM(ref) UOdinDecoder*& decoder,
                                                                                 const FOdinNativeDecoderGetPipelineError&   onError,
                                                                                 const FOdinNativeDecoderGetPipelineSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeDecoderGetPipeline>();
    action->Decoder   = decoder;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeDecoderGetPipeline::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            UOdinPipeline* pipeline = nullptr;
            if (this->Decoder.IsValid()) {
                pipeline = Decoder->GetOrCreatePipeline();
            }

            if (!IsValid(pipeline)) {
                OnError.ExecuteIfBound(this->Decoder.Get());
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(pipeline);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeEncoderCreate* UOdinNativeEncoderCreate::EncoderCreate(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, int64 peer_id,
                                                                  int32 sample_rate, const bool stereo, const FOdinNativeEncoderCreateError& onError,
                                                                  const FOdinNativeEncoderCreateSuccess& onSuccess)
{
    auto action        = NewObject<UOdinNativeEncoderCreate>();
    action->Encoder    = encoder;
    action->PeerId     = peer_id;
    action->Samplerate = sample_rate;
    action->Stereo     = stereo;
    action->OnError    = onError;
    action->OnSuccess  = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderCreate::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!this->Encoder.IsValid()) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            OdinEncoder* encoder = nullptr;
            auto         result  = odin_encoder_create(this->PeerId, this->Samplerate, this->Stereo, &encoder);

            if (result == OdinError::ODIN_ERROR_SUCCESS) {
                this->Encoder->SetHandle(encoder);
            }

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->PeerId, this->Samplerate, this->Stereo, this->Encoder.Get());
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeEncoderCreateEx* UOdinNativeEncoderCreateEx::EncoderCreateEx(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, int64 peer_id,
                                                                        int32 sample_rate, bool stereo, bool application_voip, int32 bitrate_kbps,
                                                                        int32 packet_loss_perc, int64 update_position_interval_ms,
                                                                        const FOdinNativeEncoderCreateExError&   onError,
                                                                        const FOdinNativeEncoderCreateExSuccess& onSuccess)
{
    auto action                    = NewObject<UOdinNativeEncoderCreateEx>();
    action->Encoder                = encoder;
    action->PeerId                 = peer_id;
    action->Samplerate             = sample_rate;
    action->Stereo                 = stereo;
    action->ApplicationVoip        = application_voip;
    action->BitrateKbps            = bitrate_kbps;
    action->PacketLossPerc         = packet_loss_perc;
    action->UpdatePositionInterval = update_position_interval_ms;
    action->OnError                = onError;
    action->OnSuccess              = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderCreateEx::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!this->Encoder.IsValid()) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            OdinEncoder* encoder = nullptr;
            auto result = odin_encoder_create_ex(this->PeerId, this->Samplerate, this->Stereo, this->ApplicationVoip, this->BitrateKbps, this->PacketLossPerc,
                                                 this->UpdatePositionInterval, &encoder);

            if (result == OdinError::ODIN_ERROR_SUCCESS) {
                this->Encoder->SetHandle(encoder);
            }

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->PeerId, this->Samplerate, this->Stereo, this->ApplicationVoip, this->BitrateKbps, this->UpdatePositionInterval,
                                         this->Encoder.Get());
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeEncoderFree* UOdinNativeEncoderFree::EncoderFree(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder,
                                                            const FOdinNativeEncoderFreeError& onError, const FOdinNativeEncoderFreeSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeEncoderFree>();
    action->Encoder   = encoder;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderFree::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Encoder)) {
                OnError.ExecuteIfBound();
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            const bool bFreed = UOdinEncoder::FreeEncoder(this->Encoder);
            this->Encoder     = nullptr;
            if (!bFreed) {
                OnError.ExecuteIfBound();
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound();
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeEncoderIsSilent* UOdinNativeEncoderIsSilent::EncoderIsSilent(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, const bool& bIsSilent,
                                                                        const FOdinNativeEncoderIsSilentError&   onError,
                                                                        const FOdinNativeEncoderIsSilentSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeEncoderIsSilent>();
    action->Encoder   = encoder;
    action->Silent    = bIsSilent;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderIsSilent::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Encoder)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            this->Silent = odin_encoder_is_silent(this->Encoder->GetHandle());
            OnSuccess.ExecuteIfBound(this->Silent);
            OnResponse.Broadcast(true);
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeEncoderPush* UOdinNativeEncoderPush::EncoderPush(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, const TArray<float>& samples,
                                                            const FOdinNativeEncoderPushError& onError, const FOdinNativeEncoderPushSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeEncoderPush>();
    action->Encoder   = encoder;
    action->Samples   = samples;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderPush::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Encoder)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto size    = this->Samples.Num();
            auto ehandle = this->Encoder->GetHandle();
            auto result  = odin_encoder_push(ehandle, this->Samples.GetData(), size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(size);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeEncoderPop* UOdinNativeEncoderPop::EncoderPop(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, const TArray<uint8_t>& buffer,
                                                         const FOdinNativeEncoderPopError& onError, const FOdinNativeEncoderPopSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeEncoderPop>();
    action->Encoder   = encoder;
    action->Datagram  = buffer;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderPop::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Encoder)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            uint32_t buffer_count = this->Datagram.Num() > 0 ? this->Datagram.Num() : 4096;
            this->Datagram.SetNumUninitialized(buffer_count);
            auto ehandle = this->Encoder->GetHandle();
            auto result  = odin_encoder_pop(ehandle, this->Datagram.GetData(), &buffer_count);

            if (result != OdinError::ODIN_ERROR_SUCCESS && result != OdinError::ODIN_ERROR_NO_DATA) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->Datagram.SetNum(result == OdinError::ODIN_ERROR_SUCCESS ? buffer_count : 0);
                OnSuccess.ExecuteIfBound(this->Datagram);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeEncoderGetPipeline* UOdinNativeEncoderGetPipeline::EncoderGetPipeline(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder,
                                                                                 const FOdinNativeEncoderGetPipelineError&   onError,
                                                                                 const FOdinNativeEncoderGetPipelineSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeEncoderGetPipeline>();
    action->Encoder   = encoder;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderGetPipeline::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            UOdinPipeline* pipeline = UOdinEncoder::GetOrCreateEncoderPipeline(this->Encoder);

            if (!IsValid(pipeline)) {
                OnError.ExecuteIfBound(this->Encoder);
                OnResponse.Broadcast(false);
            } else {
                this->Pipeline = pipeline->GetHandle();
                OnSuccess.ExecuteIfBound(pipeline);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineInsertVadEffect* UOdinNativePipelineInsertVadEffect::PipelineInsertVadEffect(UObject*                    WorldContextObject,
                                                                                                UPARAM(ref) UOdinPipeline*& pipeline, const int32 index,
                                                                                                const FOdinNativePipelineInsertVadEffectError&   onError,
                                                                                                const FOdinNativePipelineInsertVadEffectSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineInsertVadEffect>();
    action->Pipeline  = pipeline;
    action->Index     = index;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineInsertVadEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_insert_vad_effect(phandle, this->Index, &this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Index, this->EffectId);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineInsertApmEffect* UOdinNativePipelineInsertApmEffect::PipelineInsertApmEffect(UObject*                    WorldContextObject,
                                                                                                UPARAM(ref) UOdinPipeline*& pipeline, const int32 index,
                                                                                                const int32 playback_sample_rate, const bool playback_stereo,
                                                                                                const FOdinNativePipelineInsertApmEffectError&   onError,
                                                                                                const FOdinNativePipelineInsertApmEffectSuccess& onSuccess)
{
    auto action                = NewObject<UOdinNativePipelineInsertApmEffect>();
    action->Pipeline           = pipeline;
    action->Index              = index;
    action->PlaybackSamplerate = playback_sample_rate;
    action->PlaybackStereo     = playback_stereo;
    action->OnError            = onError;
    action->OnSuccess          = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineInsertApmEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_insert_apm_effect(phandle, this->Index, this->PlaybackSamplerate, this->PlaybackStereo, &this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                if (IsValid(this->Pipeline)) {
                    this->Pipeline->RegisterApmPlaybackFormat(this->EffectId, this->PlaybackSamplerate, this->PlaybackStereo);
                }
                OnSuccess.ExecuteIfBound(this->Index, this->EffectId);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineInsertCustomEffect* UOdinNativePipelineInsertCustomEffect::PipelineInsertCustomEffect(
    UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline, UPARAM(ref) UOdinCustomEffect*& effect, const int32 index,
    const FOdinNativePipelineInsertCustomEffectError& onError, const FOdinNativePipelineInsertCustomEffectSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineInsertCustomEffect>();
    action->Pipeline  = pipeline;
    action->Effect    = effect;
    action->Index     = index;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineInsertCustomEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = this->Pipeline->InsertCustomEffectNative(this->Index, this->Effect, this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Index, this->EffectId);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineRemoveEffect* UOdinNativePipelineRemoveEffect::PipelineRemoveEffect(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                       const int32                                   effectId,
                                                                                       const FOdinNativePipelineRemoveEffectError&   onError,
                                                                                       const FOdinNativePipelineRemoveEffectSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineRemoveEffect>();
    action->Pipeline  = pipeline;
    action->EffectId  = effectId;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineRemoveEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = this->Pipeline->RemoveEffectWithCleanup(this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineMoveEffect* UOdinNativePipelineMoveEffect::PipelineMoveEffect(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                 const int32 effectId, const int32 newIndex,
                                                                                 const FOdinNativePipelineMoveEffectError&   onError,
                                                                                 const FOdinNativePipelineMoveEffectSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineMoveEffect>();
    action->Pipeline  = pipeline;
    action->EffectId  = effectId;
    action->NewIndex  = newIndex;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineMoveEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_move_effect(phandle, this->EffectId, this->NewIndex);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->NewIndex);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineGetEffectId* UOdinNativePipelineGetEffectId::PipelineGetEffectId(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                    const int32 index, const FOdinNativePipelineGetEffectIdError& onError,
                                                                                    const FOdinNativePipelineGetEffectIdSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineGetEffectId>();
    action->Pipeline  = pipeline;
    action->Index     = index;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetEffectId::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_get_effect_id(phandle, this->Index, &this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->Index);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineGetEffectIndex* UOdinNativePipelineGetEffectIndex::PipelineGetEffectIndex(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                             const int32                                     effectId,
                                                                                             const FOdinNativePipelineGetEffectIndexError&   onError,
                                                                                             const FOdinNativePipelineGetEffectIndexSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineGetEffectIndex>();
    action->Pipeline  = pipeline;
    action->EffectId  = effectId;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetEffectIndex::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_get_effect_index(phandle, this->EffectId, &this->Index);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->Index);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineGetEffectType* UOdinNativePipelineGetEffectType::PipelineGetEffectType(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                          const int32                                    effectId,
                                                                                          const FOdinNativePipelineGetEffectTypeError&   onError,
                                                                                          const FOdinNativePipelineGetEffectTypeSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineGetEffectType>();
    action->Pipeline  = pipeline;
    action->EffectId  = effectId;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetEffectType::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            OdinEffectType effectType;
            auto           phandle = this->Pipeline->GetHandle();
            auto           result  = odin_pipeline_get_effect_type(phandle, this->EffectId, &effectType);
            this->EffectType       = (EOdinEffectType)effectType;

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->EffectType);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineGetEffectCount* UOdinNativePipelineGetEffectCount::PipelineGetEffectCount(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                             const FOdinNativePipelineGetEffectCountError&   onError,
                                                                                             const FOdinNativePipelineGetEffectCountSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineGetEffectCount>();
    action->Pipeline  = pipeline;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetEffectCount::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto phandle      = this->Pipeline->GetHandle();
            this->EffectCount = odin_pipeline_get_effect_count(phandle);
            OnSuccess.ExecuteIfBound(this->EffectCount);
            OnResponse.Broadcast(true);
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineSetVadConfig* UOdinNativePipelineSetVadConfig::PipelineSetVadConfig(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                       const int32 effectId, UPARAM(ref) struct FOdinVadConfig& config,
                                                                                       const FOdinNativePipelineSetVadConfigError&   onError,
                                                                                       const FOdinNativePipelineSetVadConfigSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineSetVadConfig>();
    action->Pipeline  = pipeline;
    action->EffectId  = effectId;
    action->Config    = config;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineSetVadConfig::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto          phandle = this->Pipeline->GetHandle();
            OdinVadConfig config  = this->Config;
            auto          result  = odin_pipeline_set_vad_config(phandle, this->EffectId, &config);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineGetVadConfig* UOdinNativePipelineGetVadConfig::PipelineGetVadConfig(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                       const int32                                   effectId,
                                                                                       const FOdinNativePipelineGetVadConfigError&   onError,
                                                                                       const FOdinNativePipelineGetVadConfigSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineGetVadConfig>();
    action->Pipeline  = pipeline;
    action->EffectId  = effectId;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetVadConfig::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto          phandle = this->Pipeline->GetHandle();
            OdinVadConfig config  = {};
            auto          result  = odin_pipeline_get_vad_config(phandle, this->EffectId, &config);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->Config = FOdinVadConfig{.VoiceActivity = FOdinSensitivityConfig{.Enabled          = config.voice_activity.enabled,
                                                                                      .AttackThreshold  = config.voice_activity.attack_threshold,
                                                                                      .ReleaseThreshold = config.voice_activity.release_threshold},
                                              .VolumeGate    = FOdinSensitivityConfig{.Enabled          = config.volume_gate.enabled,
                                                                                      .AttackThreshold  = config.volume_gate.attack_threshold,
                                                                                      .ReleaseThreshold = config.volume_gate.release_threshold}};
                OnSuccess.ExecuteIfBound(this->EffectId, this->Config);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineSetApmConfig* UOdinNativePipelineSetApmConfig::PipelineSetApmConfig(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                       const int32 effectId, UPARAM(ref) struct FOdinApmConfig& config,
                                                                                       const FOdinNativePipelineSetApmConfigError&   onError,
                                                                                       const FOdinNativePipelineSetApmConfigSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineSetApmConfig>();
    action->Pipeline  = pipeline;
    action->EffectId  = effectId;
    action->Config    = config;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineSetApmConfig::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = this->Pipeline->SetApmConfigNative(this->EffectId, this->Config);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativePipelineUpdateApmPlayback* UOdinNativePipelineUpdateApmPlayback::PipelineUpdateApmPlayback(
    UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline, const int32 effect_id, const TArray<float>& samples, const int32 samples_count,
    const int32 delay, const FOdinNativePipelineUpdateApmPlaybackError& onError, const FOdinNativePipelineUpdateApmPlaybackSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativePipelineUpdateApmPlayback>();
    action->Pipeline  = pipeline;
    action->EffectId  = effect_id;
    action->Samples   = samples;
    action->Delay     = delay;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineUpdateApmPlayback::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Pipeline)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_update_apm_playback(phandle, this->EffectId, this->Samples.GetData(), this->Samples.Num(), this->Delay);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->Samples.Num());
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomCreate* UOdinNativeRoomCreate::RoomCreate(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, const FString gateway,
                                                         const FString authentication, const FOdinNativeRoomCreateError& onError,
                                                         const FOdinNativeRoomCreateSuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeRoomCreate>();
    action->Room           = room;
    action->Gateway        = gateway;
    action->Authentication = authentication;
    action->OnError        = onError;
    action->OnSuccess      = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomCreate::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!this->Room.IsValid()) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = this->Room->ConnectRoomNative(this->Gateway, this->Authentication, this->Room->Crypto);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Room.Get());
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomClose* UOdinNativeRoomClose::RoomClose(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, const FOdinNativeRoomCloseError& onError,
                                                      const FOdinNativeRoomCloseSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeRoomClose>();
    action->Room      = room;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomClose::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound();
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            const bool bHadHandle = this->Room->GetHandle() != nullptr;
            this->Room->ConditionalBeginDestroy();
            if (!bHadHandle) {
                OnError.ExecuteIfBound();
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound();
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomFree* UOdinNativeRoomFree::RoomFree(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, const FOdinNativeRoomFreeError& onError,
                                                   const FOdinNativeRoomFreeSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeRoomFree>();
    action->Room      = room;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomFree::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound();
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            const bool bFreed = this->Room->FreeRoom();
            this->Room->ConditionalBeginDestroy();
            if (!bFreed) {
                OnError.ExecuteIfBound();
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound();
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomGetName* UOdinNativeRoomGetName::RoomGetName(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                            const FOdinNativeRoomGetNameError& onError, const FOdinNativeRoomGetNameSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeRoomGetName>();
    action->Room      = room;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomGetName::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            uint32_t     size = 1024;
            TArray<char> buffer;
            buffer.SetNumZeroed(size + 1);
            auto result = odin_room_get_name(this->Room->GetHandle(), buffer.GetData(), &size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->RoomName = FString(UTF8_TO_TCHAR(buffer.GetData()));
                OnSuccess.ExecuteIfBound(this->RoomName);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomGetConnectionId* UOdinNativeRoomGetConnectionId::RoomGetConnectionId(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                                                    const FOdinNativeRoomGetConnectionIdError&   onError,
                                                                                    const FOdinNativeRoomGetConnectionIdSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeRoomGetConnectionId>();
    action->Room      = room;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomGetConnectionId::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = odin_room_get_connection_id(this->Room->GetHandle());
            OnSuccess.ExecuteIfBound(result);
            OnResponse.Broadcast(true);
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomGetConnectionStats* UOdinNativeRoomGetConnectionStats::RoomGetConnectionStats(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                                                             const FOdinNativeRoomGetConnectionStatsError&   onError,
                                                                                             const FOdinNativeRoomGetConnectionStatsSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeRoomGetConnectionStats>();
    action->Room      = room;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomGetConnectionStats::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            OdinConnectionStats stats  = {};
            auto                result = odin_room_get_connection_stats(this->Room->GetHandle(), &stats);
            this->Stats                = FOdinConnectionStats(stats);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Stats);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomResendUserData* UOdinNativeRoomResendUserData::RoomResendUserData(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                                                 const FOdinNativeRoomResendUserDataError&   onError,
                                                                                 const FOdinNativeRoomResendUserDataSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeRoomResendUserData>();
    action->Room      = room;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomResendUserData::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = odin_room_resend_user_data(this->Room->GetHandle());

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound();
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomSendRpc* UOdinNativeRoomSendRpc::RoomSendRpc(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, FString rpcBody,
                                                            const FOdinNativeRoomSendRpcError& onError, const FOdinNativeRoomSendRpcSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeRoomSendRpc>();
    action->Room      = room;
    action->RpcBody   = rpcBody;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomSendRpc::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = odin_room_send_rpc(this->Room->GetHandle(), TCHAR_TO_UTF8(*this->RpcBody));

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->RpcBody);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomSendLoopbackRpc* UOdinNativeRoomSendLoopbackRpc::RoomSendLoopbackRpc(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, FString rpcBody,
                                                                                    const FOdinNativeRoomSendLoopbackRpcError&   onError,
                                                                                    const FOdinNativeRoomSendLoopbackRpcSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeRoomSendLoopbackRpc>();
    action->Room      = room;
    action->RpcBody   = rpcBody;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomSendLoopbackRpc::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = odin_room_send_loopback_rpc(this->Room->GetHandle(), TCHAR_TO_UTF8(*this->RpcBody));

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->RpcBody.Len());
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeRoomSendDatagram* UOdinNativeRoomSendDatagram::RoomSendDatagram(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                                           const TArray<uint8>& datagram, const FOdinNativeRoomSendDatagramError& onError,
                                                                           const FOdinNativeRoomSendDatagramSuccess& onSuccess)
{
    auto action           = NewObject<UOdinNativeRoomSendDatagram>();
    action->Room          = room;
    action->DatagramBytes = datagram;
    action->OnError       = onError;
    action->OnSuccess     = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomSendDatagram::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->Room)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            auto result = odin_room_send_datagram(this->Room->GetHandle(), this->DatagramBytes.GetData(), this->DatagramBytes.Num());

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->DatagramBytes.Num());
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeSocketCreate* UOdinNativeSocketCreate::SocketCreate(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, EOdinSocketKind socketKind,
                                                               int32 remotePeerId, int32 label, int32 priority, const FOdinNativeSocketCreateError& onError,
                                                               const FOdinNativeSocketCreateSuccess& onSuccess)
{
    auto action          = NewObject<UOdinNativeSocketCreate>();
    action->Room         = room;
    action->SocketKind   = socketKind;
    action->TargetPeerId = remotePeerId;
    action->Label        = label;
    action->Priority     = priority;
    action->OnError      = onError;
    action->OnSuccess    = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeSocketCreate::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinSocket* socketHandle = nullptr;
            OdinRoom*   handle       = nullptr;
            if (const UOdinRoom* room = this->Room.Get())
                handle = room->GetHandle();

            auto result =
                odin_socket_create(handle, static_cast<OdinSocketKind>(this->SocketKind), this->TargetPeerId, this->Label, this->Priority, &socketHandle);
            this->Socket = NewObject<UOdinSocket>(this->GetWorld());
            if (result == OdinError::ODIN_ERROR_SUCCESS) {
                this->Socket->SetHandle(socketHandle);
            }

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Socket);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeSocketInfo* UOdinNativeSocketInfo::SocketInfo(UObject* WorldContextObject, UPARAM(ref) UOdinSocket*& socket,
                                                         const FOdinNativeSocketInfoError& onError, const FOdinNativeSocketInfoSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeSocketInfo>();
    action->Socket    = socket;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeSocketInfo::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinSocketInfo socketInfo = {};
            OdinSocket*    handle     = nullptr;
            if (const UOdinSocket* socket = this->Socket.Get())
                handle = socket->GetNativeHandle();

            auto result = odin_socket_info(handle, &socketInfo);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->Info = FOdinSocketInfo(socketInfo);
                OnSuccess.ExecuteIfBound(this->Info);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeSocketSend* UOdinNativeSocketSend::SocketSend(UObject* WorldContextObject, UPARAM(ref) UOdinSocket*& socket, const TArray<uint8>& message,
                                                         const FOdinNativeSocketSendError& onError, const FOdinNativeSocketSendSuccess& onSuccess)
{
    auto action          = NewObject<UOdinNativeSocketSend>();
    action->Socket       = socket;
    action->MessageBytes = message;
    action->OnError      = onError;
    action->OnSuccess    = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeSocketSend::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinSocket* handle = nullptr;
            if (const UOdinSocket* socket = this->Socket.Get())
                handle = socket->GetNativeHandle();

            auto result = odin_socket_send(handle, this->MessageBytes.GetData(), this->MessageBytes.Num());

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->MessageBytes.Num());
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeSocketReset* UOdinNativeSocketReset::SocketReset(UObject* WorldContextObject, UPARAM(ref) UOdinSocket*& socket,
                                                            const FOdinNativeSocketResetError& onError, const FOdinNativeSocketResetSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeSocketReset>();
    action->Socket    = socket;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeSocketReset::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinSocket* handle = nullptr;
            if (const UOdinSocket* socket = this->Socket.Get())
                handle = socket->GetNativeHandle();

            auto result = odin_socket_reset(handle);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->Socket.Reset();
                OnSuccess.ExecuteIfBound();
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeTokenGeneratorCreate* UOdinNativeTokenGeneratorCreate::TokenGeneratorCreate(UObject* WorldContextObject, FString access_key,
                                                                                       const FOdinNativeTokenGeneratorCreateError&   onError,
                                                                                       const FOdinNativeTokenGeneratorCreateSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeTokenGeneratorCreate>();
    action->AccessKey = access_key;
    action->OnError   = onError;
    action->OnSuccess = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorCreate::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinTokenGenerator* tokenGenerator = nullptr;
            auto                result         = odin_token_generator_create(TCHAR_TO_UTF8(*this->AccessKey), &tokenGenerator);
            this->TokenGenerator               = NewObject<UOdinTokenGenerator>(this->GetWorld());
            if (result == OdinError::ODIN_ERROR_SUCCESS) {
                this->TokenGenerator->SetHandle(tokenGenerator);
            }

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->TokenGenerator);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeTokenGeneratorFree* UOdinNativeTokenGeneratorFree::TokenGeneratorFree(UObject* WorldContextObject, UPARAM(ref) UOdinTokenGenerator*& tokenGenerator,
                                                                                 const FOdinNativeTokenGeneratorFreeError&   onError,
                                                                                 const FOdinNativeTokenGeneratorFreeSuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeTokenGeneratorFree>();
    action->TokenGenerator = tokenGenerator;
    action->OnError        = onError;
    action->OnSuccess      = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorFree::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->TokenGenerator)) {
                OnError.ExecuteIfBound();
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            const bool bHadHandle = this->TokenGenerator->GetHandle() != nullptr;
            this->TokenGenerator->ReleaseHandle();
            if (!bHadHandle) {
                OnError.ExecuteIfBound();
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound();
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeTokenGeneratorGetAccessKey*
UOdinNativeTokenGeneratorGetAccessKey::TokenGeneratorGetAccessKey(UObject* WorldContextObject, UPARAM(ref) UOdinTokenGenerator*& tokenGenerator,
                                                                  const FOdinNativeTokenGeneratorGetAccessKeyError&   onError,
                                                                  const FOdinNativeTokenGeneratorGetAccessKeySuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeTokenGeneratorGetAccessKey>();
    action->TokenGenerator = tokenGenerator;
    action->OnError        = onError;
    action->OnSuccess      = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorGetAccessKey::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->TokenGenerator)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            uint32_t     size = 1024;
            TArray<char> buffer;
            buffer.SetNumZeroed(size + 1);
            auto result = odin_token_generator_get_access_key(this->TokenGenerator->GetHandle(), buffer.GetData(), &size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->AccessKey = FString(UTF8_TO_TCHAR(buffer.GetData()));
                OnSuccess.ExecuteIfBound(this->AccessKey);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeTokenGeneratorGetKeyId* UOdinNativeTokenGeneratorGetKeyId::TokenGeneratorGetKeyId(UObject*                                        WorldContextObject,
                                                                                             UPARAM(ref) UOdinTokenGenerator*&               tokenGenerator,
                                                                                             const FOdinNativeTokenGeneratorGetKeyIdError&   onError,
                                                                                             const FOdinNativeTokenGeneratorGetKeyIdSuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeTokenGeneratorGetKeyId>();
    action->TokenGenerator = tokenGenerator;
    action->OnError        = onError;
    action->OnSuccess      = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorGetKeyId::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->TokenGenerator)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            uint32_t     size = 1024;
            TArray<char> buffer;
            buffer.SetNumZeroed(size + 1);
            auto result = odin_token_generator_get_key_id(this->TokenGenerator->GetHandle(), buffer.GetData(), &size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->KeyId = FString(UTF8_TO_TCHAR(buffer.GetData()));
                OnSuccess.ExecuteIfBound(this->KeyId);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

UOdinNativeTokenGeneratorSign* UOdinNativeTokenGeneratorSign::TokenGeneratorSign(UObject* WorldContextObject, UPARAM(ref) UOdinTokenGenerator*& tokenGenerator,
                                                                                 const FString body, const FOdinNativeTokenGeneratorSignError& onError,
                                                                                 const FOdinNativeTokenGeneratorSignSuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeTokenGeneratorSign>();
    action->TokenGenerator = tokenGenerator;
    action->Body           = body;
    action->OnError        = onError;
    action->OnSuccess      = onSuccess;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorSign::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (!IsValid(this->TokenGenerator)) {
                OnError.ExecuteIfBound((EOdinError)(OdinError::ODIN_ERROR_ARGUMENT_NULL + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
                this->SetReadyToDestroy();
                return;
            }
            uint32_t     size = 4096;
            TArray<char> buffer;
            buffer.SetNumZeroed(size + 1);
            auto result = odin_token_generator_sign(this->TokenGenerator->GetHandle(), TCHAR_TO_UTF8(*this->Body), buffer.GetData(), &size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->Token = FString(UTF8_TO_TCHAR(buffer.GetData()));
                OnSuccess.ExecuteIfBound(this->Token);
                OnResponse.Broadcast(true);
            }
            this->SetReadyToDestroy();
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}