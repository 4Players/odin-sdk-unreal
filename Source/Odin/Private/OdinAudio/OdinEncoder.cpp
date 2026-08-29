/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinEncoder.h"

#include "AudioDevice.h"
#include "OdinFunctionLibrary.h"
#include "OdinRoom.h"
#include "OdinSubsystem.h"
#include "OdinVoice.h"
#include "SampleBuffer.h"
#include "OdinAudio/OdinPipeline.h"
#include "Runtime/Launch/Resources/Version.h"

namespace
{
void ConvertChannelsInterleaved(const float* InSamples, const uint32 NumSamples, const int32 InChannels, const int32 OutChannels, TArray<float>& OutSamples)
{
    constexpr float K = 0.70710678f; // -3 dB for center/surround contributions

    const int32 NumFrames = NumSamples / InChannels;
    OutSamples.Reset();
    OutSamples.Reserve(NumFrames * OutChannels);

    for (int32 Frame = 0; Frame < NumFrames; ++Frame) {
        const float* In = InSamples + Frame * InChannels;

        // fold the input frame to stereo first (UE channel order: FL FR FC LFE (BL BR) SL SR)
        float Left, Right;
        if (InChannels == 1) {
            Left = Right = In[0];
        } else if (InChannels == 2) {
            Left  = In[0];
            Right = In[1];
        } else if (InChannels == 6) {
            constexpr float Scale = 1.0f / (1.0f + 2.0f * K);
            Left                  = (In[0] + K * In[2] + K * In[4]) * Scale;
            Right                 = (In[1] + K * In[2] + K * In[5]) * Scale;
        } else if (InChannels == 8) {
            constexpr float Scale = 1.0f / (1.0f + 3.0f * K);
            Left                  = (In[0] + K * In[2] + K * In[4] + K * In[6]) * Scale;
            Right                 = (In[1] + K * In[2] + K * In[5] + K * In[7]) * Scale;
        } else {
            // unknown layout: average alternating channels
            float SumLeft = 0.0f, SumRight = 0.0f;
            int32 NumLeft = 0, NumRight = 0;
            for (int32 Channel = 0; Channel < InChannels; ++Channel) {
                if (Channel & 1) {
                    SumRight += In[Channel];
                    ++NumRight;
                } else {
                    SumLeft += In[Channel];
                    ++NumLeft;
                }
            }
            Left  = NumLeft > 0 ? SumLeft / NumLeft : 0.0f;
            Right = NumRight > 0 ? SumRight / NumRight : 0.0f;
        }

        if (OutChannels == 1) {
            OutSamples.Add(FMath::Clamp(0.5f * (Left + Right), -1.0f, 1.0f));
        } else {
            OutSamples.Add(FMath::Clamp(Left, -1.0f, 1.0f));
            OutSamples.Add(FMath::Clamp(Right, -1.0f, 1.0f));
        }
    }
}
} // namespace

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
        OdinSubsystem->DeregisterEncoder(Handle);
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
{ return odin_encoder_is_silent(this->GetHandle()); }

TArray<UOdinRoom*> UOdinEncoder::GetLinkedRooms() const
{
    TArray<UOdinRoom*> Rooms;
    if (const UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {
        for (const TWeakObjectPtr<UOdinRoom>& Room : OdinSubsystem->GetRoomsForEncoder(this->GetHandle())) {
            if (Room.IsValid()) {
                Rooms.Add(Room.Get());
            }
        }
    }
    return Rooms;
}

bool UOdinEncoder::SetAudioEventHandler(int EFilter)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::SetAudioEventHandler);
    uint64 RegistrationId = 0;
    if (const UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {
        RegistrationId = OdinSubsystem->GetEncoderRegistrationId(this->GetHandle());
    }
    if (RegistrationId == 0) {
        ODIN_LOG(Error, "Aborting SetAudioEventHandler, the encoder handle is not registered.");
        return false;
    }

    const auto Result = odin_encoder_set_event_callback(this->GetHandle(), static_cast<enum OdinAudioEvents>(EFilter), this->OdinEncoderEventCallbackFunc,
                                                        reinterpret_cast<void*>(static_cast<UPTRINT>(RegistrationId)));
    if (Result != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Aborting SetAudioEventHandler due to invalid odin_encoder_set_event_callback call: %s", Result);
        return false;
    }
    return true;
}

