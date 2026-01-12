/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinDecoder.h"
#include "OdinCore/include/odin.h"

#include "OdinSubsystem.h"
#include "OdinVoice.h"
#include "OdinAudio/OdinPipeline.h"

#include "Async/Async.h"

UOdinDecoder::UOdinDecoder(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

void UOdinDecoder::SetHandle(OdinDecoder *NewHandle)
{
    if (GetNativeHandle() != NewHandle && GetNativeHandle()) {
        if (UOdinSubsystem *OdinSubsystem = UOdinSubsystem::Get()) {
            OdinSubsystem->DeregisterDecoder(this);
        }
    }

    if (NewHandle && !IsValid(Handle)) {
        Handle = NewObject<UOdinHandle>();
    }
    if (IsValid(Handle)) {
        Handle->SetHandle(NewHandle);
    }
    if (nullptr != NewHandle) {
        if (UOdinSubsystem *OdinSubsystem = UOdinSubsystem::Get()) {
            OdinSubsystem->RegisterDecoderObject(this);
        }
    }
}

void UOdinDecoder::BeginDestroy()
{
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
    FreeDecoderInternal(GetNativeHandle());
    Super::BeginDestroy();
}

UOdinDecoder *UOdinDecoder::ConstructDecoder(UObject *WorldContextObject, OdinDecoder *Handle)
{
    UOdinDecoder *Result = NewObject<UOdinDecoder>(WorldContextObject);
    Result->SetHandle(Handle);
    return Result;
}

UOdinDecoder *UOdinDecoder::ConstructDecoder(UObject *WorldContextObject, const int32 SampleRate, const bool bUseStereo)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::ConstructDecoder);
    ODIN_LOG(Verbose, "Construct Decoder with Sample Rate %d and Stereo Channels %d was called.", SampleRate, bUseStereo ? 2 : 1);

    UOdinDecoder *DecoderUObject = NewObject<UOdinDecoder>(WorldContextObject);
    DecoderUObject->SetupInternalDecoder(SampleRate, bUseStereo);

    return DecoderUObject;
}

UOdinDecoder *UOdinDecoder::SetupInternalDecoder(const int32 DecoderSampleRate, const bool bUseStereo)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::SetupInternalDecoder);
    ODIN_LOG(Verbose, "SetupInternalDecoder was called.")
    FreeDecoder(this);
    this->SampleRate = DecoderSampleRate;
    this->bStereo    = bUseStereo;

    OdinDecoder *NewDecoderHandle;
    const auto   Result = odin_decoder_create(this->SampleRate, this->bStereo, &NewDecoderHandle);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        SetHandle(NewDecoderHandle);
        return this;
    }
    FOdinModule::LogErrorCode("Aborting CreateDecoder due to invalid odin_decoder_create call: %s", Result);

    return nullptr;
}

bool UOdinDecoder::FreeDecoder(UOdinDecoder *Decoder)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::FreeDecoder);

    if (!IsValid(Decoder)) {
        ODIN_LOG(Verbose, TEXT("Aborted FreeDecoder due to invalid UOdinDecoder Pointer."));
        return false;
    }

    const auto bResult = FreeDecoderInternal(Decoder->GetNativeHandle());
    if (bResult) {
        Decoder->SetHandle(nullptr);
    }

    return bResult;
}

bool UOdinDecoder::FreeDecoderInternal(OdinDecoder *DecoderHandle)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::FreeDecoderInternal);

    if (DecoderHandle == nullptr) {
        ODIN_LOG(Verbose, TEXT("Aborted FreeDecoderInternal due to invalid OdinDecoder Pointer."));
        return false;
    }

    if (UOdinSubsystem *const OdinSub = UOdinSubsystem::Get()) {
        OdinSub->DeregisterDecoder(DecoderHandle);
    }

    odin_decoder_free(DecoderHandle);
    ODIN_LOG(Verbose, "Successfully freed Decoder %p", DecoderHandle);
    return true;
}

UOdinPipeline *UOdinDecoder::GetOrCreateDecoderPipeline(UOdinDecoder *Decoder)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::GetDecoderPipeline);

    if (!IsValid(Decoder)) {
        ODIN_LOG(Error, "Aborting GetDecoderPipeline due to invalid UOdinDecoder pin.");
        return nullptr;
    }
    return Decoder->GetOrCreatePipeline();
}

