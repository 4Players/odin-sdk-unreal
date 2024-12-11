/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinSynthComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Odin.h"
#include "OdinFunctionLibrary.h"
#include "OdinInitializationSubsystem.h"
#include "OdinMediaSoundGenerator.h"
#include "Engine/GameInstance.h"

bool UOdinSynthComponent::Init(int32& SampleRate)
{
    NumChannels = 1;
    SampleRate  = 48000;

    if (GetWorld() && GetWorld()->GetGameInstance()) {
        const UOdinInitializationSubsystem* OdinInitSubsystem =
            GetWorld()->GetGameInstance()->GetSubsystem<UOdinInitializationSubsystem>();
        if (OdinInitSubsystem) {
            NumChannels = OdinInitSubsystem->GetChannelCount();
            SampleRate  = OdinInitSubsystem->GetSampleRate();
        }
    }

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

int32 UOdinSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    if (StreamHandle == 0) {
        return 0;
    }

    // Will return the number of read samples, if successful, error code otherwise
    OdinReturnCode ReadResult = odin_audio_read_data(StreamHandle, OutAudio, NumSamples);
    if (odin_is_error(ReadResult)) {
        const FString FormattedError = UOdinFunctionLibrary::FormatError(ReadResult, false);
        UE_LOG(Odin, Verbose,
               TEXT("Error while reading data from Odin in UOdinSynthComponent::OnGenerateAudio, "
                    "Error Message: %s, could be due to media being removed."),
               *FormattedError);
        return 0;
    }

    if (ReadResult > static_cast<uint32>(NumSamples)) {
        UE_LOG(Odin, Verbose,
               TEXT("Error while reading data from Odin in UOdinSynthComponent::OnGenerateAudio, "
                    "number of read samples returned by Odin is larger than requested number of "
                    "samples."));
        return 0;
    }

    for (IAudioBufferListener* AudioBufferListener : AudioBufferListeners) {
        AudioBufferListener->OnGeneratedBuffer(OutAudio, NumSamples, NumChannels);
    }
    return ReadResult;
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

void UOdinSynthComponent::Odin_AssignSynthToMedia(UPARAM(ref) UOdinPlaybackMedia*& media)
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

void UOdinSynthComponent::AdjustAttenuation(const FSoundAttenuationSettings& InAttenuationSettings)
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

UOdinPlaybackMedia* UOdinSynthComponent::GetConnectedPlaybackMedia() const
{
    return playback_media_;
}

void UOdinSynthComponent::AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    AudioBufferListeners.AddUnique(InAudioBufferListener);
}

void UOdinSynthComponent::RemoveAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    AudioBufferListeners.Remove(InAudioBufferListener);
}