void UOdinEncoder::HandleOdinAudioEventCallback(OdinEncoder* EncoderHandle, const OdinAudioEvents Events, const uint64 RegistrationId)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::HandleOdinAudioEventCallback)
    auto filter = static_cast<EOdinAudioEvents>(Events);

    if (RegistrationId == 0) {
        return;
    }

    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [EncoderHandle, RegistrationId, filter]() {
            const UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get();
            if (OdinSubsystem == nullptr) {
                return;
            }
            if (UOdinEncoder* EncoderPtr = OdinSubsystem->GetEncoderByRegistration(EncoderHandle, RegistrationId).Get()) {
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

    if (IsValid(AudioGenerator)) {
        AudioGenerator->RemoveGeneratorDelegate(Audio_Generator_Handle);
    }
    if (!Generator) {
        ODIN_LOG(Error, "UOdinEncoder::SetAudioGenerator - audio generator is null, microphone "
                        "will not work.");
        this->AudioGenerator = nullptr;
        return;
    }

    this->AudioGenerator = Generator;
    int32 OdinSampleRate = SampleRate;
    int32 OdinChannels   = bStereo + 1;

    TWeakObjectPtr<UOdinHandle>     WeakOdinHandle = Handle;
    TWeakObjectPtr<UOdinSubsystem>  SubsystemPtr   = UOdinSubsystem::Get();
    TWeakObjectPtr<UAudioGenerator> WeakGenerator  = Generator;
    // resampler state shared with the capture callback, which runs on the capture thread only
    TSharedPtr<FOdinStreamResampler, ESPMode::ThreadSafe> CaptureResampler = MakeShared<FOdinStreamResampler, ESPMode::ThreadSafe>();

    // Create generator delegate TFunction<void(const float *InAudio, int32 NumSamples)>
    TFunction<void(const float* InAudio, int32 NumSamples)> audioGeneratorHandle = [OdinSampleRate, OdinChannels, WeakOdinHandle, SubsystemPtr, WeakGenerator,
                                                                                    CaptureResampler](const float* InAudio, int32 NumSamples) {
        TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder - Audio Generator Callback);

        // read the generator's current format on every callback instead of the format snapshotted at registration time
        const UAudioGenerator* Generator = WeakGenerator.Get();
        if (Generator == nullptr) {
            return;
        }
        const int32 CaptureSampleRate = Generator->GetSampleRate();
        const int32 CaptureChannels   = Generator->GetNumChannels();
        if (CaptureSampleRate <= 0 || CaptureChannels <= 0) {
            return;
        }

        const float* pbuffer   = InAudio;
        int32        bufferNum = NumSamples;

        ODIN_LOG(VeryVerbose, "Encoder, stream: %d hz %d ch, capture: %d hz %d ch. ue-downmix: %d, ue-resample: %d", OdinSampleRate, OdinChannels,
                 CaptureSampleRate, CaptureChannels, (OdinChannels != CaptureChannels), (OdinSampleRate != CaptureSampleRate));

        // convert channels; buffer must outlive the copy into the push queue below
        TArray<float> ConvertedSamples;
        if (OdinChannels != CaptureChannels) {
            ConvertChannelsInterleaved(InAudio, NumSamples, CaptureChannels, OdinChannels, ConvertedSamples);
            pbuffer   = ConvertedSamples.GetData();
            bufferNum = ConvertedSamples.Num();
        }

        // configure runs on every callback, so a device change during an equal-rate phase still resets the resampler state
        CaptureResampler->Configure(CaptureSampleRate, OdinSampleRate, OdinChannels);
        TArray<float> ResampledSamples;
        if (OdinSampleRate != CaptureSampleRate) {
            CaptureResampler->Process(pbuffer, bufferNum, ResampledSamples);
            pbuffer   = ResampledSamples.GetData();
            bufferNum = ResampledSamples.Num();
            if (bufferNum == 0) {
                return;
            }
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

bool UOdinEncoder::SetPositions(const TArray<FOdinChannelPosition>& Positions)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinEncoder::SetPositions);

    bool bAllSucceeded = true;
    for (const FOdinChannelPosition& Entry : Positions) {
        bAllSucceeded &= SetPosition(Entry.ChannelMask, Entry.Position);
    }
    return bAllSucceeded;
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
            if (UOdinSubsystem* const OdinSubsystem = UOdinSubsystem::Get()) {
                OdinSubsystem->DeregisterEncoder(static_cast<OdinEncoder*>(Handle->GetHandle()));
            }
            Handle->SetHandle(nullptr);
        }
        return;
    }

    if (UOdinSubsystem* const OdinSubsystem = UOdinSubsystem::Get()) {
        if (IsValid(Handle)) {
            OdinSubsystem->DeregisterEncoder(static_cast<OdinEncoder*>(Handle->GetHandle()));
        }
    }

    this->Handle = NewObject<UOdinHandle>();
    this->Handle->SetHandle(handle);
    if (UOdinSubsystem* const OdinSubsystem = UOdinSubsystem::Get()) {
        OdinSubsystem->RegisterEncoder(handle, this);
    }
}

