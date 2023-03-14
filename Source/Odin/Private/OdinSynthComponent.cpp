/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#include "OdinSynthComponent.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

#include "OdinMediaSoundGenerator.h"

bool UOdinSynthComponent::Init(int32 &SampleRate)
{
    NumChannels = 2;
    SampleRate  = 48000;
    return true;
}

void UOdinSynthComponent::BeginDestroy()
{
    if (this->sound_generator_) {
        this->sound_generator_->SetOdinStream(0);
    }

    this->sound_generator_ = nullptr;
    this->playback_media_  = nullptr;

    Super::BeginDestroy();
}

void UOdinSynthComponent::Odin_AssignSynthToMedia(UOdinPlaybackMedia *media)
{
    this->playback_media_ = media;
    if (sound_generator_) {
        sound_generator_->SetOdinStream(media->GetMediaHandle());
    }
}

void UOdinSynthComponent::AdjustAttenuation(const FSoundAttenuationSettings &InAttenuationSettings)
{
    bAllowSpatialization = true;
    Deactivate();

    bOverrideAttenuation = true;
    AttenuationOverrides = InAttenuationSettings;

    auto audioComponent = GetAudioComponent();
    if (audioComponent) {
        audioComponent->AdjustAttenuation(InAttenuationSettings);
    }

    Activate(true);
}

#if ENGINE_MAJOR_VERSION >= 5
ISoundGeneratorPtr
UOdinSynthComponent::CreateSoundGenerator(const FSoundGeneratorInitParams &InParams)
#else
ISoundGeneratorPtr UOdinSynthComponent::CreateSoundGenerator(int32 InSampleRate,
                                                             int32 InNumChannels)
#endif
{
    // TODO: use information from UE5 InParams if needed
    // int32 InSampleRate, InNumChannels, NumFramesPerCallback
    // uint64 InstanceID
    // FString GraphName
    // bool bIsPreviewSound
    this->sound_generator_ = MakeShared<OdinMediaSoundGenerator, ESPMode::ThreadSafe>();
    if (this->playback_media_ != 0) {
        sound_generator_->SetOdinStream(this->playback_media_->GetMediaHandle());
    }
    return sound_generator_;
}
