/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "Components/SynthComponent.h"
#include "CoreMinimal.h"

#include "OdinPlaybackMedia.h"

#include "OdinSynthComponent.generated.h"

class OdinMediaSoundGenerator;

UCLASS(ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinSynthComponent : public USynthComponent
{
    GENERATED_BODY()

  public:
    UFUNCTION(BlueprintCallable, meta = (Category    = "Odin|Sound",
                                         DisplayName = "Assign Odin Synth Component to Media"))
    void Odin_AssignSynthToMedia(UPARAM(ref) UOdinPlaybackMedia *&media);

    /**
     * This function can be used to reset media handle assigned to the targeted ODIN Synth instance.
     * Resetting a media handle will restore it to its default configuration. This operation resets
     * the internal Opus encoder/decoder, ensuring a clean state. Additionally, it clears internal
     * buffers, providing a fresh start.
     */
    UFUNCTION(BlueprintCallable, meta = (Category = "Odin|Sound", DisplayName = "Reset Odin Media"))
    void Reset();

    /**
     * This function is used to modify the Attenuation Settings on the targeted ODIN Synth instance.
     * It is worth noting that Attenuation Settings are only passed to new Active Sounds on start,
     * so modified Attenuation data should be set before sound playback.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Sound")
    void AdjustAttenuation(const FSoundAttenuationSettings &InAttenuationSettings);

  protected:
    bool Init(int32 &SampleRate) override;
    void BeginDestroy() override;

  protected:
#if ENGINE_MAJOR_VERSION >= 5
    virtual ISoundGeneratorPtr
    CreateSoundGenerator(const FSoundGeneratorInitParams &InParams) override;
#else
    virtual ISoundGeneratorPtr CreateSoundGenerator(int32 InSampleRate,
                                                    int32 InNumChannels) override;
#endif

  private:
    UPROPERTY()
    UOdinPlaybackMedia *playback_media_ = nullptr;

    TSharedPtr<OdinMediaSoundGenerator, ESPMode::ThreadSafe> sound_generator_;
};
