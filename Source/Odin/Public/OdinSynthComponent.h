/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "Components/SynthComponent.h"
#include "CoreMinimal.h"
#include "odin_sdk.h"
#include "Runtime/Launch/Resources/Version.h"
#include "OdinMediaSoundGenerator.h"
#include "OdinSynthComponent.generated.h"

class UOdinPlaybackMedia;
class OdinMediaSoundGenerator;

/**
 * Component for playing back audio data received from an Odin Media Stream in Unreal.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinSynthComponent : public USynthComponent
{
    GENERATED_BODY()

  public:
    UOdinSynthComponent(const FObjectInitializer& ObjInitializer);

    UFUNCTION(BlueprintCallable, meta = (Category    = "Odin|Sound",
                                         DisplayName = "Assign Odin Synth Component to Media"))
    void Odin_AssignSynthToMedia(UPARAM(ref) UOdinPlaybackMedia*& media);

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
    void AdjustAttenuation(const FSoundAttenuationSettings& InAttenuationSettings);

    /**
     * Retrieves the playback media stream pointer that was assigned to this synth component.
     * @return A pointer to the playback media stream that is used to playback audio.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin")
    UOdinPlaybackMedia* GetConnectedPlaybackMedia() const;

    // We want to hide the non-virtual function in USynthComponent here!
    void AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener);
    // We want to hide the non-virtual function in USynthComponent here!
    void RemoveAudioBufferListener(IAudioBufferListener* InAudioBufferListener);

    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

  protected:
    virtual bool Init(int32& SampleRate) override;

    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;
    virtual void OnRegister() override;

#if ENGINE_MAJOR_VERSION >= 5
    virtual ISoundGeneratorPtr
    CreateSoundGenerator(const FSoundGeneratorInitParams& InParams) override;
#else
    virtual ISoundGeneratorPtr CreateSoundGenerator(int32 InSampleRate,
                                                    int32 InNumChannels) override;
#endif
    /**
     * Called (c++ only) before changing from the OldMedia to the NewMedia Playback Stream.
     * @param OldMedia Media that is replaced by the new media
     * @param NewMedia Media that is replacing the old media
     */
    virtual void NativeOnPreChangePlaybackMedia(UOdinPlaybackMedia* OldMedia,
                                                UOdinPlaybackMedia* NewMedia);

    void SetOdinStream(OdinMediaStreamHandle NewStreamHandle);
    void ResetOdinStream(OdinMediaStreamHandle HandleToReset);

    UPROPERTY()
    TWeakObjectPtr<UOdinPlaybackMedia> playback_media_ = nullptr;

    TSharedRef<OdinMediaSoundGenerator, ESPMode::ThreadSafe> SoundGenerator;

    /**
     * Used to keep track of our read progress on the circular buffer of the playback media. This
     * way multiple Synth Components can playback audio from the same odin media stream without
     * leading to an audio buffer underrun.
     */
    int32 PlaybackMediaReadIndex = 0;
};
