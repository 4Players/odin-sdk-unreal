/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinSynthComponent.h"

#include "OdinVoice.h"
#include "OdinAudio/OdinDecoder.h"
#include "OdinAudio/OdinSoundGenerator.h"
#include "Async/Async.h"

UOdinSynthComponent::UOdinSynthComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Decoder(nullptr)
{
    // voice keeps playing while the game is paused
    bIsUISound = true;
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
    FScopeLock Lock(&GeneratorSection);
    // a generator created with the decoder is not bound to an audio device yet
    if (OdinSoundGeneratorPtr.IsValid() && CurrentAudioDeviceId.GetValue() != INDEX_NONE && CurrentAudioDeviceId.GetValue() != InParams.AudioDeviceID) {
        OdinSoundGeneratorPtr->Close();
        OdinSoundGeneratorPtr.Reset();
    }

    if (!OdinSoundGeneratorPtr.IsValid()) {
        OdinSoundGeneratorPtr = MakeShared<FOdinSoundGenerator, ESPMode::ThreadSafe>();
    }
    CurrentAudioDeviceId.Set(InParams.AudioDeviceID);
    if (Decoder && OdinSoundGeneratorPtr.IsValid()) {
        OdinSoundGeneratorPtr->SetOdinDecoder(Decoder);
    } else {
        ODIN_LOG(Verbose, "Decoder not valid while trying to Create Sound Generator");
    }
    return OdinSoundGeneratorPtr;
}

UOdinDecoder* UOdinSynthComponent::GetDecoder() const
{ return Decoder; }

UAudioComponent* UOdinSynthComponent::GetConnectedAudioComponent()
{ return GetAudioComponent(); }

void UOdinSynthComponent::SetDecoder(UOdinDecoder* InDecoder)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinSynthComponent::SetDecoder);

    if (InDecoder != Decoder) {
        Decoder = InDecoder;
        EnsureSoundGenerator();
        RestartSynthComponent();
    }
}

int32 UOdinSynthComponent::CatchUpToRealTime()
{ return CatchUpToRealTime(FPlatformTime::Seconds()); }

int32 UOdinSynthComponent::CatchUpToRealTime(double NowSeconds)
{
    TSharedPtr<FOdinSoundGenerator, ESPMode::ThreadSafe> Generator;
    {
        FScopeLock Lock(&GeneratorSection);
        Generator = OdinSoundGeneratorPtr;
    }
    return Generator.IsValid() ? Generator->CatchUpToRealTime(NowSeconds) : 0;
}

void UOdinSynthComponent::EnsureSoundGenerator()
{
    {
        FScopeLock Lock(&GeneratorSection);
        if (!OdinSoundGeneratorPtr.IsValid() && Decoder) {
            OdinSoundGeneratorPtr = MakeShared<FOdinSoundGenerator, ESPMode::ThreadSafe>();
        }
        if (OdinSoundGeneratorPtr.IsValid()) {
            OdinSoundGeneratorPtr->SetOdinDecoder(Decoder);
        }
    }

    // the core ticker also ticks while the game is paused, unlike the component tick
    if (!CatchUpTickerHandle.IsValid() && Decoder) {
        CatchUpTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float) {
            CatchUpToRealTime();
            return true;
        }));
    }
}

void UOdinSynthComponent::RemoveCatchUpTicker()
{
    if (CatchUpTickerHandle.IsValid()) {
        FTSTicker::GetCoreTicker().RemoveTicker(CatchUpTickerHandle);
        CatchUpTickerHandle.Reset();
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
    // the decoder may have been set without SetDecoder (on spawn)
    if (Decoder) {
        EnsureSoundGenerator();
    }
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

void UOdinSynthComponent::BeginDestroy()
{
    CloseSoundGenerator();
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

void UOdinSynthComponent::CloseSoundGenerator()
{
    RemoveCatchUpTicker();
    FScopeLock Lock(&GeneratorSection);
    if (OdinSoundGeneratorPtr.IsValid()) {
        OdinSoundGeneratorPtr->Close();
        OdinSoundGeneratorPtr.Reset();
    }
}

void UOdinSynthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CloseSoundGenerator();
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