UOdinPipeline *UOdinDecoder::GetOrCreatePipeline()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::GetPipeline);

    if (IsValid(this->Pipeline)) {
        return this->Pipeline;
    }

    auto handle    = odin_decoder_get_pipeline(this->GetNativeHandle());
    this->Pipeline = UOdinPipeline::ConstructPipeline(this, handle);
    return this->Pipeline;
}

bool UOdinDecoder::GetIsSilent() const
{
    return odin_decoder_is_silent(this->GetNativeHandle());
}

bool UOdinDecoder::SetAudioEventHandler(int EFilter)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::SetAudioEventHandler);
    const TWeakObjectPtr<UOdinDecoder> DataPtr = this;
    const auto Result = odin_decoder_set_event_callback(this->GetNativeHandle(), static_cast<enum OdinAudioEvents>(EFilter), this->OdinDecoderEventCallbackFunc,
                                                        DataPtr.IsValid() ? DataPtr.Get() : nullptr);
    if (Result != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Aborting SetAudioEventHandler due to invalid odin_decoder_set_event_callback call: %s", Result);
        return false;
    }
    return true;
}

void UOdinDecoder::HandleOdinAudioEventCallback(OdinDecoder *DecoderHandle, const OdinAudioEvents Events, TWeakObjectPtr<UObject> UserData)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::HandleOdinAudioEventCallback)
    auto filter = static_cast<EOdinAudioEvents>(Events);
    ODIN_LOG(VeryVerbose, "Received HandleOdinAudioEventCallback for Decoder %p, Event: %s (%d)", DecoderHandle, *UEnum::GetValueAsString(filter), Events);

    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [DecoderHandle, filter, Events]() {
            const UOdinSubsystem        *OdinSubsystem    = UOdinSubsystem::Get();
            TWeakObjectPtr<UOdinDecoder> DecoderObjectPtr = OdinSubsystem->GetDecoderByHandle(DecoderHandle);
            if (!DecoderObjectPtr.IsValid() || DecoderObjectPtr.IsStale(true, true)) {
                ODIN_LOG(VeryVerbose,
                         "HandleOdinAudioEventCallback is aborted, referenced Decoder UObject is not valid anymore for Decoder %p, Event: %s, (%d)",
                         DecoderHandle, *UEnum::GetValueAsString(filter), Events);
                return;
            }
            if (DecoderObjectPtr->OnAudioEventCallbackBP.IsBound()) {
                DecoderObjectPtr->OnAudioEventCallbackBP.Broadcast(DecoderObjectPtr.Get(), filter);
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

int64 UOdinDecoder::GetActiveChannelMask() const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::GetActiveChannelMask);

    return odin_decoder_get_active_channels(this->GetNativeHandle());
}

TArray<FOdinPosition> UOdinDecoder::GetPositions(FOdinChannelMask channelMask) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::GetPositions);

    TArray<FOdinPosition> Positions;
    Positions.SetNumZeroed(64);
    OdinPosition *Buffer       = reinterpret_cast<OdinPosition *>(Positions.GetData());
    uint32        NumPositions = Positions.Num();
    auto          Result       = odin_decoder_get_positions(this->GetNativeHandle(), channelMask, Buffer, &NumPositions);
    if (Result != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Aborting GetPositions due to invalid odin_decoder_get_positions call: %s", Result);
    } else {
        ODIN_LOG(Verbose, "Received %d positions.", NumPositions);
    }

    return Positions;
}

int32 UOdinDecoder::Pop(float *Samples, int32 Count, bool *bSilence) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::Pop);
    auto ret = odin_decoder_pop(this->GetNativeHandle(), Samples, Count, bSilence);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        return Count;
    } else if (ret != OdinError::ODIN_ERROR_NO_DATA) {
        FOdinModule::LogErrorCode("Aborting Pop due to invalid odin_decoder_pop call: %s", ret);
    }

    return 0;
}

int32 UOdinDecoder::Pop(TArray<float> &Samples, bool &bSilence) const
{
    int32 size = Samples.Num();
    return Pop(Samples.GetData(), size, &bSilence);
}

int32 UOdinDecoder::Push(const TArray<uint8> &Datagram) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinDecoder::Push);
    auto size = Datagram.Num();
    auto ret  = odin_decoder_push(this->GetNativeHandle(), Datagram.GetData(), size);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        return size;
    } else {
        FOdinModule::LogErrorCode("Aborting Push due to invalid odin_decoder_push call: %s", ret);
    }

    return 0;
}
