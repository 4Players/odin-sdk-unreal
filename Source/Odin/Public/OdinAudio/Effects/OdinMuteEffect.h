/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */
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
 * While MuteFlag is Set, the effect marks the audio as silent and zeroes the samples, so the following effects in the chain and the
 * decoder output get silence and an encoder stops sending after the current talk spurt.
 * With bClearSamples off only the silent flag is set and the samples pass through the chain unchanged, e.g. for following effects that
 * still need the audio. A decoder plays the samples of silent audio though, and an encoder sends the first muted block of a talk spurt
 * as its end, so this alone does not silence the output.
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
              meta     = (DisplayName = "Construct Mute Effect", ToolTip = "Creates a new Mute effect to silence the audio and set the is_silent flag",
                          HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline|Effects")
    static UOdinMuteEffect* ConstructMuteEffect(UObject*               WorldContextObject,
                                                EOdinMuteEffectOptions Toggle          = EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_UNCHANGED,
                                                bool                   bInClearSamples = true);

    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    EOdinMuteEffectOptions MuteFlag = EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_UNCHANGED;
    /** Zero the samples while muted. Turn off to only set the silent flag and keep the samples for the following effects in the chain. */
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    bool                                       bClearSamples = true;
    TOdinCustomEffectUserData<UOdinMuteEffect> UserData;

  private:
    virtual void BeginDestroy() override;
};