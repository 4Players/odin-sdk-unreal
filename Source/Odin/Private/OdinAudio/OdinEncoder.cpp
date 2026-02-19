/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinEncoder.h"

#include "AudioDevice.h"
#include "OdinFunctionLibrary.h"
#include "OdinSubsystem.h"
#include "OdinVoice.h"
#include "SampleBuffer.h"
#include "OdinAudio/OdinPipeline.h"
#include "Runtime/Launch/Resources/Version.h"

UOdinEncoder::UOdinEncoder(const class FObjectInitializer& PCIP)
    : Super(PCIP)
    , SubmixListener(MakeShared<FOdinSubmixListener>())
{
}

void UOdinEncoder::BeginDestroy()
{
    ODIN_LOG(Verbose, "ODIN Destroy: %s", ANSI_TO_TCHAR(__FUNCTION__));
    if (IsValid(AudioGenerator)) {
        AudioGenerator->RemoveGeneratorDelegate(Audio_Generator_Handle);
    }
    this->AudioGenerator = nullptr;
    FreeEncoder(this);
    if (Pipeline) {
        Pipeline->OnApmConfigChanged.RemoveDynamic(this, &UOdinEncoder::OnPipelineApmConfigChanged);
    }
    if (SubmixListener.IsValid()) {
        SubmixListener->DetachFromSubmix();
        SubmixListener.Reset();
    }

    Super::BeginDestroy();
}

UOdinEncoder* UOdinEncoder::ConstructEncoder(UObject* WorldContextObject, OdinEncoder* Handle)
{
    UOdinEncoder* result = NewObject<UOdinEncoder>(WorldContextObject);
    result->SetHandle(Handle);

    return result;
}

UOdinEncoder* UOdinEncoder::ConstructEncoder(UObject* WorldContextObject, int64 PeerId, int32 SampleRate, bool bStereo)
{
    UOdinEncoder* result = NewObject<UOdinEncoder>(WorldContextObject);
    result               = result->CreateEncoder(PeerId, SampleRate, bStereo); // override self on invalid odin call
    return result;
}

UOdinEncoder* UOdinEncoder::ConstructEncoderEx(UObject* WorldContextObject, int64 PeerId, int32 SampleRate, bool bStereo, bool bApplication_VOIP,
                                               int32 Bitrate_Kbps, int32 Packet_Loss_Perc, int64 Update_Position_Interval_MS)
{
    UOdinEncoder* result = NewObject<UOdinEncoder>(WorldContextObject);
    result               = result->CreateEncoderEx(PeerId, SampleRate, bStereo, bApplication_VOIP, Bitrate_Kbps, Packet_Loss_Perc,
                                                   Update_Position_Interval_MS); // override self on invalid odin call
    return result;
}

UOdinEncoder* UOdinEncoder::CreateEncoder(int64 InPeerId, int32 InSampleRate, bool bUseStereo)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::CreateEncoder);

    OdinEncoder* encoder = this->GetHandle();
    if (encoder != nullptr) {
        FreeEncoderHandle(encoder);
    }

    this->PeerId     = InPeerId;
    this->SampleRate = InSampleRate;
    this->bStereo    = bUseStereo;

    ODIN_LOG(Verbose, "odin_encoder_create for peer %lld with sample rate: %d, channels: %d", InPeerId, InSampleRate, (bUseStereo ? 2 : 1));

    OdinEncoder* handle;
    auto         ret = odin_encoder_create(this->PeerId, this->SampleRate, this->bStereo, &handle);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        this->SetHandle(handle);
        return this;
    } else {
        FOdinModule::LogErrorCode("Aborting CreateEncoder due to invalid odin_encoder_create call: %s", ret);
    }

    return nullptr;
}

