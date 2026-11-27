/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */
#pragma once

#include <atomic>

#include "OdinCustomEffect.h"

#include "OdinVolumeEffect.generated.h"

class UOdinPipeline;

/**
 * Codec-Effect for the odin audio pipeline.
 * The effect will mutate samples buffer with logarithmic or exponental scale.
 * Provides a simple way for an audio boost functionality.
 * For more complex amplitude checkout dBFS and override the custom effect function.
 * A gain of (nearly) 0 zeroes the samples and marks them as silent. A gain that is not a finite number, like a negative SampleScale with a
 * fractional ScaleExponent, leaves the samples unchanged and logs a warning.
 * A changed gain fades over the next block (20 ms) instead of jumping, which would click: the block after lowering the gain to 0 fades
 * out and is already marked as silent. The first block of the effect starts at the gain.
 */
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinVolumeEffect : public UOdinCustomEffect
{
    GENERATED_BODY()

  public:
    UOdinVolumeEffect(const class FObjectInitializer& PCIP);

    virtual void CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent,
                              TOdinCustomEffectUserData<UOdinCustomEffect>* const InUSerData) const override;

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Volume Effect", ToolTip = "Creates a new Volume effect to change the buffer audio level",
                          HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline|Effects")
    static UOdinVolumeEffect* ConstructVolumeEffect(UObject* WorldContextObject, float scale = 1.0f);

    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    float SampleScale = 2.0;
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    float ScaleExponent = 1.0;

    /** If set, SampleScale is a gain in decibels (e.g. -6 halves the amplitude) instead of a linear factor. */
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    bool VolumeLog10 = false;

    TOdinCustomEffectUserData<UOdinVolumeEffect> UserData;

  private:
    virtual void BeginDestroy() override;

    /** Set while the gain is invalid and that was logged, so the warning is not repeated for every block. */
    mutable std::atomic<bool> bWarnedInvalidGain{false};
    /** Gain the last block ended with, which a changed gain fades from; only used on the thread of the pipeline. */
    mutable float AppliedScale     = 1.0f;
    mutable bool  bHasAppliedScale = false;
};