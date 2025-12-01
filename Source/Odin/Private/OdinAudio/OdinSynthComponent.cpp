// Fill out your copyright notice in the Description page of Project Settings.

#include "OdinAudio/OdinSynthComponent.h"

#include "OdinVoice.h"
#include "OdinAudio/OdinDecoder.h"
#include "OdinAudio/OdinSoundGenerator.h"
#include "Async/Async.h"

UOdinSynthComponent::UOdinSynthComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Decoder(nullptr)
{
}

bool UOdinSynthComponent::Init(int32& SampleRate)
{
    if (Decoder) {
        NumChannels = Decoder->bStereo ? 2 : 1;
        SampleRate  = Decoder->SampleRate;
    } else {
        ODIN_LOG(Log, "UOdinSynthComponent::Init: Decoder was not available during Init");
    }

    return true;
}

ISoundGeneratorPtr UOdinSynthComponent::CreateSoundGenerator(const FSoundGeneratorInitParams& InParams)
{
    ODIN_LOG(Verbose, "UOdinSynthComponent::CreateSoundGenerator was called, audio component id: %d, device id: %d, Instance Id: %llu",
             InParams.AudioComponentId, InParams.AudioDeviceID, InParams.InstanceID);
    if (OdinSoundGeneratorPtr.IsValid()) {
        if (CurrentAudioDeviceId.GetValue() != InParams.AudioDeviceID) {

            OdinSoundGeneratorPtr->Close();
            OdinSoundGeneratorPtr.Reset();
        }
    }

    if (!OdinSoundGeneratorPtr.IsValid()) {
        OdinSoundGeneratorPtr = MakeShared<FOdinSoundGenerator, ESPMode::ThreadSafe>();
        CurrentAudioDeviceId.Set(InParams.AudioDeviceID);
    }
    if (Decoder && OdinSoundGeneratorPtr.IsValid()) {
        OdinSoundGeneratorPtr->SetOdinDecoder(Decoder);
    } else {
        ODIN_LOG(Verbose, "Decoder not valid while trying to Create Sound Generator");
    }
    return OdinSoundGeneratorPtr;
}

UOdinDecoder* UOdinSynthComponent::GetDecoder() const
{
    return Decoder;
}

UAudioComponent* UOdinSynthComponent::GetConnectedAudioComponent()
{
    return GetAudioComponent();
}

void UOdinSynthComponent::SetDecoder(UOdinDecoder* InDecoder)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinSynthComponent::SetDecoder);

    if (InDecoder && InDecoder != Decoder) {
        Decoder = InDecoder;

        if (OdinSoundGeneratorPtr.IsValid()) {
            OdinSoundGeneratorPtr->SetOdinDecoder(Decoder);
        }
        RestartSynthComponent();
    }
}

void UOdinSynthComponent::RestartSynthComponent()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinSynthComponent::RestartSynthComponent);

    if (IsInGameThread()) {
        Stop();
        Start();
    } else {
        TWeakObjectPtr<UOdinSynthComponent> WeakThis = this;
        // Needs to be done in game thread. Ensure it's running in game thread.
        AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
            if (WeakThis.IsValid()) {
                WeakThis->Stop();
                WeakThis->Start();
            }
        });
    }
}

void UOdinSynthComponent::BeginPlay()
{
    Super::BeginPlay();
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

void UOdinSynthComponent::BeginDestroy()
{
    if (OdinSoundGeneratorPtr.IsValid()) {
        OdinSoundGeneratorPtr.Reset();
    }
    Super::BeginDestroy();
    ODIN_LOG(Verbose, "ODIN Destroy: %s", ANSI_TO_TCHAR(__FUNCTION__));
}

void UOdinSynthComponent::OnRegister()
{
    Super::OnRegister();
    if (bWasPlayingBeforeUnregister) {
        Stop();
        Start();
        ODIN_LOG(Verbose, "UOdinSynthComponent: Restarting in OnRegister, most likely after Seamless Server Travel.");
    }
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

void UOdinSynthComponent::OnUnregister()
{
    bWasPlayingBeforeUnregister = IsActive();
    Super::OnUnregister();
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

void UOdinSynthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

void UOdinSynthComponent::AdjustAttenuation(const FSoundAttenuationSettings& InAttenuationSettings)
{
    Deactivate();
    bOverrideAttenuation = true;

    bAllowSpatialization = InAttenuationSettings.bSpatialize;
    AttenuationOverrides = InAttenuationSettings;
    if (const auto AudioComponentPointer = GetConnectedAudioComponent()) {
        AudioComponentPointer->AdjustAttenuation(InAttenuationSettings);
    }

    Activate(true);
}