UOdinEncoder* UOdinEncoder::CreateEncoderEx(int64 InConnectedPeerId, int32 InSampleRate, bool bUseStereo, bool bApplication_VOIP, int32 Bitrate_Kbps,
                                            int32 Packet_Loss_Perc, int64 Update_Position_Interval_MS)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::CreateEncoderEx);

    OdinEncoder* encoder = this->GetHandle();
    if (encoder != nullptr)
        FreeEncoderHandle(encoder);

    this->PeerId     = InConnectedPeerId;
    this->SampleRate = InSampleRate;
    this->bStereo    = bUseStereo;

    ODIN_LOG(Verbose, "odin_encoder_create_ex for peer %lld with sample rate: %d, channels: %d voip: %d bitrate: %d kbps interval: %d ms", InConnectedPeerId,
             InSampleRate, (bUseStereo ? 2 : 1), bApplication_VOIP, Bitrate_Kbps, Update_Position_Interval_MS);

    OdinEncoder* handle;
    auto         ret = odin_encoder_create_ex(this->PeerId, this->SampleRate, this->bStereo, bApplication_VOIP, Bitrate_Kbps, Packet_Loss_Perc,
                                              Update_Position_Interval_MS, &handle);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        this->SetHandle(handle);
        return this;
    } else {
        FOdinModule::LogErrorCode("Aborting CreateEncoderEx due to invalid odin_encoder_create_ex call: %s", ret);
    }

    return nullptr;
}

bool UOdinEncoder::FreeEncoder(UOdinEncoder* Encoder)
{

    if (!IsValid(Encoder)) {
        ODIN_LOG(Verbose, "FreeEncoder was called with invalid Odin Encoder UObject Pointer");
        return false;
    }

    if (Encoder->SubmixListener.IsValid()) {
        Encoder->SubmixListener->DetachFromSubmix();
    }

    const bool Result = FreeEncoderHandle(Encoder->GetHandle());
    if (Result) {
        Encoder->SetHandle(nullptr);
    }

    return Result;
}

bool UOdinEncoder::FreeEncoderHandle(OdinEncoder* Handle)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::FreeEncoderHandle);

    if (Handle == nullptr) {
        ODIN_LOG(Verbose, "FreeEncoderHandle was called with invalid Handle");
        return false;
    }

    if (UOdinSubsystem* const& OdinSubsystem = UOdinSubsystem::Get()) {
        OdinSubsystem->UnlinkEncoder(Handle);
    }

    odin_encoder_free(Handle);
    return true;
}

UOdinPipeline* UOdinEncoder::GetOrCreateEncoderPipeline(UOdinEncoder* Encoder)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::GetEncoderPipeline);

    if (!IsValid(Encoder)) {
        ODIN_LOG(Error, "Aborting GetPipeline due to invalid UOdinEncoder pin.");
        return nullptr;
    }

    return Encoder->GetOrCreatePipeline();
}

UOdinPipeline* UOdinEncoder::GetOrCreatePipeline()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::GetPipeline);

    if (IsValid(this->Pipeline)) {
        return this->Pipeline;
    }

    auto handle    = odin_encoder_get_pipeline(this->GetHandle());
    this->Pipeline = UOdinPipeline::ConstructPipeline(this, handle);
    if (Pipeline) {
        Pipeline->OnApmConfigChanged.AddUniqueDynamic(this, &UOdinEncoder::OnPipelineApmConfigChanged);
        if (SubmixListener.IsValid()) {
            SubmixListener->SetPipelineHandle(Pipeline);
        }
    }

    return this->Pipeline;
}

bool UOdinEncoder::GetIsSilent() const
{
    return odin_encoder_is_silent(this->GetHandle());
}

bool UOdinEncoder::SetAudioEventHandler(int EFilter)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::SetAudioEventHandler);
    const TWeakObjectPtr<UOdinEncoder> DataPtr = this;
    const auto Result = odin_encoder_set_event_callback(this->GetHandle(), static_cast<enum OdinAudioEvents>(EFilter), this->OdinEncoderEventCallbackFunc,
                                                        DataPtr.IsValid() ? DataPtr.Get() : nullptr);
    if (Result != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Aborting SetAudioEventHandler due to invalid odin_encoder_set_event_callback call: %s", Result);
        return false;
    }
    return true;
}

