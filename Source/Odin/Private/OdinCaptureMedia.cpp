/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinCaptureMedia.h"
#include "Async/Async.h"
#include "Odin.h"
#include "OdinFunctionLibrary.h"
#include "odin_sdk.h"

UOdinCaptureMedia::UOdinCaptureMedia(const class FObjectInitializer& PCIP)
    : Super(PCIP)
{
}

void UOdinCaptureMedia::SetRoom(UOdinRoom* connected_room)
{
    this->connected_room_ = connected_room;
}

void UOdinCaptureMedia::RemoveRoom()
{
    this->connected_room_ = nullptr;
}

void UOdinCaptureMedia::SetAudioCapture(UAudioCapture* audio_capture)
{
    SetAudioGenerator(audio_capture);
}

void UOdinCaptureMedia::SetAudioGenerator(UAudioGenerator* audioGenerator)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCaptureMedia::SetAudioCapture)

    if (!audioGenerator) {
        UE_LOG(Odin, Error,
               TEXT("UOdinCaptureMedia::SetAudioCapture - audio capture is null, microphone will "
                    "not work."));
        return;
    }

    this->audio_capture_ = audioGenerator;

    if (this->stream_handle_) {
        TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCaptureMedia::SetAudioCapture Odin Create Audio Stream)
        odin_media_stream_destroy(this->stream_handle_);
        this->SetMediaHandle(0);
    }

    if (audio_capture_) {
        stream_sample_rate_  = audio_capture_->GetSampleRate();
        stream_num_channels_ = audio_capture_->GetNumChannels();
    }
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCaptureMedia::SetAudioCapture Odin Create Audio Stream)

        UE_LOG(Odin, Log,
               TEXT("Initializing Audio Capture stream with Sample Rate: %d and Channels: %d"),
               stream_sample_rate_, stream_num_channels_);

        const int32 OdinForcedNumChannels = GetEnableMonoMixing() ? 1 : stream_num_channels_;
        this->stream_handle_              = odin_audio_stream_create(
            OdinAudioStreamConfig{static_cast<uint32_t>(stream_sample_rate_),
                                  static_cast<uint8_t>(OdinForcedNumChannels)});
    }

    TWeakObjectPtr<UOdinCaptureMedia> WeakThisPtr = this;
    if (audio_capture_ && IsValid(audio_capture_)) {
        TWeakObjectPtr<UAudioGenerator> WeakAudioGenerator = audio_capture_;
        // Create generator delegate
        TFunction<void(const float* InAudio, int32 NumSamples)> audioGeneratorDelegate =
            [WeakThisPtr, WeakAudioGenerator](const float* InAudio, int32 NumSamples) {
                if (UOdinCaptureMedia* This = WeakThisPtr.Get()) {
                    if (UAudioGenerator* AudioCapture = WeakAudioGenerator.Get()) {
                        AudioGeneratorCallback(This, AudioCapture, InAudio, NumSamples);
                    }
                }
            };
        FScopeLock lock(&this->capture_generator_delegate_);
        this->audio_generator_handle_ =
            audio_capture_->AddGeneratorDelegate(audioGeneratorDelegate);
    }
}

void UOdinCaptureMedia::Reset()
{
    FScopeLock lock(&this->capture_generator_delegate_);
    if (nullptr != audio_capture_) {
        audio_capture_->RemoveGeneratorDelegate(audio_generator_handle_);
        audio_capture_                = nullptr;
        this->audio_generator_handle_ = {};
    }

    if (this->stream_handle_) {
        odin_media_stream_destroy(this->stream_handle_);
        this->stream_handle_ = 0;
    }
}

OdinReturnCode UOdinCaptureMedia::ResetOdinStream()
{
    FScopeLock lock(&this->capture_generator_delegate_);
    if (nullptr != audio_capture_) {
        this->audio_capture_->RemoveGeneratorDelegate(this->audio_generator_handle_);
    }

    this->audio_generator_handle_ = {};

    if (this->stream_handle_) {
        auto result          = odin_media_stream_destroy(this->stream_handle_);
        this->stream_handle_ = 0;
        return result;
    }
    return 0;
}

float UOdinCaptureMedia::GetVolumeMultiplier() const
{
    return volume_multiplier_;
}

void UOdinCaptureMedia::SetVolumeMultiplier(const float newValue)
{
    this->volume_multiplier_ = FMath::Clamp(newValue, 0.0f, GetMaxVolumeMultiplier());
}

float UOdinCaptureMedia::GetMaxVolumeMultiplier() const
{
    return max_volume_multiplier_;
}

void UOdinCaptureMedia::SetMaxVolumeMultiplier(const float newValue)
{
    this->max_volume_multiplier_ = newValue;
}

bool UOdinCaptureMedia::GetEnableMonoMixing() const
{
    return bEnableMonoMixing;
}

void UOdinCaptureMedia::SetEnableMonoMixing(const bool bShouldEnableMonoMixing)
{
    const bool bOldValue    = this->bEnableMonoMixing;
    this->bEnableMonoMixing = bShouldEnableMonoMixing;
    if (bOldValue != bShouldEnableMonoMixing) {
        UE_LOG(Odin, Verbose,
               TEXT("UOdinCaptureMedia: Reconnecting due to EnableMonoMixing Value switch."))
        Reconnect();
    }
}

void UOdinCaptureMedia::Reconnect()
{
    ReconnectCaptureMedia(this);
}

