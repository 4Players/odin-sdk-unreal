// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/SynthComponent.h"
#include "CoreMinimal.h"
#include "OdinSynthComponent.generated.h"

class FOdinSoundGenerator;
class UOdinDecoder;

/**
 * Component for playing back audio data received from an Odin Decoder in Unreal.
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

    TSharedPtr<FOdinSoundGenerator, ESPMode::ThreadSafe> OdinSoundGeneratorPtr;
    FThreadSafeCounter                                   CurrentAudioDeviceId = -1;

  private:
    bool bWasPlayingBeforeUnregister = false;
};