void UOdinEncoder::HandleOdinAudioEventCallback(OdinEncoder* EncoderHandle, const OdinAudioEvents Events, TWeakObjectPtr<UOdinEncoder> WeakEncoderPtr)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::HandleOdinAudioEventCallback)
    auto filter = static_cast<EOdinAudioEvents>(Events);
    ODIN_LOG(VeryVerbose, "Received HandleOdinAudioEventCallback for Encoder %p, Event: %s (%d)", EncoderHandle, *UEnum::GetValueAsString(filter), Events);
    if (!WeakEncoderPtr.IsValid() || WeakEncoderPtr.IsStale(true, true)) {
        ODIN_LOG(VeryVerbose, "HandleOdinAudioEventCallback is aborted, referenced Encoder UObject is not valid anymore for Encoder %p, Event: %s, (%d)",
                 EncoderHandle, *UEnum::GetValueAsString(filter), Events);
        return;
    }

    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [EncoderHandle, WeakEncoderPtr, filter]() {
            if (UOdinEncoder* EncoderPtr = WeakEncoderPtr.Get()) {
                // it is possible to proxy a callback
                if (const OdinEncoder* userDataHandle = EncoderPtr->GetHandle(); userDataHandle != EncoderHandle) {
                    ODIN_LOG(Verbose, "Received missmatch Encoder %p != %p in HandleOdinAudioEventCallback. Redirect to %p", EncoderHandle, userDataHandle,
                             userDataHandle);
                }
                if (EncoderPtr->OnAudioEventCallbackBP.IsBound()) {
                    EncoderPtr->OnAudioEventCallbackBP.Broadcast(EncoderPtr, filter);
                }
            }
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

void UOdinEncoder::SetAudioGenerator(UAudioGenerator* Generator)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::SetAudioGenerator);

    if (!Generator) {
        ODIN_LOG(Error, "UOdinEncoder::SetAudioCapture - audio capture is null, microphone will "
                        "not work.");
    }
    if (IsValid(AudioGenerator)) {
        AudioGenerator->RemoveGeneratorDelegate(Audio_Generator_Handle);
    }

    this->AudioGenerator    = Generator;
    int32 CaptureSampleRate = AudioGenerator->GetSampleRate();
    int32 CaptureChannels   = AudioGenerator->GetNumChannels();
    int32 OdinSampleRate    = SampleRate;
    int32 OdinChannels      = bStereo + 1;

    TWeakObjectPtr<UOdinHandle>    WeakOdinHandle = Handle;
    TWeakObjectPtr<UOdinSubsystem> SubsystemPtr   = UOdinSubsystem::Get();
    // Create generator delegate TFunction<void(const float *InAudio, int32 NumSamples)>
    TFunction<void(const float* InAudio, int32 NumSamples)> audioGeneratorHandle = [CaptureSampleRate, CaptureChannels, OdinSampleRate, OdinChannels,
                                                                                    WeakOdinHandle, SubsystemPtr](const float* InAudio, int32 NumSamples) {
        TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder - Audio Generator Callback);

        const float* pbuffer   = InAudio;
        int32        bufferNum = NumSamples;

        ODIN_LOG(VeryVerbose, "Encoder, stream: %d hz %d ch, capture: %d hz %d ch. ue-downmix: %d, odin-resample: %d", OdinSampleRate, OdinChannels,
                 CaptureSampleRate, CaptureChannels, (OdinChannels != CaptureChannels), (OdinSampleRate != CaptureSampleRate));

        // downmix channels
        if (OdinChannels != CaptureChannels) {
            Audio::TSampleBuffer<float> buffer(InAudio, NumSamples, CaptureChannels, CaptureSampleRate);
            buffer.MixBufferToChannels(OdinChannels);
            pbuffer   = buffer.GetData();
            bufferNum = buffer.GetNumSamples();
        }

        OdinEncoder* EncoderHandle = nullptr;
        if (const UOdinHandle* OdinHandle = WeakOdinHandle.Get()) {
            EncoderHandle = static_cast<OdinEncoder*>(OdinHandle->GetHandle());
        }
        if (EncoderHandle) {
            if (SubsystemPtr.IsValid()) {
                SubsystemPtr->PushAudioToEncoder(EncoderHandle, TArray<float>(pbuffer, bufferNum));
            }
        }
    };
    this->Audio_Generator_Handle = AudioGenerator->AddGeneratorDelegate(audioGeneratorHandle);
}

