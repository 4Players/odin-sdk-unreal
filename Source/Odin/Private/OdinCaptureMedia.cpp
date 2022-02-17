#include "OdinCaptureMedia.h"

#include "OdinLibrary/include/odin.h"

UOdinCaptureMedia::UOdinCaptureMedia(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

void UOdinCaptureMedia::SetAudioCapture(UAudioCapture *audio_capture)
{
    this->audio_capture_ = audio_capture;

    if (this->stream_) {
        odin_media_stream_destroy(this->stream_);
        this->stream_ = nullptr;
    }

    auto sample_rate   = 48000;
    auto channel_count = 1;

    if (audio_capture) {
        sample_rate   = audio_capture->GetSampleRate();
        channel_count = audio_capture->GetNumChannels();
    }

    this->stream_ = odin_audio_stream_create(
        OdinAudioStreamConfig{(uint32_t)sample_rate, (uint8_t)channel_count});

    TFunction<void(const float *InAudio, int32 NumSamples)> fp = [this](const float *InAudio,
                                                                        int32        NumSamples) {
        if (this->stream_) {
            odin_audio_push_data(this->stream_, (float *)InAudio, NumSamples);
        }
    };

    if (audio_capture) {
        this->audio_generator_handle_ = audio_capture->AddGeneratorDelegate(fp);
    }
}

void UOdinCaptureMedia::Reset()
{
    if (this->audio_capture_) {
        this->audio_capture_->RemoveGeneratorDelegate(this->audio_generator_handle_);
    }

    if (this->stream_) {
        odin_media_stream_destroy(this->stream_);
        this->stream_ = nullptr;
    }
}

void UOdinCaptureMedia::BeginDestroy()
{
    Reset();
    Super::BeginDestroy();
}