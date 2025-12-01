/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinNative/OdinUtils.h"

#include "OdinCore/include/odin.h"

#include "Async/Async.h"
#include "Async/AsyncWork.h"

#include "OdinAudio/OdinDecoder.h"
#include "OdinAudio/OdinEncoder.h"
#include "OdinAudio/OdinPipeline.h"
#include "OdinRoom.h"
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
    auto action     = NewObject<UOdinNativeInitialize>();
    action->Version = version;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeInitialize::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            if (Version != ODIN_VERSION)
                return;

            auto result = odin_initialize(StringCast<ANSICHAR>(*Version).Get());

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(Version);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeShutdown* UOdinNativeShutdown::Shutdown(UObject* WorldContextObject, const FOdinNativeShutdownError& onError,
                                                   const FOdinNativeShutdownSuccess& onSuccess)
{
    auto action = NewObject<UOdinNativeShutdown>();
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
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeErrorGetLastError* UOdinNativeErrorGetLastError::ErrorGetLastError(UObject* WorldContextObject, const FOdinNativeErrorGetLastErrorError& onError,
                                                                              const FOdinNativeErrorGetLastErrorSuccess& onSuccess)
{
    auto action = NewObject<UOdinNativeErrorGetLastError>();
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeErrorGetLastError::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            FString result(odin_error_get_last_error());

            this->LastError = result;

            if (result.IsEmpty()) {
                OnError.ExecuteIfBound("");
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(result);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeErrorResetLastError* UOdinNativeErrorResetLastError::ErrorResetLastError(UObject*                                     WorldContextObject,
                                                                                    const FOdinNativeErrorResetLastErrorError&   onError,
                                                                                    const FOdinNativeErrorResetLastErrorSuccess& onSuccess)
{
    auto action = NewObject<UOdinNativeErrorResetLastError>();
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
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeDecoderIsSilent* UOdinNativeDecoderIsSilent::DecoderIsSilent(UObject* WorldContextObject, UPARAM(ref) UOdinDecoder*& decoder, const bool& bIsSilent,
                                                                        const FOdinNativeDecoderIsSilentError&   onError,
                                                                        const FOdinNativeDecoderIsSilentSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeDecoderIsSilent>();
    action->Decoder = decoder;
    action->Silent  = bIsSilent;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeDecoderIsSilent::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            this->Silent = odin_decoder_is_silent(this->Decoder->GetNativeHandle());
            OnSuccess.ExecuteIfBound(this->Silent);
            OnResponse.Broadcast(true);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeDecoderPush* UOdinNativeDecoderPush::DecoderPush(UObject* WorldContextObject, UPARAM(ref) UOdinDecoder*& decoder, const TArray<uint8>& datagram,
                                                            const FOdinNativeDecoderPushError& onError, const FOdinNativeDecoderPushSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativeDecoderPush>();
    action->Decoder  = decoder;
    action->Datagram = datagram;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeDecoderPush::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto size   = this->Datagram.Num();
            auto result = odin_decoder_push(this->Decoder->GetNativeHandle(), this->Datagram.GetData(), size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(size);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeDecoderPop* UOdinNativeDecoderPop::DecoderPop(UObject* WorldContextObject, UPARAM(ref) UOdinDecoder*& decoder, const TArray<float>& buffer,
                                                         const FOdinNativeDecoderPopError& onError, const FOdinNativeDecoderPopSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeDecoderPop>();
    action->Decoder = decoder;
    action->Samples = buffer;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeDecoderPop::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
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
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeDecoderGetPipeline* UOdinNativeDecoderGetPipeline::DecoderGetPipeline(UObject* WorldContextObject, UPARAM(ref) UOdinDecoder*& decoder,
                                                                                 const FOdinNativeDecoderGetPipelineError&   onError,
                                                                                 const FOdinNativeDecoderGetPipelineSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeDecoderGetPipeline>();
    action->Decoder = decoder;
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
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
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
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderCreate::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinEncoder* encoder;
            auto         result = odin_encoder_create(this->PeerId, this->Samplerate, this->Stereo, &encoder);

            this->Encoder->SetHandle(encoder);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->PeerId, this->Samplerate, this->Stereo, this->Encoder.Get());
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeEncoderCreateEx* UOdinNativeEncoderCreateEx::EncoderCreateEx(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, int64 peer_id,
                                                                        int32 sample_rate, bool stereo, bool application_voip, int32 bitrate_kbps,
                                                                        int32 packet_loss_perc, int64 update_position_interval_ms,
                                                                        const FOdinNativeEncoderCreateExError&   onError,
                                                                        const FOdinNativeEncoderCreateExSuccess& onSuccess)
{
    auto action                    = NewObject<UOdinNativeEncoderCreateEx>();
    action->PeerId                 = peer_id;
    action->Samplerate             = sample_rate;
    action->Stereo                 = stereo;
    action->PacketLossPerc         = packet_loss_perc;
    action->UpdatePositionInterval = update_position_interval_ms;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderCreateEx::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinEncoder* encoder;
            auto result = odin_encoder_create_ex(this->PeerId, this->Samplerate, this->Stereo, this->ApplicationVoip, this->BitrateKbps, this->PacketLossPerc,
                                                 this->UpdatePositionInterval, &encoder);

            this->Encoder->SetHandle(encoder);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->PeerId, this->Samplerate, this->Stereo, this->ApplicationVoip, this->BitrateKbps, this->UpdatePositionInterval,
                                         this->Encoder.Get());
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeEncoderFree* UOdinNativeEncoderFree::EncoderFree(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder,
                                                            const FOdinNativeEncoderFreeError& onError, const FOdinNativeEncoderFreeSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeEncoderFree>();
    action->Encoder = encoder;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderFree::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto ehandle = this->Encoder->GetHandle();
            odin_encoder_free(ehandle);
            this->Encoder = nullptr;
            OnSuccess.ExecuteIfBound();
            OnResponse.Broadcast(true);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeEncoderIsSilent* UOdinNativeEncoderIsSilent::EncoderIsSilent(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, const bool& bIsSilent,
                                                                        const FOdinNativeEncoderIsSilentError&   onError,
                                                                        const FOdinNativeEncoderIsSilentSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeEncoderIsSilent>();
    action->Encoder = encoder;
    action->Silent  = bIsSilent;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderIsSilent::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            this->Silent = odin_encoder_is_silent(this->Encoder->GetHandle());
            OnSuccess.ExecuteIfBound(this->Silent);
            OnResponse.Broadcast(true);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeEncoderPush* UOdinNativeEncoderPush::EncoderPush(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, const TArray<float>& samples,
                                                            const FOdinNativeEncoderPushError& onError, const FOdinNativeEncoderPushSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeEncoderPush>();
    action->Encoder = encoder;
    action->Samples = samples;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderPush::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
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
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeEncoderPop* UOdinNativeEncoderPop::EncoderPop(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder, const TArray<uint8_t>& buffer,
                                                         const FOdinNativeEncoderPopError& onError, const FOdinNativeEncoderPopSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativeEncoderPop>();
    action->Encoder  = encoder;
    action->Datagram = buffer;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderPop::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            uint32_t buffer_count = this->Datagram.Num();
            this->Datagram.Empty(buffer_count);
            auto ehandle = this->Encoder->GetHandle();
            auto result  = odin_encoder_pop(ehandle, this->Datagram.GetData(), &buffer_count);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Datagram);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeEncoderGetPipeline* UOdinNativeEncoderGetPipeline::EncoderGetPipeline(UObject* WorldContextObject, UPARAM(ref) UOdinEncoder*& encoder,
                                                                                 const FOdinNativeEncoderGetPipelineError&   onError,
                                                                                 const FOdinNativeEncoderGetPipelineSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeEncoderGetPipeline>();
    action->Encoder = encoder;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeEncoderGetPipeline::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto ehandle            = this->Encoder->GetHandle();
            this->Pipeline          = odin_encoder_get_pipeline(ehandle);
            UOdinPipeline* pipeline = NewObject<UOdinPipeline>();
            pipeline->SetHandle(this->Pipeline);

            if (this->Pipeline == nullptr) {
                OnError.ExecuteIfBound(this->Encoder);
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(pipeline);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineInsertVadEffect* UOdinNativePipelineInsertVadEffect::PipelineInsertVadEffect(UObject*                    WorldContextObject,
                                                                                                UPARAM(ref) UOdinPipeline*& pipeline, const int32 index,
                                                                                                const FOdinNativePipelineInsertVadEffectError&   onError,
                                                                                                const FOdinNativePipelineInsertVadEffectSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineInsertVadEffect>();
    action->Pipeline = pipeline;
    action->Index    = index;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineInsertVadEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_insert_vad_effect(phandle, this->Index, &this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Index, this->EffectId);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
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
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineInsertApmEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_insert_apm_effect(phandle, this->Index, this->PlaybackSamplerate, this->PlaybackStereo, &this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Index, this->EffectId);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineInsertCustomEffect* UOdinNativePipelineInsertCustomEffect::PipelineInsertCustomEffect(
    UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline, UPARAM(ref) UOdinCustomEffect*& effect, const int32 index,
    const FOdinNativePipelineInsertCustomEffectError& onError, const FOdinNativePipelineInsertCustomEffectSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineInsertCustomEffect>();
    action->Pipeline = pipeline;
    action->Effect   = effect;
    action->Index    = index;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineInsertCustomEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle  = this->Pipeline->GetHandle();
            auto callback = (OdinCustomEffectCallback)this->Effect;
            auto result   = odin_pipeline_insert_custom_effect(phandle, this->Index, callback, nullptr, &this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Index, this->EffectId);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineRemoveEffect* UOdinNativePipelineRemoveEffect::PipelineRemoveEffect(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                       const int32                                   effectId,
                                                                                       const FOdinNativePipelineRemoveEffectError&   onError,
                                                                                       const FOdinNativePipelineRemoveEffectSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineRemoveEffect>();
    action->Pipeline = pipeline;
    action->EffectId = effectId;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineRemoveEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_remove_effect(phandle, this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineMoveEffect* UOdinNativePipelineMoveEffect::PipelineMoveEffect(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                 const int32 effectId, const int32 newIndex,
                                                                                 const FOdinNativePipelineMoveEffectError&   onError,
                                                                                 const FOdinNativePipelineMoveEffectSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineMoveEffect>();
    action->Pipeline = pipeline;
    action->EffectId = effectId;
    action->NewIndex = newIndex;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineMoveEffect::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_move_effect(phandle, this->EffectId, this->NewIndex);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->NewIndex);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineGetEffectId* UOdinNativePipelineGetEffectId::PipelineGetEffectId(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                    const int32 index, const FOdinNativePipelineGetEffectIdError& onError,
                                                                                    const FOdinNativePipelineGetEffectIdSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineGetEffectId>();
    action->Pipeline = pipeline;
    action->Index    = index;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetEffectId::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_get_effect_id(phandle, this->Index, &this->EffectId);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->Index);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineGetEffectIndex* UOdinNativePipelineGetEffectIndex::PipelineGetEffectIndex(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                             const int32                                     effectId,
                                                                                             const FOdinNativePipelineGetEffectIndexError&   onError,
                                                                                             const FOdinNativePipelineGetEffectIndexSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineGetEffectIndex>();
    action->Pipeline = pipeline;
    action->EffectId = effectId;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetEffectIndex::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_get_effect_index(phandle, this->EffectId, &this->Index);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->Index);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineGetEffectType* UOdinNativePipelineGetEffectType::PipelineGetEffectType(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                          const int32                                    effectId,
                                                                                          const FOdinNativePipelineGetEffectTypeError&   onError,
                                                                                          const FOdinNativePipelineGetEffectTypeSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineGetEffectType>();
    action->Pipeline = pipeline;
    action->EffectId = effectId;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetEffectType::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
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
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineGetEffectCount* UOdinNativePipelineGetEffectCount::PipelineGetEffectCount(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                             const FOdinNativePipelineGetEffectCountError&   onError,
                                                                                             const FOdinNativePipelineGetEffectCountSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineGetEffectCount>();
    action->Pipeline = pipeline;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetEffectCount::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle      = this->Pipeline->GetHandle();
            this->EffectCount = odin_pipeline_get_effect_count(phandle);
            OnSuccess.ExecuteIfBound(this->EffectCount);
            OnResponse.Broadcast(true);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineSetVadConfig* UOdinNativePipelineSetVadConfig::PipelineSetVadConfig(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                       const int32 effectId, UPARAM(ref) struct FOdinVadConfig& config,
                                                                                       const FOdinNativePipelineSetVadConfigError&   onError,
                                                                                       const FOdinNativePipelineSetVadConfigSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineSetVadConfig>();
    action->Pipeline = pipeline;
    action->EffectId = effectId;
    action->Config   = &config;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineSetVadConfig::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto config  = (const OdinVadConfig*)this->Config;
            auto result  = odin_pipeline_set_vad_config(phandle, this->EffectId, config);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineGetVadConfig* UOdinNativePipelineGetVadConfig::PipelineGetVadConfig(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                       const int32                                   effectId,
                                                                                       const FOdinNativePipelineGetVadConfigError&   onError,
                                                                                       const FOdinNativePipelineGetVadConfigSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineGetVadConfig>();
    action->Pipeline = pipeline;
    action->EffectId = effectId;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineGetVadConfig::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_get_vad_config(phandle, this->EffectId, this->Config);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineSetApmConfig* UOdinNativePipelineSetApmConfig::PipelineSetApmConfig(UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline,
                                                                                       const int32                                   effectId,
                                                                                       const FOdinNativePipelineSetApmConfigError&   onError,
                                                                                       const FOdinNativePipelineSetApmConfigSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineSetApmConfig>();
    action->Pipeline = pipeline;
    action->EffectId = effectId;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineSetApmConfig::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_set_apm_config(phandle, this->EffectId, this->Config);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativePipelineUpdateApmPlayback* UOdinNativePipelineUpdateApmPlayback::PipelineUpdateApmPlayback(
    UObject* WorldContextObject, UPARAM(ref) UOdinPipeline*& pipeline, const int32 effect_id, const TArray<float>& samples, const int32 samples_count,
    const int32 delay, const FOdinNativePipelineUpdateApmPlaybackError& onError, const FOdinNativePipelineUpdateApmPlaybackSuccess& onSuccess)
{
    auto action      = NewObject<UOdinNativePipelineUpdateApmPlayback>();
    action->Pipeline = pipeline;
    action->EffectId = effect_id;
    action->Samples  = samples;
    action->Delay    = delay;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativePipelineUpdateApmPlayback::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto phandle = this->Pipeline->GetHandle();
            auto result  = odin_pipeline_update_apm_playback(phandle, this->EffectId, this->Samples.GetData(), this->Samples.Num(), this->Delay);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->EffectId, this->Samples.Num());
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomCreate* UOdinNativeRoomCreate::RoomCreate(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, const FString gateway,
                                                         const FString authentication, const FOdinNativeRoomCreateError& onError,
                                                         const FOdinNativeRoomCreateSuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeRoomCreate>();
    action->Room           = room;
    action->Gateway        = gateway;
    action->Authentication = authentication;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomCreate::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinRoom* room;
            auto      result = odin_room_create(StringCast<ANSICHAR>(*this->Gateway).Get(), StringCast<ANSICHAR>(*this->Authentication).Get(),
                                                this->Room->GetRoomEvents(), this->Room->GetRoomCipher(), &room);

            OdinRoom* lastHandle = this->Room->GetHandle();
            if (room) {
                auto esub = UOdinSubsystem::Get();
                if (esub && esub->GlobalIsRoomValid(lastHandle))
                    esub->SwapRoomHandle(lastHandle, room);
            }
            this->Room->SetHandle(room);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->Room->SetHandle(room);
                OnSuccess.ExecuteIfBound(this->Room.Get());
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomClose* UOdinNativeRoomClose::RoomClose(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, const FOdinNativeRoomCloseError& onError,
                                                      const FOdinNativeRoomCloseSuccess& onSuccess)
{
    auto action  = NewObject<UOdinNativeRoomClose>();
    action->Room = room;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomClose::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            odin_room_close(this->Room->GetHandle());
            this->Room->ConditionalBeginDestroy();
            OnSuccess.ExecuteIfBound();
            OnResponse.Broadcast(true);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomFree* UOdinNativeRoomFree::RoomFree(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, const FOdinNativeRoomFreeError& onError,
                                                   const FOdinNativeRoomFreeSuccess& onSuccess)
{
    auto action  = NewObject<UOdinNativeRoomFree>();
    action->Room = room;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomFree::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            odin_room_free(this->Room->GetHandle());
            this->Room->ConditionalBeginDestroy();
            OnSuccess.ExecuteIfBound();
            OnResponse.Broadcast(true);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomGetName* UOdinNativeRoomGetName::RoomGetName(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                            const FOdinNativeRoomGetNameError& onError, const FOdinNativeRoomGetNameSuccess& onSuccess)
{
    auto action  = NewObject<UOdinNativeRoomGetName>();
    action->Room = room;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomGetName::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            uint32_t size   = 1024;
            char*    buffer = new char[size];
            auto     result = odin_room_get_name(this->Room->GetHandle(), buffer, &size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->RoomName = FString(buffer);
                OnSuccess.ExecuteIfBound(this->RoomName);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomGetConnectionId* UOdinNativeRoomGetConnectionId::RoomGetConnectionId(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                                                    const FOdinNativeRoomGetConnectionIdError&   onError,
                                                                                    const FOdinNativeRoomGetConnectionIdSuccess& onSuccess)
{
    auto action  = NewObject<UOdinNativeRoomGetConnectionId>();
    action->Room = room;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomGetConnectionId::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto result = odin_room_get_connection_id(this->Room->GetHandle());
            OnSuccess.ExecuteIfBound(result);
            OnResponse.Broadcast(true);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomGetConnectionStats* UOdinNativeRoomGetConnectionStats::RoomGetConnectionStats(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                                                             const FOdinNativeRoomGetConnectionStatsError&   onError,
                                                                                             const FOdinNativeRoomGetConnectionStatsSuccess& onSuccess)
{
    auto action  = NewObject<UOdinNativeRoomGetConnectionStats>();
    action->Room = room;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomGetConnectionStats::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinConnectionStats stats;
            auto                result = odin_room_get_connection_stats(this->Room->GetHandle(), &stats);
            this->Stats                = FOdinConnectionStats(stats);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->Stats);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomResendUserData* UOdinNativeRoomResendUserData::RoomResendUserData(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                                                 const FOdinNativeRoomResendUserDataError&   onError,
                                                                                 const FOdinNativeRoomResendUserDataSuccess& onSuccess)
{
    auto action  = NewObject<UOdinNativeRoomResendUserData>();
    action->Room = room;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomResendUserData::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto result = odin_room_resend_user_data(this->Room->GetHandle());

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound();
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomSendRpc* UOdinNativeRoomSendRpc::RoomSendRpc(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, FString rpcBody,
                                                            const FOdinNativeRoomSendRpcError& onError, const FOdinNativeRoomSendRpcSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeRoomSendRpc>();
    action->Room    = room;
    action->RpcBody = rpcBody;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomSendRpc::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto result = odin_room_send_rpc(this->Room->GetHandle(), TCHAR_TO_UTF8(*this->RpcBody));

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->RpcBody);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomSendLoopbackRpc* UOdinNativeRoomSendLoopbackRpc::RoomSendLoopbackRpc(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room, FString rpcBody,
                                                                                    const FOdinNativeRoomSendLoopbackRpcError&   onError,
                                                                                    const FOdinNativeRoomSendLoopbackRpcSuccess& onSuccess)
{
    auto action     = NewObject<UOdinNativeRoomSendLoopbackRpc>();
    action->Room    = room;
    action->RpcBody = rpcBody;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomSendLoopbackRpc::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto result = odin_room_send_rpc(this->Room->GetHandle(), TCHAR_TO_UTF8(*this->RpcBody));

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->RpcBody.Len());
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeRoomSendDatagram* UOdinNativeRoomSendDatagram::RoomSendDatagram(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& room,
                                                                           const TArray<uint8>& datagram, const FOdinNativeRoomSendDatagramError& onError,
                                                                           const FOdinNativeRoomSendDatagramSuccess& onSuccess)
{
    auto action           = NewObject<UOdinNativeRoomSendDatagram>();
    action->Room          = room;
    action->DatagramBytes = datagram;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeRoomSendDatagram::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            auto result = odin_room_send_datagram(this->Room->GetHandle(), this->DatagramBytes.GetData(), this->DatagramBytes.Num());

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->DatagramBytes.Num());
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeTokenGeneratorCreate* UOdinNativeTokenGeneratorCreate::TokenGeneratorCreate(UObject* WorldContextObject, FString access_key,
                                                                                       const FOdinNativeTokenGeneratorCreateError&   onError,
                                                                                       const FOdinNativeTokenGeneratorCreateSuccess& onSuccess)
{
    auto action       = NewObject<UOdinNativeTokenGeneratorCreate>();
    action->AccessKey = access_key;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorCreate::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            OdinTokenGenerator* tokenGenerator;
            auto                result = odin_token_generator_create(TCHAR_TO_UTF8(*this->AccessKey), &tokenGenerator);
            this->TokenGenerator       = NewObject<UOdinTokenGenerator>(this->GetWorld());
            this->TokenGenerator->SetHandle(tokenGenerator);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                OnSuccess.ExecuteIfBound(this->TokenGenerator);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeTokenGeneratorFree* UOdinNativeTokenGeneratorFree::TokenGeneratorFree(UObject* WorldContextObject, UPARAM(ref) UOdinTokenGenerator*& tokenGenerator,
                                                                                 const FOdinNativeTokenGeneratorFreeError&   onError,
                                                                                 const FOdinNativeTokenGeneratorFreeSuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeTokenGeneratorFree>();
    action->TokenGenerator = tokenGenerator;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorFree::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            odin_token_generator_free(this->TokenGenerator->GetHandle());
            OnSuccess.ExecuteIfBound();
            OnResponse.Broadcast(true);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeTokenGeneratorGetAccessKey*
UOdinNativeTokenGeneratorGetAccessKey::TokenGeneratorGetAccessKey(UObject* WorldContextObject, UPARAM(ref) UOdinTokenGenerator*& tokenGenerator,
                                                                  const FOdinNativeTokenGeneratorGetAccessKeyError&   onError,
                                                                  const FOdinNativeTokenGeneratorGetAccessKeySuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeTokenGeneratorGetAccessKey>();
    action->TokenGenerator = tokenGenerator;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorGetAccessKey::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            uint32_t size   = 1024;
            char*    buffer = new char[size];
            auto     result = odin_token_generator_get_access_key(this->TokenGenerator->GetHandle(), buffer, &size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->AccessKey = FString(buffer);
                OnSuccess.ExecuteIfBound(this->AccessKey);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeTokenGeneratorGetKeyId* UOdinNativeTokenGeneratorGetKeyId::TokenGeneratorGetKeyId(UObject*                                        WorldContextObject,
                                                                                             UPARAM(ref) UOdinTokenGenerator*&               tokenGenerator,
                                                                                             const FOdinNativeTokenGeneratorGetKeyIdError&   onError,
                                                                                             const FOdinNativeTokenGeneratorGetKeyIdSuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeTokenGeneratorGetKeyId>();
    action->TokenGenerator = tokenGenerator;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorGetKeyId::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            uint32_t size   = 1024;
            char*    buffer = new char[size];
            auto     result = odin_token_generator_get_key_id(this->TokenGenerator->GetHandle(), buffer, &size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->KeyId = FString(buffer);
                OnSuccess.ExecuteIfBound(this->KeyId);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}

UOdinNativeTokenGeneratorSign* UOdinNativeTokenGeneratorSign::TokenGeneratorSign(UObject* WorldContextObject, UPARAM(ref) UOdinTokenGenerator*& tokenGenerator,
                                                                                 const FString body, const FOdinNativeTokenGeneratorSignError& onError,
                                                                                 const FOdinNativeTokenGeneratorSignSuccess& onSuccess)
{
    auto action            = NewObject<UOdinNativeTokenGeneratorSign>();
    action->TokenGenerator = tokenGenerator;
    action->Body           = body;
    action->RegisterWithGameInstance(WorldContextObject);
    return action;
}

void UOdinNativeTokenGeneratorSign::Activate()
{
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this]() {
            uint32_t size   = 4096;
            char*    buffer = new char[size];
            auto     result = odin_token_generator_sign(this->TokenGenerator->GetHandle(), TCHAR_TO_UTF8(*this->Body), buffer, &size);

            if (result != OdinError::ODIN_ERROR_SUCCESS) {
                OnError.ExecuteIfBound((EOdinError)(result + OdinUtility::EODIN_ERROR_OFFSET));
                OnResponse.Broadcast(false);
            } else {
                this->Token = FString(buffer);
                OnSuccess.ExecuteIfBound(this->Token);
                OnResponse.Broadcast(true);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);

    this->SetReadyToDestroy();
}