bool UOdinEncoder::SetPosition(FOdinChannelMask ChannelMask, FOdinPosition Position)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::SetPosition);

    OdinPosition pos = Position;
    auto         ret = odin_encoder_set_position(this->GetHandle(), ChannelMask.GetChannelMask(), &pos);
    ODIN_LOG(Verbose, TEXT("Set Position Called with ChannelMask %llu and position %s"), ChannelMask.GetChannelMask(), *Position.ToString());
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        return true;
    } else {
        FOdinModule::LogErrorCode("Aborting SetPositions due to invalid odin_encoder_set_position call: %s", ret);
    }

    return false;
}

bool UOdinEncoder::ClearPosition(FOdinChannelMask ChannelMask)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::ClearPosition);

    auto ret = odin_encoder_clear_position(this->GetHandle(), ChannelMask.GetChannelMask());
    ODIN_LOG(Verbose, "Clear Position called with Channelmask %llu", ChannelMask.GetChannelMask());
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        return true;
    } else {
        FOdinModule::LogErrorCode("Aborting ClearPosition due to invalid odin_encoder_clear_position call: %s", ret);
    }

    return false;
}

int32 UOdinEncoder::Pop(TArray<uint8>& Datagram)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::Pop);

    uint32_t size = Datagram.Num();
    auto     ret  = odin_encoder_pop(this->GetHandle(), Datagram.GetData(), &size);
    if (ret == OdinError::ODIN_ERROR_SUCCESS || ret == OdinError::ODIN_ERROR_NO_DATA) {
        return size;
    } else {
        FOdinModule::LogErrorCode("Aborting Pop due to invalid odin_encoder_pop call: %s", ret);
    }

    return 0;
}

int32 UOdinEncoder::Push(float* Samples, int32 Count)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::Push);

    auto ret = odin_encoder_push(this->GetHandle(), Samples, Count);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        return Count;
    } else {
        FOdinModule::LogErrorCode("Aborting Push due to invalid odin_encoder_push call: %s", ret);
    }

    return 0;
}

void UOdinEncoder::SetHandle(OdinEncoder* handle)
{
    if (nullptr == handle) {
        if (IsValid(Handle)) {
            Handle->SetHandle(nullptr);
        }
        return;
    }

    this->Handle = NewObject<UOdinHandle>();
    this->Handle->SetHandle(handle);
}

int32 UOdinEncoder::Push(TArray<float> Samples)
{
    return Push(Samples.GetData(), Samples.Num());
}

void UOdinEncoder::SetEchoCancellationProcessingDelay(int32 DelayInMs)
{
    if (DelayInMs < 0) {
        ODIN_LOG(Warning, "Tried Setting Echo Cancellation Processing Delay to a value smaller than 0, this is not allowed.");
        return;
    }
    if (SubmixListener.IsValid()) {
        SubmixListener->SetDelay(DelayInMs);
    }
}

