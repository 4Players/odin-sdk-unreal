/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinSynthComponent.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

#include "Odin.h"
#include "OdinMediaSoundGenerator.h"

bool UOdinSynthComponent::Init(int32 &SampleRate)
{
    NumChannels = 2;
    // We reset the stream handle here, to avoid any kind of delays after re-enabling
    ResetOdinStream(StreamHandle);
    return true;
}

void UOdinSynthComponent::BeginDestroy()
{
    this->playback_media_ = nullptr;
    Super::BeginDestroy();
}

void UOdinSynthComponent::OnRegister()
{
    Super::OnRegister();
    if (nullptr != playback_media_ && 0 != StreamHandle) {
        Start();
    }
}

int32 UOdinSynthComponent::OnGenerateAudio(float *OutAudio, int32 NumSamples)
{
    if (StreamHandle == 0) {
        return 0;
    }

    auto read = odin_audio_read_data(StreamHandle, OutAudio, NumSamples);
    if (odin_is_error(read)) {
        return NumSamples;
    }
    for (IAudioBufferListener *AudioBufferListener : AudioBufferListeners) {
        AudioBufferListener->OnGeneratedBuffer(OutAudio, NumSamples, NumChannels);
    }
    return read;
}

void UOdinSynthComponent::SetOdinStream(OdinMediaStreamHandle NewStreamHandle)
{
    ResetOdinStream(NewStreamHandle);
    this->StreamHandle = NewStreamHandle;
}

void UOdinSynthComponent::ResetOdinStream(OdinMediaStreamHandle HandleToReset)
{
    if (0 != HandleToReset) {
        odin_audio_reset(HandleToReset);
    }
}

void UOdinSynthComponent::Odin_AssignSynthToMedia(UPARAM(ref) UOdinPlaybackMedia *&media)
{
    if (nullptr != media) {
        this->playback_media_ = media;
        SetOdinStream(media->GetMediaHandle());
    } else {
        UE_LOG(Odin, Error,
               TEXT("UOdinSynthComponent::Odin_AssignSynthToMedia: Tried to assign null media to "
                    "synth component on actor %s"),
               *GetOwner()->GetName());
    }
}

void UOdinSynthComponent::Reset()
{
    if (this->playback_media_ != nullptr) {
        ResetOdinStream(this->playback_media_->GetMediaHandle());
    }
}

void UOdinSynthComponent::AdjustAttenuation(const FSoundAttenuationSettings &InAttenuationSettings)
{
    bAllowSpatialization = true;
    Deactivate();

    bOverrideAttenuation = true;
    AttenuationOverrides = InAttenuationSettings;

    auto AudioComponentPointer = GetAudioComponent();
    if (AudioComponentPointer) {
        AudioComponentPointer->AdjustAttenuation(InAttenuationSettings);
    }

    Activate(true);
}

void UOdinSynthComponent::AddAudioBufferListener(IAudioBufferListener *InAudioBufferListener)
{
    AudioBufferListeners.AddUnique(InAudioBufferListener);
}

void UOdinSynthComponent::RemoveAudioBufferListener(IAudioBufferListener *InAudioBufferListener)
{
    AudioBufferListeners.Remove(InAudioBufferListener);
}