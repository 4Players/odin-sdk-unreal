#include "OdinSynthComponent.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

#include "OdinMediaSoundGenerator.h"

bool UOdinSynthComponent::Init(int32 &SampleRate)
{
    NumChannels = 1;
    SampleRate  = 48000;
    return true;
}

void UOdinSynthComponent::BeginDestroy()
{
    if (this->sound_generator_) {
        this->sound_generator_->SetOdinStream(0);
    }

    this->sound_generator_       = nullptr;
    this->pending_stream_handle_ = 0;
    this->playback_media_        = nullptr;

    Super::BeginDestroy();
}

void UOdinSynthComponent::Odin_AssignSynthToMedia(UOdinPlaybackMedia *media)
{
    this->playback_media_ = media;

    if (sound_generator_) {
        sound_generator_->SetOdinStream(media->GetMediaHandle());
    } else {
        pending_stream_handle_ = media->GetMediaHandle();
    }
}

#if ENGINE_MAJOR_VERSION >= 5
ISoundGeneratorPtr
UOdinSynthComponent::CreateSoundGenerator(const FSoundGeneratorInitParams &InParams)
#else
ISoundGeneratorPtr UOdinSynthComponent::CreateSoundGenerator(int32 InSampleRate,
                                                             int32 InNumChannels)
#endif
{
    sound_generator_ = MakeShared<OdinMediaSoundGenerator, ESPMode::ThreadSafe>();
    if (this->pending_stream_handle_) {
        sound_generator_->SetOdinStream(this->pending_stream_handle_);
        this->pending_stream_handle_ = 0;
    }
    return sound_generator_;
}
