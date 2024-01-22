/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinCaptureMedia.h"

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

    if (audio_capture && audio_capture->IsValidLowLevel()) {
        // Create generator delegate
        TFunction<void(const float* InAudio, int32 NumSamples)> audioGeneratorDelegate =
            [this](const float* InAudio, int32 NumSamples) {
                if (bIsBeingReset) {
                    return;
                }

                if (nullptr != audio_capture_
                    && (stream_sample_rate_ != audio_capture_->GetSampleRate()
                        || stream_num_channels_ != audio_capture_->GetNumChannels())) {
                    UE_LOG(Odin, Display,
                           TEXT("Incompatible sample rate, stream: %d, capture: %d. Restarting "
                                "stream."),
                           stream_sample_rate_, audio_capture_->GetSampleRate());

                    HandleInputDeviceChanges();
                    return;
                }

                if (this->stream_handle_) {
                    if (volume_adjusted_audio_size_ < NumSamples) {
                        delete[] volume_adjusted_audio_;
                        volume_adjusted_audio_      = new float[NumSamples];
                        volume_adjusted_audio_size_ = NumSamples;
                    }
                    for (int i = 0; i < NumSamples; ++i) {
                        volume_adjusted_audio_[i] = InAudio[i] * GetVolumeMultiplierAdjusted();
                    }

                    odin_audio_push_data(this->stream_handle_, volume_adjusted_audio_, NumSamples);
                }
            };
        this->audio_generator_handle_ = audio_capture->AddGeneratorDelegate(audioGeneratorDelegate);
    }
}

void UOdinCaptureMedia::Reset()
{
    if (nullptr != audio_capture_) {
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

void UOdinCaptureMedia::BeginDestroy()
{
    Reset();
    delete[] volume_adjusted_audio_;
    Super::BeginDestroy();
}

void UOdinCaptureMedia::HandleInputDeviceChanges()
{
    bIsBeingReset = true;

    // Perform stream reset in game thread
    AsyncTask(ENamedThreads::GameThread, [this]() {
        if (!connected_room_.IsValid()) {
            UE_LOG(Odin, Error,
                   TEXT("Missing connected Room on capture stream when trying to reconnect due to "
                        "Input Device change."));
            return;
        }
        if (!audio_capture_) {
            UE_LOG(Odin, Error,
                   TEXT("Missing connected audio capture object on capture stream when trying to "
                        "reconnect due to Input Device change."));
            return;
        }

        auto       capturePointer = audio_capture_;
        const auto roomPointer    = connected_room_.Get();

        // disconnect current stream from connected room
        roomPointer->UnbindCaptureMedia(this);
        // reset audio capture generator delegate and media stream
        this->ResetOdinStream();

        // Create new capture media. Odin_CreateMedia also creates new audio capture generator
        UOdinCaptureMedia* NewCaptureMedia = UOdinFunctionLibrary::Odin_CreateMedia(capturePointer);
        // perform Add Media To Room functionality
        const OdinRoomHandle        room_handle = roomPointer ? roomPointer->RoomHandle() : 0;
        const OdinMediaStreamHandle media_handle =
            NewCaptureMedia ? NewCaptureMedia->GetMediaHandle() : 0;
        const OdinReturnCode result = odin_room_add_media(room_handle, media_handle);
        if (odin_is_error(result)) {
            const FString FormattedError = UOdinFunctionLibrary::FormatError(result, true);
            UE_LOG(Odin, Error,
                   TEXT("Error during media stream reset due to input device changes: %s"),
                   *FormattedError);
        } else {
            roomPointer->BindCaptureMedia(NewCaptureMedia);
            UE_LOG(Odin, Verbose, TEXT("Binding to New Capture Media."));
        }
    });
}

float UOdinCaptureMedia::GetVolumeMultiplierAdjusted() const
{
    return FMath::Pow(GetVolumeMultiplier(), 3);
}
