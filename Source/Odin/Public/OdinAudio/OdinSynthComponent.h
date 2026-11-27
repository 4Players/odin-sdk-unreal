/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#pragma once

#include "Components/SynthComponent.h"
#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "OdinSynthComponent.generated.h"

class FOdinSoundGenerator;
class UOdinDecoder;

/**
 * Component for playing back audio data received from an Odin Decoder in Unreal.
 * The decoder is drained in real time even while the engine does not pull the audio (game paused, the component stopped or deactivated,
 * the voice virtualized by voice limits or concurrency), so its events keep coming and no stale audio builds up.
 * Plays while the game is paused by default ("Is UI Sound"); turn that off to pause voice with the game. As a UI sound it also keeps
 * playing when the engine stops all game sounds (e.g. on a map change) and ignores the global pitch scale.
 */
UCLASS(ClassGroup = (Odin), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinSynthComponent : public USynthComponent
{
    GENERATED_BODY()
    UOdinSynthComponent(const FObjectInitializer& ObjectInitializer);

    // Called when synth is created
    virtual bool Init(int32& SampleRate) override;

    virtual ISoundGeneratorPtr CreateSoundGenerator(const FSoundGeneratorInitParams& InParams) override;

  public:
    /**
     * This function is used to modify the Attenuation Settings on the targeted ODIN Synth instance.
     * It is worth noting that Attenuation Settings are only passed to new Active Sounds on start,
     * so modified Attenuation data should be set before sound playback.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Sound")
    void AdjustAttenuation(const FSoundAttenuationSettings& InAttenuationSettings);

    /**
     * Retrieves the Odin Decoder instance associated with this component.
     *
     * @return A pointer to the UOdinDecoder instance used by the component.
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Sound")
    UOdinDecoder* GetDecoder() const;

    /**
     * Returns the internal audio component associated with this synth component.
     * @return The connected UAudioComponent.
     */
    UFUNCTION(BlueprintPure, Category = "Sound")
    UAudioComponent* GetConnectedAudioComponent();

    /**
     * Sets the decoder for the Odin Synth Component. If the provided decoder differs
     * from the current one, the component is restarted to apply the new decoder.
     *
     * @param InDecoder Pointer to the Odin Decoder instance to be set.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Sound")
    void SetDecoder(UOdinDecoder* InDecoder);

    /**
     * Pops and discards the decoder audio the engine did not pull, see FOdinSoundGenerator::CatchUpToRealTime. The component calls it
     * every frame, also while the game is paused.
     * @return Number of samples popped and discarded
     */
    int32 CatchUpToRealTime();
    /** CatchUpToRealTime with an explicit FPlatformTime::Seconds timestamp. */
    int32 CatchUpToRealTime(double NowSeconds);

  protected:
    /**
     * Restarts the Odin Synth Component by stopping and then restarting its audio generation.
     * Ensures that the restart operation is performed on the game thread.
     */
    void RestartSynthComponent();

    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    void         CloseSoundGenerator();
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    /**
     * Holds a reference to an Odin Decoder instance for retrieving audio data in the Odin Synth Component.
     */
    UPROPERTY(BlueprintGetter = GetDecoder, BlueprintSetter = SetDecoder, meta = (ExposeOnSpawn = true), Category = "Odin|Sound")
    UOdinDecoder* Decoder;

    /** Created with the decoder, so the decoder is drained even before (or without) the engine playing the sound; guarded by GeneratorSection */
    TSharedPtr<FOdinSoundGenerator, ESPMode::ThreadSafe> OdinSoundGeneratorPtr;
    FThreadSafeCounter                                   CurrentAudioDeviceId = -1;

  private:
    /** Creates the sound generator if needed and hands it the decoder; game thread. */
    void EnsureSoundGenerator();
    void RemoveCatchUpTicker();

    /** The engine creates sound generators on the audio thread */
    FCriticalSection           GeneratorSection;
    FTSTicker::FDelegateHandle CatchUpTickerHandle;

    bool bWasPlayingBeforeUnregister = false;
};