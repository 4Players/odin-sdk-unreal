/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
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
    {
        return this->Pipeline;
    }

    inline void SetParent(TWeakObjectPtr<UOdinPipeline> InPipeline)
    {
        this->Pipeline = InPipeline;
    }

    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Detach Effect", ToolTip = "Try to remove this effect from parent pipeline"),
              Category = "Odin|Audio Pipeline")
    bool RemoveSelfFromParent();

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