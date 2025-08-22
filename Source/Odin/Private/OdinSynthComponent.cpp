/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinSynthComponent.h"
#include "Engine/Engine.h"
#include "Odin.h"
#include "OdinFunctionLibrary.h"
#include "OdinSubsystem.h"
#include "Engine/GameInstance.h"

UOdinSynthComponent::UOdinSynthComponent(const FObjectInitializer& ObjInitializer)
    : Super(ObjInitializer)
    , SoundGenerator(MakeShared<OdinMediaSoundGenerator, ESPMode::ThreadSafe>())
{
}

bool UOdinSynthComponent::Init(int32& SampleRate)
{
    NumChannels = ODIN_DEFAULT_CHANNEL_COUNT;
    SampleRate  = ODIN_DEFAULT_SAMPLE_RATE;

    if (GetWorld() && GetWorld()->GetGameInstance()) {
        const UOdinSubsystem* OdinInitSubsystem =
            GetWorld()->GetGameInstance()->GetSubsystem<UOdinSubsystem>();
        if (OdinInitSubsystem) {
            NumChannels = OdinInitSubsystem->GetChannelCount();
            SampleRate  = OdinInitSubsystem->GetSampleRate();
        }
    }

    // We reset the stream handle here, to avoid any kind of delays after re-enabling
    if (playback_media_.IsValid())
        ResetOdinStream(playback_media_->GetMediaHandle());
    return true;
}

void UOdinSynthComponent::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(Odin, Verbose, TEXT("UOdinSynthComponent::BeginPlay"));
}

void UOdinSynthComponent::BeginDestroy()
{
    this->playback_media_ = nullptr;
    Super::BeginDestroy();
}

void UOdinSynthComponent::OnRegister()
{
    Super::OnRegister();
    if (playback_media_.IsValid() && 0 != playback_media_->GetMediaHandle()) {
        Start();
    }
}

#if ENGINE_MAJOR_VERSION >= 5
ISoundGeneratorPtr
UOdinSynthComponent::CreateSoundGenerator(const FSoundGeneratorInitParams& InParams)
{
    return SoundGenerator;
}
#else
ISoundGeneratorPtr UOdinSynthComponent::CreateSoundGenerator(int32 InSampleRate,
                                                             int32 InNumChannels)
{
    return SoundGenerator;
}
#endif
void UOdinSynthComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    UE_LOG(Odin, Verbose, TEXT("UOdinSynthComponent::Activate"));
}

void UOdinSynthComponent::Deactivate()
{
    Super::Deactivate();
    UE_LOG(Odin, Verbose, TEXT("UOdinSynthComponent::Deactivate"));
}

void UOdinSynthComponent::NativeOnPreChangePlaybackMedia(UOdinPlaybackMedia* OldMedia,
                                                         UOdinPlaybackMedia* NewMedia)
{
    // Do nothing
}

void UOdinSynthComponent::SetOdinStream(OdinMediaStreamHandle NewStreamHandle)
{
    ResetOdinStream(NewStreamHandle);
}

void UOdinSynthComponent::ResetOdinStream(OdinMediaStreamHandle HandleToReset)
{
    if (0 != HandleToReset) {
        odin_audio_reset(HandleToReset);
    }
}

void UOdinSynthComponent::Odin_AssignSynthToMedia(UPARAM(ref) UOdinPlaybackMedia*& media)
{
    UE_LOG(Odin, Verbose, TEXT("UOdinSynthComponent::Odin_AssignSynthToMedia called"));

    if (nullptr != media) {
        NativeOnPreChangePlaybackMedia(playback_media_.Get(), media);
        this->playback_media_ = media;
        SoundGenerator->SetStreamReader(playback_media_->GetPlaybackStreamReader());

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

    if (auto AudioComponentPointer = GetAudioComponent()) {
        AudioComponentPointer->AdjustAttenuation(InAttenuationSettings);
    }

    Activate(true);
}

UOdinPlaybackMedia* UOdinSynthComponent::GetConnectedPlaybackMedia() const
{
    return playback_media_.Get();
}

void UOdinSynthComponent::AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    SoundGenerator->AddAudioBufferListener(InAudioBufferListener);
}

void UOdinSynthComponent::RemoveAudioBufferListener(IAudioBufferListener* InAudioBufferListener)
{
    SoundGenerator->RemoveAudioBufferListener(InAudioBufferListener);
}