int32 UOdinEncoder::Push(TArray<float> Samples)
{ return Push(Samples.GetData(), Samples.Num()); }

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
            int32 PlaybackSampleRate = 48000;
            bool  bPlaybackStereo    = true;
            if (!AffectedPipeline->GetApmPlaybackFormat(EffectId, PlaybackSampleRate, bPlaybackStereo)) {
                ODIN_LOG(Warning, "Unknown playback format for APM effect id %d, assuming %d hz %s.", EffectId, PlaybackSampleRate,
                         bPlaybackStereo ? TEXT("stereo") : TEXT("mono"));
            }
            SubmixListener->AddEffectId(EffectId, PlaybackSampleRate, bPlaybackStereo);

        } else {
            SubmixListener->RemoveEffectId(EffectId);
        }
    }
}

void FOdinStreamResampler::Configure(const int32 InRate, const int32 OutRate, const int32 InChannels)
{
    if (InRate == InputRate && OutRate == OutputRate && InChannels == NumChannels) {
        return;
    }
    InputRate   = InRate;
    OutputRate  = OutRate;
    NumChannels = InChannels;
    Step        = OutRate > 0 ? static_cast<double>(InRate) / static_cast<double>(OutRate) : 1.0;
    Phase       = 0.0;
    LastFrame.Reset();
    LastFrame.AddZeroed(InChannels);
}

