/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

#include "CoreMinimal.h"
#include "OdinNative/OdinNativeHandle.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "UObject/Object.h"

#include "OdinPipeline.generated.h"

class UOdinCloneEffect;
/**
 * A highly dynamic audio processing chain that manages a thread-safe collection of filters like
 * voice activity detection, echo cancellation, noise suppression and even custom effects.
 * This allows sequential processing and real-time modification of audio streams through operations like insertion,
 * removal, reordering and configuration updates.
 */
UCLASS(ClassGroup = Odin, BlueprintType)
class ODIN_API UOdinPipeline : public UObject
{
    GENERATED_BODY()
    friend class UOdinCustomEffect;
    friend class UOdinActivityEffect;
    friend class UOdinMuteEffect;
    friend class UOdinVolumeEffect;

  public:
    UOdinPipeline(const class FObjectInitializer& PCIP);
    static UOdinPipeline* ConstructPipeline(UObject* WorldContextObject, const OdinPipeline* Handle);
    /**
     * Retrieves the total number of effects currently in the audio pipeline.
     * @return effect count
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Effect Count", ToolTip = "Get effect count of the pipeline"), Category = "Odin|Audio Pipeline")
    int32 GetEffectCount() const;
    /**
     * Returns the unique effect identifier from an audio pipeline corresponding to the effect located
     * at the specified index.
     * @param Index index of effect
     * @return effect id
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Effect Id", ToolTip = "Get effect id at the given index of the pipeline"),
              Category = "Odin|Audio Pipeline")
    int32 GetEffectId(int32 Index) const;
    /**
     * Searches the specified audio pipeline for the effect with the specified `effect_id` and returns
     * its current index.
     * @param EffectId id of effect
     * @return effect index
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Effect Index", ToolTip = "Get index of the given effect id in the pipeline"),
              Category = "Odin|Audio Pipeline")
    int32 GetEffectIndex(int32 EffectId) const;
    /**
     * Obtains the effect type (VAD, APM, or Custom) for the effect identified by `effect_id`.
     * @param EffectId id of effect
     * @return effect type
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Effect Type", ToolTip = "Get effect type of the given effect id"), Category = "Odin|Audio Pipeline")
    EOdinEffectType GetEffectType(int32 EffectId) const;
    /**
     * Reorders the audio pipeline by moving the effect with the specified `EffectId` to a new index.
     * @param EffectId Effect for which the index should be changed
     * @param Index new index
     * @return true on success
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Move Effect", ToolTip = "Move an effect with a given index to reorder the pipeline callorder"),
              Category = "Odin|Audio Pipeline")
    bool MoveEffect(int32 EffectId, int32 Index);
    /**
     * Deletes the effect identified by `effect_id` from the specified audio pipeline.
     * @param EffectId id of effect
     * @return true on success
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove Effect", ToolTip = "Removes an effect from the pipeline"), Category = "Odin|Audio Pipeline")
    bool RemoveEffect(int32 EffectId);
    /**
     * Inserts a user-defined custom effect at the specified index in the audio pipeline. The effect
     * is implemented via a callback function and associated user data. A unique effect identifier
     * is returned.
     * @remarks Index is shifting subsequent effects. An index of `0` inserts at the beginning of the pipeline.
     * @param Index desired index
     * @param Effect effect object
     * @return the new effect id or 0
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Insert Custom Effect", ToolTip = "Add a custom effect to the pipline"),
              Category = "Odin|Audio Pipeline")
    int32 InsertCustomEffect(int32 Index, UOdinCustomEffect* Effect);
    /**
     * Inserts a Voice Activity Detection (VAD) effect into the audio pipeline at the specified
     * index and returns a unique effect identifier.
     * @remarks Index is shifting subsequent effects. An index of `0` inserts at the beginning of the pipeline.
     * @param Index desired index
     * @return the new effect id or 0
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Insert VAD Effect", ToolTip = "Add VAD effect and configuration to the pipeline"),
              Category = "Odin|Audio Pipeline")
    int32 InsertVadEffect(int32 Index);
    /**
     * Retrieves the configuration for a VAD effect identified by `effect_id` from the specified
     * audio pipeline.
     * @remarks Index is shifting subsequent effects. An index of `0` inserts at the beginning of the pipeline.
     * @remarks If the effect was disabled the resulting config object is filled with default values/empty
     * @param EffectId effect id
     * @return VAD config
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get VAD Config", ToolTip = "Get the VAD configuration of the given effect id"),
              Category = "Odin|Audio Pipeline")
    FOdinVadConfig GetVadConfig(int32 EffectId) const;
    /**
     * Updates the configuration settings of the VAD effect identified by `effect_id` in the specified
     * audio pipeline.
     * @param EffectId effect id
     * @param Config the config to set
     * @return true on success
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set VAD Config", ToolTip = "Set VAD a new configuration to the VAD effect by id"),
              Category = "Odin|Audio Pipeline")
    bool SetVadConfig(int32 EffectId, FOdinVadConfig Config);

    /**
     * Inserts an APM effect into the pipeline at the specified index.
     *
     * @remarks Index is shifting subsequent effects. An index of `0` inserts at the beginning of the pipeline.
     * @param Index The position at which the APM effect should be inserted in the pipeline.
     * @param SampleRate The playback sample rate for the APM effect.
     * @param bStereo A boolean indicating whether the APM effect should use stereo (true) or mono (false) mode.
     * @return The unique identifier of the inserted APM effect if the operation is successful, or -1 if an error occurs.
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Insert APM Effect", ToolTip = "Add APM effect and configuration to the pipeline", ReturnDisplayName = "Effect Id"),
              Category = "Odin|Audio Pipeline")
    int32 InsertApmEffect(int32 Index, int32 SampleRate = 48000, bool bStereo = false);
    /**
     * Updates the configuration settings of the APM effect identified by `effect_id` in the specified
     * audio pipeline.
     * @param EffectId effect id
     * @param Config the config to set
     * @return true on success
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set APM Config", ToolTip = "Set APM a new  configuration to the APM effect by id"),
              Category = "Odin|Audio Pipeline")
    bool SetApmConfig(int32 EffectId, FOdinApmConfig Config);
    /**
     * Retrieves the configuration for an APM effect identified by `effect_id` from the specified
     * audio pipeline.
     * @remarks if the effect was disabled the resulting config object is filled with default values/empty
     * @param EffectId effect id
     * @return APM config
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get APM Config", ToolTip = "Get the APM config by effect id"), Category = "Odin|Audio Pipeline")
    FOdinApmConfig GetApmConfig(int32 EffectId) const;
    /**
     * Updates the specified APM effect's sample buffer for processing the reverse (playback) audio
     * stream. The provided samples must be interleaved float values in the range [-1, 1]. The delay
     * parameter is used to align the reverse stream processing with the forward (capture) stream.
     * @remarks The delay can be expressed as:
     * @remarks `delay = (t_render - t_analyze) + (t_process - t_capture)`
     * @remarks where:
     * @remarks - `t_render` is the time the first sample of the same frame is rendered by the audio hardware.
     * @remarks - `t_analyze` is the time the frame is processed in the reverse stream.
     * @remarks - `t_capture` is the time the first sample of a frame is captured by the audio hardware.
     * @remarks - `t_process` is the time the frame is processed in the forward stream.
     * @param EffectId effect id
     * @param Samples audio buffer
     * @param Delay delay in milliseconds (ms)
     * @return true on success
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Update APM Playback", ToolTip = "Update playback samples for echo cancellation to the effect by id"),
              Category = "Odin|Audio Pipeline")
    bool UpdateApmPlayback(int32 EffectId, TArray<float>& Samples, int32 Delay = 10);
    /**
     * Updates the specified APM effect's sample buffer for processing the reverse (playback) audio
     * stream. The provided samples must be interleaved float values in the range [-1, 1]. The delay
     * parameter is used to align the reverse stream processing with the forward (capture) stream.
     * The delay can be expressed as:
     *
     *  `delay = (t_render - t_analyze) + (t_process - t_capture)`
     *
     * where:
     *  - `t_render` is the time the first sample of the same frame is rendered by the audio hardware.
     *  - `t_analyze` is the time the frame is processed in the reverse stream.
     *  - `t_capture` is the time the first sample of a frame is captured by the audio hardware.
     *  - `t_process` is the time the frame is processed in the forward stream.
     */
    bool UpdateApmPlayback(int32 EffectId, const float* Samples, int32 Count, int32 Delay = 10);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FApmConfigChangedDelegate, UOdinPipeline*, Pipeline, int32, EffectId, FOdinApmConfig, NewApmConfig);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Audio Pipeline")
    FApmConfigChangedDelegate OnApmConfigChanged;

    // CustomEffects

    /**
     * PRESET: Codec-Effect for the odin audio pipeline.
     * The effect will invoke activity event and field based on the silent flag.
     * Event is dispatched to the GameThread.
     * @param Index desired index
     * @return the custom effect object
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Activity Effect", ToolTip = "Create a custom effect in the pipeline"),
              Category = "Odin|Audio Pipeline|Effects")
    UOdinActivityEffect* CreateActivityEffect(int32 Index);
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Activity Effect", ToolTip = "Create a custom effect in the pipeline"),
              Category = "Odin|Audio Pipeline|Effects")
    UOdinActivityEffect* AppendActivityEffect();
    /**
     * PRESET: Codec-Effect for the odin audio pipeline.
     * The effect will invoke Callback function with a copy of samples buffer
     * and if bound to the delegate, a clone of that buffer.
     * Callback will be called on the GameThread by default.
     * @param Index desired index
     * @return the custom effect object
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Clone Effect", ToolTip = "Create a custom effect in the pipeline"),
              Category = "Odin|Audio Pipeline|Effects")
    UOdinCloneEffect* CreateCloneEffect(int32 Index);
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Clone Effect", ToolTip = "Create a custom effect in the pipeline"),
              Category = "Odin|Audio Pipeline|Effects")
    UOdinCloneEffect* AppendCloneEffect();
    /**
     * PRESET: Codec-Effect for the odin audio pipeline.
     * The effect will mutate the silent flag based on the mute uproperty.
     * Sets the flag for following chaining effects in the pipeline as well.
     * Symbolizing silenced audio data without changing the buffer.
     * This results in not necessarily sending the audio packet over the network at all.
     * @param Index desired index
     * @return the custom effect object
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Mute Effect", ToolTip = "Create a custom effect in the pipeline"),
              Category = "Odin|Audio Pipeline|Effects")
    UOdinMuteEffect* CreateMuteEffect(int32 Index);
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Mute Effect", ToolTip = "Create a custom effect in the pipeline"),
              Category = "Odin|Audio Pipeline|Effects")
    UOdinMuteEffect* AppendMuteEffect();
    /**
     * PRESET: Codec-Effect for the odin audio pipeline.
     * The effect will invoke activity event and field based on the silent flag.
     * Event is dispatched to the GameThread.
     * @param Index desired index
     * @return the custom effect object
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Volume Effect", ToolTip = "Create a custom effect in the pipeline"),
              Category = "Odin|Audio Pipeline|Effects")
    UOdinVolumeEffect* CreateVolumeEffect(int32 Index);
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Volume Effect", ToolTip = "Create a custom effect in the pipeline"),
              Category = "Odin|Audio Pipeline|Effects")
    UOdinVolumeEffect* AppendVolumeEffect();

    inline const OdinPipeline* GetHandle() const
    {
        return reinterpret_cast<const OdinPipeline*>(Handle->GetHandle());
    }

    void SetHandle(const OdinPipeline* NewHandle);

  private:
    UPROPERTY()
    UOdinHandle*                  Handle;
    TWeakObjectPtr<UOdinPipeline> Self = this;
};