/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinCustomEffect.h"

#include "OdinMuteEffect.generated.h"

class UOdinPipeline;

UENUM(BlueprintType)
enum class EOdinMuteEffectOptions : uint8 {
    ODIN_EFFECT_TOGGLE_OFF = 0 UMETA(DisplayName = "Unset"),
    ODIN_EFFECT_TOGGLE_ON        UMETA(DisplayName = "Set"),
    ODIN_EFFECT_TOGGLE_UNCHANGED UMETA(DisplayName = "None"),
};

/**
 * Codec-Effect for the odin audio pipeline.
 * The effect will mutate the silent flag based on the mute uproperty.
 * Sets the flag for chaining effects in the pipeline as well.
 * Symbolizing silenced audio data without changing the buffer.
 * This results in not necessarily sending the audio packet over the network at all.
 */
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinMuteEffect : public UOdinCustomEffect
{
    GENERATED_BODY()

  public:
    UOdinMuteEffect(const FObjectInitializer& PCIP);

    virtual void CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent,
                              TOdinCustomEffectUserData<UOdinCustomEffect>* const InUserData) const override;

    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Construct Mute Effect", ToolTip = "Creates a new Mute effect to set the is_silent flag", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline|Effects")
    static UOdinMuteEffect* ConstructMuteEffect(UObject*               WorldContextObject,
                                                EOdinMuteEffectOptions Toggle = EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_UNCHANGED);

    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    EOdinMuteEffectOptions                     MuteFlag;
    TOdinCustomEffectUserData<UOdinMuteEffect> UserData;

  private:
    virtual void BeginDestroy() override;
};