void FOdinStreamResampler::Process(const float* InSamples, const uint32 NumSamples, TArray<float>& OutSamples)
{
    OutSamples.Reset();
    if (NumChannels <= 0 || NumSamples < static_cast<uint32>(NumChannels)) {
        return;
    }
    const int32 InFrames = NumSamples / NumChannels;
    OutSamples.Reserve((static_cast<int32>(InFrames / Step) + 2) * NumChannels);

    double Position = Phase;
    while (true) {
        const int32 Frame0 = FMath::FloorToInt32(Position);
        const int32 Frame1 = Frame0 + 1;
        if (Frame1 >= InFrames) {
            break;
        }
        const float Frac = static_cast<float>(Position - Frame0);
        for (int32 Channel = 0; Channel < NumChannels; ++Channel) {
            const float Sample0 = Frame0 < 0 ? LastFrame[Channel] : InSamples[Frame0 * NumChannels + Channel];
            const float Sample1 = InSamples[Frame1 * NumChannels + Channel];
            OutSamples.Add(FMath::Lerp(Sample0, Sample1, Frac));
        }
        Position += Step;
    }

    Phase = Position - InFrames;
    LastFrame.SetNumUninitialized(NumChannels);
    FMemory::Memcpy(LastFrame.GetData(), InSamples + (InFrames - 1) * NumChannels, NumChannels * sizeof(float));
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
    const OdinPipeline* OdinPipeline = NativePipelineHandle.load();
    if (bIsListening && OdinPipeline != nullptr) {

        {
            FScopeLock EffectAccessLock(&EffectIdAccessSection);
            EffectInfoScratch = ApmEffectIds;
        }
        for (const FApmEffectInfo& EffectInfo : EffectInfoScratch) {
            ODIN_LOG(VeryVerbose, "odin_pipeline_update_apm_playback called for EffectId %d, Num Samples %d, Delay %d", EffectInfo.EffectId, NumSamples,
                     DelayMs.load());
            TRACE_CPUPROFILER_EVENT_SCOPE(FOdinSubmixListener::OnNewSubmixBuffer - odin_pipeline_update_apm_playback);

            const float* PlaybackData    = AudioData;
            uint32       PlaybackSamples = NumSamples;

            if (EffectInfo.PlaybackChannels != NumChannels) {
                ConvertChannelsInterleaved(PlaybackData, PlaybackSamples, NumChannels, EffectInfo.PlaybackChannels, ChannelScratch);
                PlaybackData    = ChannelScratch.GetData();
                PlaybackSamples = ChannelScratch.Num();
            }

            if (EffectInfo.PlaybackSampleRate != SampleRate) {
                FOdinStreamResampler& Resampler = PlaybackResamplers.FindOrAdd(EffectInfo.EffectId);
                Resampler.Configure(SampleRate, EffectInfo.PlaybackSampleRate, EffectInfo.PlaybackChannels);
                Resampler.Process(PlaybackData, PlaybackSamples, ResampleScratch);
                PlaybackData    = ResampleScratch.GetData();
                PlaybackSamples = ResampleScratch.Num();
                if (PlaybackSamples == 0) {
                    continue; // the resampler has not accumulated enough input yet
                }
            }

            OdinError Result = odin_pipeline_update_apm_playback(OdinPipeline, EffectInfo.EffectId, PlaybackData, PlaybackSamples, DelayMs);
            if (Result != OdinError::ODIN_ERROR_SUCCESS) {
                ODIN_LOG(Error, "odin_pipeline_update_apm_playback failed, reason: %s",
                         *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(Result), false));
            }
        }

        for (auto It = PlaybackResamplers.CreateIterator(); It; ++It) {
            const uint32 EffectId = It.Key();
            if (!EffectInfoScratch.ContainsByPredicate([EffectId](const FApmEffectInfo& Info) { return Info.EffectId == EffectId; })) {
                It.RemoveCurrent();
            }
        }
    }
}

void FOdinSubmixListener::SetPipelineHandle(UOdinPipeline* NewHandle)
{
    if (NewHandle) {
        NativePipelineHandle = NewHandle->GetHandle();
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

void FOdinSubmixListener::AddEffectId(const uint32 EffectId, const int32 PlaybackSampleRate, const bool bPlaybackStereo)
{
    FApmEffectInfo EffectInfo;
    EffectInfo.EffectId           = EffectId;
    EffectInfo.PlaybackSampleRate = PlaybackSampleRate;
    EffectInfo.PlaybackChannels   = bPlaybackStereo ? 2 : 1;

    int32 NumApmEffects;
    {
        FScopeLock EffectAccessLock(&EffectIdAccessSection);
        if (!ApmEffectIds.Contains(EffectInfo)) {
            ODIN_LOG(Log, "Added effect id %d with playback format %d hz %d ch", EffectId, PlaybackSampleRate, EffectInfo.PlaybackChannels);
            ApmEffectIds.AddUnique(EffectInfo);
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
        int32      NumRemovedEffects = ApmEffectIds.RemoveAll([EffectId](const FApmEffectInfo& Info) { return Info.EffectId == EffectId; });
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
{ DelayMs = NewDelayInMs; }

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