/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinCustomEffect.h"

#include "OdinVolumeEffect.generated.h"

class UOdinPipeline;

/**
 * Codec-Effect for the odin audio pipeline.
 * The effect will mutate samples buffer with logarithmic or exponental scale.
 * Provides a simple way for an audio boost functionality.
 * For more complex amplitude checkout dBFS and override the custom effect function.
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

    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    bool VolumeLog10 = false;

    TOdinCustomEffectUserData<UOdinVolumeEffect> UserData;

  private:
    virtual void BeginDestroy() override;
};