void UOdinEncoder::OnPipelineApmConfigChanged(UOdinPipeline* AffectedPipeline, int32 EffectId, FOdinApmConfig NewApmConfig)
{
    ODIN_LOG(Verbose, "Encoder Pipeline Apm Config Changed.");
    if (AffectedPipeline && SubmixListener.IsValid()) {
        if (NewApmConfig.echo_canceller) {
            SubmixListener->AddEffectId(EffectId);

        } else {
            SubmixListener->RemoveEffectId(EffectId);
        }
    }
}

FOdinSubmixListener::FOdinSubmixListener()
{
    AudioDeviceCreatedCallbackHandle   = FAudioDeviceManagerDelegates::OnAudioDeviceCreated.AddRaw(this, &FOdinSubmixListener::OnAudioDeviceCreated);
    AudioDeviceDestroyedCallbackHandle = FAudioDeviceManagerDelegates::OnAudioDeviceDestroyed.AddRaw(this, &FOdinSubmixListener::OnAudioDeviceDestroyed);
}

FOdinSubmixListener::~FOdinSubmixListener()
{
    ODIN_LOG(Log, "Odin Submix Listener destroyed.");
    FAudioDeviceManagerDelegates::OnAudioDeviceCreated.Remove(AudioDeviceCreatedCallbackHandle);
    FAudioDeviceManagerDelegates::OnAudioDeviceDestroyed.Remove(AudioDeviceDestroyedCallbackHandle);
}

void FOdinSubmixListener::OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate,
                                            double AudioClock)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinSubmixListener::OnNewSubmixBuffer);
    if (bIsListening && PipelineHandle.IsValid()) {
        const OdinPipeline* OdinPipeline = PipelineHandle->GetHandle();

        TArray<uint32> EffectIds;
        {
            FScopeLock EffectAccessLock(&EffectIdAccessSection);
            EffectIds = ApmEffectIds;
        }
        for (const uint32 EffectId : EffectIds) {
            ODIN_LOG(VeryVerbose, "odin_pipeline_update_apm_playback called for EffectId %d, Num Samples %d, Delay %d", EffectId, NumSamples, DelayMs.load());
            TRACE_CPUPROFILER_EVENT_SCOPE(FOdinSubmixListener::OnNewSubmixBuffer - odin_pipeline_update_apm_playback);
            OdinError Result = odin_pipeline_update_apm_playback(OdinPipeline, EffectId, AudioData, NumSamples, DelayMs);
            if (Result != OdinError::ODIN_ERROR_SUCCESS) {
                ODIN_LOG(Error, "odin_pipeline_update_apm_playback failed, reason: %s",
                         *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(Result), false));
            }
        }
    }
}

void FOdinSubmixListener::SetPipelineHandle(UOdinPipeline* NewHandle)
{
    if (NewHandle) {
        PipelineHandle = NewHandle;
    }
}

void FOdinSubmixListener::AttachToSubmix()
{
    if (!FAudioDevice::GetAudioDeviceManager()) {
        ODIN_LOG(Log, "FOdinSubmixListener::AttachToSubmix failed, could not retrieve audio device manager");
        return;
    }

    if (bIsListening) {
        ODIN_LOG(Log, "FOdinSubmixListener::AttachToSubmix skipped, already listening.");
        return;
    }

    if (FAudioDeviceHandle AudioDevice = FAudioDevice::GetAudioDeviceManager()->GetActiveAudioDevice(); AudioDevice.IsValid()) {
        bIsListening = true;
        ListenTargetId.Reset();
        ListenTargetId = MakeShared<Audio::DeviceID>(AudioDevice.GetDeviceID());

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
        USoundSubmix* ConnectedSubmix = &AudioDevice->GetMainSubmixObject();
        AudioDevice->RegisterSubmixBufferListener(AsShared(), *ConnectedSubmix);
#else
        AudioDevice->RegisterSubmixBufferListener(this);
#endif

        ODIN_LOG(Log, "FOdinSubmixListener::AttachToSubmix Successfully started listening to submix");
    }
}

