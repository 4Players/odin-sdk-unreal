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
        this->sound_generator_->SetOdinStream(nullptr);
    }

    this->sound_generator_ = nullptr;
    this->pending_stream_  = nullptr;
    this->playback_media_  = nullptr;

    Super::BeginDestroy();
}

void UOdinSynthComponent::Odin_AssignSynthToMedia(UOdinPlaybackMedia *media)
{
    this->playback_media_ = media;

    if (sound_generator_) {
        sound_generator_->SetOdinStream(media->GetMedia());
    } else {
        pending_stream_ = media->GetMedia();
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
    if (this->pending_stream_) {
        sound_generator_->SetOdinStream(this->pending_stream_);
        this->pending_stream_ = nullptr;
    }
    return sound_generator_;
}