void UOdinCaptureMedia::BeginDestroy()
{
    Reset();
    delete[] volume_adjusted_audio_;
    volume_adjusted_audio_ = nullptr;
    Super::BeginDestroy();
}

void UOdinCaptureMedia::AudioGeneratorCallback(UOdinCaptureMedia*     Media,
                                               const UAudioGenerator* AudioGenerator,
                                               const float* InAudio, int32 NumSamples)
{
    if (!Media) {
        UE_LOG(
            Odin, Error,
            TEXT("Aborting Audio Generator Callback due to invalid Odin Capture Media reference."));
        return;
    }

    if (!AudioGenerator) {
        UE_LOG(Odin, Error,
               TEXT("Aborting Audio Generator Callback due to invalid AudioGenerator reference."));
        return;
    }

    const int32 StreamSampleRate  = Media->stream_sample_rate_;
    const int32 StreamNumChannels = Media->stream_num_channels_;
    if (StreamSampleRate != AudioGenerator->GetSampleRate()
        || StreamNumChannels != AudioGenerator->GetNumChannels()) {

        if (!Media->bIsBeingReset) {
            UE_LOG(Odin, Display,
                   TEXT("Incompatible sample rate, stream: %d, capture: %d. Restarting "
                        "stream."),
                   StreamSampleRate, AudioGenerator->GetSampleRate());

            ReconnectCaptureMedia(Media);
        }
        return;
    }

    if (Media->stream_handle_) {
        int32 TargetSampleCount =
            Media->GetEnableMonoMixing() ? NumSamples / StreamNumChannels : NumSamples;
        if (Media->volume_adjusted_audio_size_ < TargetSampleCount) {
            delete[] Media->volume_adjusted_audio_;
            Media->volume_adjusted_audio_      = new float[TargetSampleCount];
            Media->volume_adjusted_audio_size_ = TargetSampleCount;
        }

        if (Media->GetEnableMonoMixing()) {
            // Simple mix to mono
            for (int32 i = 0; i < TargetSampleCount; ++i) {
                float MixingResult = 0.0f;
                for (int32 ChannelIndex = 0; ChannelIndex < StreamNumChannels; ++ChannelIndex) {
                    float Sample = InAudio[i * StreamNumChannels + ChannelIndex];
                    MixingResult += Sample;
                }
                Media->volume_adjusted_audio_[i] =
                    (MixingResult / StreamNumChannels) * Media->GetVolumeMultiplierAdjusted();
            }
        } else {
            for (int32 i = 0; i < TargetSampleCount; ++i) {
                Media->volume_adjusted_audio_[i] =
                    InAudio[i] * Media->GetVolumeMultiplierAdjusted();
            }
        }

        odin_audio_push_data(Media->stream_handle_, Media->volume_adjusted_audio_,
                             TargetSampleCount);
    }
}

void UOdinCaptureMedia::ReconnectCaptureMedia(TWeakObjectPtr<UOdinCaptureMedia> CaptureMedia)
{
    if (!CaptureMedia.IsValid()) {
        return;
    }
    if (CaptureMedia->bIsBeingReset) {
        return;
    }
    CaptureMedia->bIsBeingReset = true;

    // Perform stream reset in game thread
    AsyncTask(ENamedThreads::GameThread, [CaptureMedia]() {
        if (!UOdinFunctionLibrary::Check(CaptureMedia,
                                         "UOdinCaptureMedia: HandleInputDeviceChanges")) {
            return;
        }

        UOdinCaptureMedia* OdinCaptureMedia = CaptureMedia.Get();

        if (!OdinCaptureMedia->connected_room_.IsValid()) {
            UE_LOG(Odin, Error,
                   TEXT("Missing connected Room on capture stream when trying to reconnect due to "
                        "Input Device change."));
            return;
        }
        if (!OdinCaptureMedia->audio_capture_) {
            UE_LOG(Odin, Error,
                   TEXT("Missing connected audio capture object on capture stream when trying to "
                        "reconnect due to Input Device change."));
            return;
        }

        auto       capturePointer = OdinCaptureMedia->audio_capture_;
        const auto roomPointer    = OdinCaptureMedia->connected_room_.Get();

        // disconnect current stream from connected room
        roomPointer->UnbindCaptureMedia(OdinCaptureMedia);
        // reset audio capture generator delegate and media stream
        OdinCaptureMedia->ResetOdinStream();
        OdinCaptureMedia->SetAudioGenerator(capturePointer);
        const OdinRoomHandle        room_handle  = roomPointer->RoomHandle();
        const OdinMediaStreamHandle media_handle = OdinCaptureMedia->GetMediaHandle();
        const OdinReturnCode        result       = odin_room_add_media(room_handle, media_handle);
        if (odin_is_error(result)) {
            const FString FormattedError = UOdinFunctionLibrary::FormatError(result, true);
            UE_LOG(Odin, Error,
                   TEXT("Error during media stream reset due to input device changes: %s"),
                   *FormattedError);
        } else {
            roomPointer->BindCaptureMedia(OdinCaptureMedia);
            UE_LOG(Odin, Verbose, TEXT("Binding to New Capture Media."));
        }
        CaptureMedia->bIsBeingReset = false;
    });
}

float UOdinCaptureMedia::GetVolumeMultiplierAdjusted() const
{
    return FMath::Pow(GetVolumeMultiplier(), 3);
}