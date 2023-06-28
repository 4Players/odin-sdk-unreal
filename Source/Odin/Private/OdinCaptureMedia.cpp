/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinCaptureMedia.h"

#include "Odin.h"
#include "OdinCore/include/odin.h"

UOdinCaptureMedia::UOdinCaptureMedia(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

void UOdinCaptureMedia::SetAudioCapture(UAudioCapture *audio_capture)
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

    auto sample_rate   = 48000;
    auto channel_count = 1;

    if (audio_capture) {
        sample_rate   = audio_capture->GetSampleRate();
        channel_count = audio_capture->GetNumChannels();
    }

    this->stream_handle_ = odin_audio_stream_create(
        OdinAudioStreamConfig{(uint32_t)sample_rate, (uint8_t)channel_count});

    if (audio_capture && audio_capture->IsValidLowLevel()) {
        TFunction<void(const float *InAudio, int32 NumSamples)> fp = [this](const float *InAudio,
                                                                            int32 NumSamples) {
            if (this->stream_handle_) {
                odin_audio_push_data(this->stream_handle_, (float *)InAudio, NumSamples);
            }
        };
        this->audio_generator_handle_ = audio_capture->AddGeneratorDelegate(fp);
    }
}

void UOdinCaptureMedia::Reset()
{
    if (nullptr != audio_capture_) {
        audio_capture_ = nullptr;
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
    if (nullptr != audio_capture_)
        this->audio_capture_->RemoveGeneratorDelegate(this->audio_generator_handle_);

    this->audio_generator_handle_ = {};

    if (this->stream_handle_) {
        auto result          = odin_media_stream_destroy(this->stream_handle_);
        this->stream_handle_ = 0;
        return result;
    }

    return 0;
}

void UOdinCaptureMedia::BeginDestroy()
{
    Reset();
    Super::BeginDestroy();
}
