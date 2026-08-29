/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinCustomEffect.h"

#include "OdinRmsEffect.generated.h"

class UOdinPipeline;

/**
 * Codec-Effect for the odin audio pipeline.
 * The effect will calculate Root Mean Square (RMS) averaging with the decibels relative to full scale (dBFS)
 */
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRmsEffect : public UOdinCustomEffect
{
    GENERATED_BODY()

  public:
    UOdinRmsEffect(const FObjectInitializer& PCIP);

    virtual void CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent,
                              TOdinCustomEffectUserData<UOdinCustomEffect>* const InUserData) const override;

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct RMS Effect", ToolTip = "Creates a new rms effect to set RMS-dBFS", HidePin = "WorldContextObject",
                          DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline|Effects")
    static UOdinRmsEffect* ConstructRmsEffect(UObject* WorldContextObject);

    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Pipeline|State")
    float                                     RmsDbfs = 0;
    TOdinCustomEffectUserData<UOdinRmsEffect> UserData;

  private:
    virtual void BeginDestroy() override;
};
