/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "Components/SynthComponent.h"
#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

#include "OdinPlaybackMedia.h"

#include "OdinSynthComponent.generated.h"

class OdinMediaSoundGenerator;

/**
 * Component for playing back audio data received from an Odin Media Stream in Unreal.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinSynthComponent : public USynthComponent
{
    GENERATED_BODY()

  public:
    UFUNCTION(BlueprintCallable, meta = (Category    = "Odin|Sound",
                                         DisplayName = "Assign Odin Synth Component to Media"))
    void Odin_AssignSynthToMedia(UPARAM(ref) UOdinPlaybackMedia *&media);

    /**
     * This function can be used to reset the media handle assigned to the targeted ODIN Synth
     * instance. Resetting a media handle will restore it to its default configuration. This
     * operation resets the internal Opus encoder/decoder, ensuring a clean state. Additionally, it
     * clears internal buffers, providing a fresh start.
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

    /**
     * Retrieves the playback media stream pointer that was assigned to this synth component.
     * @return A pointer to the playback media stream that is used to playback audio.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin")
    UOdinPlaybackMedia *GetConnectedPlaybackMedia() const;

    // We want to hide the non-virtual function in USynthComponent here!
    void AddAudioBufferListener(IAudioBufferListener *InAudioBufferListener);
    // We want to hide the non-virtual function in USynthComponent here!
    void RemoveAudioBufferListener(IAudioBufferListener *InAudioBufferListener);

  protected:
    virtual bool Init(int32 &SampleRate) override;

    virtual void BeginDestroy() override;
    virtual void OnRegister() override;

    virtual int32 OnGenerateAudio(float *OutAudio, int32 NumSamples) override;

    void SetOdinStream(OdinMediaStreamHandle NewStreamHandle);
    void ResetOdinStream(OdinMediaStreamHandle HandleToReset);

  private:
    UPROPERTY()
    UOdinPlaybackMedia *playback_media_ = nullptr;

    // TSharedPtr<OdinMediaSoundGenerator, ESPMode::ThreadSafe> sound_generator_;
    TArray<IAudioBufferListener *> AudioBufferListeners;

    OdinMediaStreamHandle StreamHandle = 0;
};
