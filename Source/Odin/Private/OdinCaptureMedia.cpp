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
    if (!audio_capture) {
        UE_LOG(Odin, Error,
               TEXT("UOdinCaptureMedia::SetAudioCapture - audio capture is null, microphone will "
                    "not work."));
    }

    this->audio_capture_ = audio_capture;

    if (this->stream_handle_) {
        odin_media_stream_destroy(this->stream_handle_);
        this->SetMediaHandle(0);
    }

    if (audio_capture) {
        stream_sample_rate_  = audio_capture->GetSampleRate();
        stream_num_channels_ = audio_capture->GetNumChannels();
    }

    UE_LOG(Odin, Log,
           TEXT("Initializing Audio Capture stream with Sample Rate: %d and Channels: %d"),
           stream_sample_rate_, stream_num_channels_);
    this->stream_handle_ = odin_audio_stream_create(OdinAudioStreamConfig{
        static_cast<uint32_t>(stream_sample_rate_), static_cast<uint8_t>(stream_num_channels_)});

    TWeakObjectPtr<UOdinCaptureMedia> WeakThisPtr = this;
    if (audio_capture && audio_capture->IsValidLowLevel()) {
        // Create generator delegate
        TFunction<void(const float* InAudio, int32 NumSamples)> audioGeneratorDelegate =
            [WeakThisPtr](const float* InAudio, int32 NumSamples) {
                if (UOdinCaptureMedia* This = WeakThisPtr.Get()) {
                    UAudioCapture* AudioCapture = This->audio_capture_;

                    if (!AudioCapture || !AudioCapture->IsValidLowLevel()) {
                        UE_LOG(Odin, Warning,
                               TEXT("Aborting audio generator callback due to LowLevelCheck."));
                        return;
                    }

                    int32 StreamSampleRate  = This->stream_sample_rate_;
                    int32 StreamNumChannels = This->stream_num_channels_;
                    if (StreamSampleRate != AudioCapture->GetSampleRate()
                        || StreamNumChannels != AudioCapture->GetNumChannels()) {
                        UE_LOG(Odin, Display,
                               TEXT("Incompatible sample rate, stream: %d, capture: %d. Restarting "
                                    "stream."),
                               StreamSampleRate, AudioCapture->GetSampleRate());

                        ReconnectCaptureMedia(WeakThisPtr);
                        return;
                    }

                    if (This->stream_handle_) {
                        if (This->volume_adjusted_audio_size_ < NumSamples) {
                            delete[] This->volume_adjusted_audio_;
                            This->volume_adjusted_audio_      = new float[NumSamples];
                            This->volume_adjusted_audio_size_ = NumSamples;
                        }
                        for (int i = 0; i < NumSamples; ++i) {
                            This->volume_adjusted_audio_[i] =
                                InAudio[i] * This->GetVolumeMultiplierAdjusted();
                        }

                        odin_audio_push_data(This->stream_handle_, This->volume_adjusted_audio_,
                                             NumSamples);
                    }
                }
            };
        this->audio_generator_handle_ = audio_capture->AddGeneratorDelegate(audioGeneratorDelegate);
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
    bIsBeingReset = false;

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

void UOdinCaptureMedia::Reconnect()
{
    ReconnectCaptureMedia(this);
}

void UOdinCaptureMedia::BeginDestroy()
{
    Reset();
    delete[] volume_adjusted_audio_;
    Super::BeginDestroy();
}

void UOdinCaptureMedia::ReconnectCaptureMedia(TWeakObjectPtr<UOdinCaptureMedia> CaptureMedia)
{
    if (!CaptureMedia.IsValid()) {
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
        OdinCaptureMedia->SetAudioCapture(capturePointer);
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
    });
}

float UOdinCaptureMedia::GetVolumeMultiplierAdjusted() const
{
    return FMath::Pow(GetVolumeMultiplier(), 3);
}