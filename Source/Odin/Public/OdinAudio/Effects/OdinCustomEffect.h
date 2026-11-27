/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */
#pragma once

#include "CoreMinimal.h"
#include "OdinCore/include/odin.h"
#include "UObject/Object.h"

#include "OdinCustomEffect.generated.h"

template <class T> struct ODIN_API TOdinCustomEffectUserData {
  public:
    TOdinCustomEffectUserData()
        : UserData(nullptr) {};

    TOdinCustomEffectUserData(T* obj)
        : TOdinCustomEffectUserData(obj, nullptr) {};

    TOdinCustomEffectUserData(T* obj, void* InUserData)
    {
        Root     = obj;
        UserData = InUserData;
    };
    TWeakObjectPtr<T> Root;
    const void*       UserData;
};

class UOdinPipeline;

/**
 * Codec-Effect base for the odin audio pipeline.
 * The effect does nothing by itself.
 *
 * OdinCustomEffects registered with odin_pipeline_insert_custom_effect
 * are executed in a native library caller thread of that codec pipeline
 * and are blocking pop functions. The corresponding codec push function
 * is not blocked. Heavy computational task in an effect will reduce performance.
 * @remarks The audio pipeline execution context is started on a pop call, not push.
 * @remarks e.g: encoder/decoder odin_encoder_push -> BLOCKING(encoder audio pipeline/effects -> odin_encoder_pop)
 * @remarks Not affected is, including, but not limited to, IO and Event library threads
 * @remarks Threads: effects of an encoder run on the audio push thread, effects of a decoder on the audio render thread (or the mixer of
 * an audio middleware like Wwise or FMOD). A slow or blocking effect stalls the audio and every effect after it in the chain; keep
 * CustomEffect short and hand heavier work to another thread, e.g. with UOdinCallbackEffect.
 * @remarks Format: every call gets one block of 20 ms of interleaved stereo at 48 kHz (PipelineBlockSamples samples), whatever the
 * sample rate and channel layout of the encoder or decoder. An encoder copies mono input to both channels at -3 dB; a decoder converts
 * the block to its output format after the pipeline.
 * @remarks The silent flag does not silence the samples: a decoder plays the samples of a block marked as silent, and an encoder still
 * sends the first silent block of a talk spurt. Zero the samples as well to silence them.
 */
UCLASS(ClassGroup = Odin, Abstract, BlueprintType, Blueprintable)
class ODIN_API UOdinCustomEffect : public UObject
{
    GENERATED_BODY()

  public:
    UOdinCustomEffect(const class FObjectInitializer& ObjectInitializer);
    virtual ~UOdinCustomEffect() = default;

    /**
     * Custom Effect callbacks with buffer or do nothing if `is silent`-data.
     * Override for a more useful implementation. *Care thread usage or dispatch to another!
     * @param InSamples raw f32 sample buffer
     * @param bIsSilent flag if sample buffer is considered silent
     * @param InUserData sending effect object (UOdinCustomEffect *)
     */
    virtual void CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent, TOdinCustomEffectUserData<UOdinCustomEffect>* const InUserData) const;

    inline TWeakObjectPtr<UOdinPipeline> GetParent() const
    { return this->Pipeline; }

    inline void SetParent(TWeakObjectPtr<UOdinPipeline> InPipeline)
    { this->Pipeline = InPipeline; }

    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Detach Effect", ToolTip = "Try to remove this effect from parent pipeline"),
              Category = "Odin|Audio Pipeline")
    bool RemoveSelfFromParent();

    /** Sample rate of the pipeline audio */
    static constexpr int32 PipelineSampleRate = 48000;
    /** Channel count of the pipeline audio, interleaved */
    static constexpr int32 PipelineChannels = 2;
    /** Samples per CustomEffect call: 20 ms of PipelineChannels at PipelineSampleRate */
    static constexpr int32 PipelineBlockSamples = PipelineSampleRate / 50 * PipelineChannels;

    /**
     * Scales a block of pipeline samples by a gain that moves linearly from StartGain to EndGain over the block, the same for every
     * channel of a frame. A gain that jumps between two blocks makes an audible click.
     * @param Samples   interleaved samples in the pipeline format
     * @param StartGain   gain the previous block ended with
     * @param EndGain   gain of the last frame of this block
     */
    static void ApplyGainRamp(const TArrayView<float>& Samples, float StartGain, float EndGain);

    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Pipeline|State")
    int32 Index;
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Pipeline|State")
    int32                                        EffectId;
    TOdinCustomEffectUserData<UOdinCustomEffect> UserData;

    OdinCustomEffectCallback FFICallback = [](float* Samples, uint32_t SamplesCount, bool* bIsSilent, const void* InUserData) {
        if (!InUserData)
            return;

        TOdinCustomEffectUserData<UOdinCustomEffect>* const effectUserData =
            const_cast<TOdinCustomEffectUserData<UOdinCustomEffect>* const>(static_cast<const TOdinCustomEffectUserData<UOdinCustomEffect>* const>(InUserData));
        if (effectUserData && effectUserData->Root.IsValid()) {
            effectUserData->Root->CustomEffect(TArrayView<float>(Samples, SamplesCount), bIsSilent, effectUserData);
        }
    };

  protected:
    TWeakObjectPtr<UOdinPipeline> Pipeline;
    virtual void                  BeginDestroy() override;
};