void FOdinSubmixListener::AddEffectId(const uint32 EffectId)
{
    int32 NumApmEffects;
    {
        FScopeLock EffectAccessLock(&EffectIdAccessSection);
        if (!ApmEffectIds.Contains(EffectId)) {
            ODIN_LOG(Log, "Added effect id %d", EffectId);
            ApmEffectIds.AddUnique(EffectId);
        }
        NumApmEffects = ApmEffectIds.Num();
    }
    if (NumApmEffects > 0 && !bIsListening) {
        AttachToSubmix();
    }
}

void FOdinSubmixListener::DetachFromSubmix()
{
    if (!ListenTargetId.IsValid() || !bIsListening) {
        ODIN_LOG(Verbose, "FOdinSubmixListener::DetachFromSubmix skipped, not listening currently.");
        return;
    }

    if (!FAudioDevice::GetAudioDeviceManager()) {
        ODIN_LOG(Log, "FOdinSubmixListener::DetachFromSubmix failed, could not retrieve audio device manager");
        return;
    }

    if (FAudioDeviceHandle AudioDevice = FAudioDevice::GetAudioDeviceManager()->GetAudioDevice(*ListenTargetId); AudioDevice.IsValid()) {

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
        USoundSubmix* ConnectedSubmix = &AudioDevice->GetMainSubmixObject();
        if (!ConnectedSubmix) {
            UE_LOG(Odin, Error, TEXT("UOdinSubmixListener: StopSubmixListener failed, Connected Submix is invalid."));
            return;
        }
        AudioDevice->UnregisterSubmixBufferListener(AsShared(), *ConnectedSubmix);
#else
        AudioDevice->UnregisterSubmixBufferListener(this);
#endif

        bIsListening = false;
        ListenTargetId.Reset();
        ODIN_LOG(Log, "FOdinSubmixListener::DetachFromSubmix Successfully detached from listening to submix");
    } else {
        ODIN_LOG(Warning, "FOdinSubmixListener::DetachFromSubmix failed, could not retrieve audio device with stored device id.");
    }
}

void FOdinSubmixListener::RemoveEffectId(const uint32 EffectId)
{
    int32 NumApmEffects;
    {
        FScopeLock EffectAccessLock(&EffectIdAccessSection);
        int32      NumRemovedEffects = ApmEffectIds.Remove(EffectId);
        if (NumRemovedEffects > 0) {
            ODIN_LOG(Log, "Removed effect id %d", EffectId);
        }
        NumApmEffects = ApmEffectIds.Num();
    }
    if (NumApmEffects < 1 && bIsListening) {
        DetachFromSubmix();
    }
}

void FOdinSubmixListener::SetDelay(int32 NewDelayInMs)
{
    DelayMs = NewDelayInMs;
}

int32 FOdinSubmixListener::GetNumEffectsRegistered() const
{
    FScopeLock EffectAccessLock(&EffectIdAccessSection);
    return ApmEffectIds.Num();
}

void FOdinSubmixListener::OnAudioDeviceCreated(Audio::FDeviceId Id)
{
    ODIN_LOG(Log, "Audio Device Created with id %lu", Id);
    if (FAudioDeviceManager* AudioDeviceManager = FAudioDevice::GetAudioDeviceManager()) {
        if (FAudioDeviceHandle ActiveAudioDevice = AudioDeviceManager->GetActiveAudioDevice()) {
            if (ListenTargetId.IsValid() && *ListenTargetId != ActiveAudioDevice.GetDeviceID()) {
                DetachFromSubmix();
                AttachToSubmix();
            }
        }
    }
}

void FOdinSubmixListener::OnAudioDeviceDestroyed(Audio::FDeviceId Id)
{
    ODIN_LOG(Log, "Audio Device Destroyed with id %lu", Id);
    if (ListenTargetId.IsValid() && *ListenTargetId == Id) {
        DetachFromSubmix();
        ListenTargetId.Reset();
        AttachToSubmix();
    }
}