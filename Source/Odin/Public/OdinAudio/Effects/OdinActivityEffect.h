/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinCustomEffect.h"

#include "OdinActivityEffect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinActivityEffectCallback, const bool, isActive, const UOdinActivityEffect*, userData);

class UOdinPipeline;

/**
 * Codec-Effect for the odin audio pipeline.
 * The effect will invoke activity event and field based on the silent flag.
 * Event is dispatched to the GameThread.
 */
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinActivityEffect : public UOdinCustomEffect
{
    GENERATED_BODY()

  public:
    UOdinActivityEffect(const FObjectInitializer& PCIP);

    virtual void CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent,
                              TOdinCustomEffectUserData<UOdinCustomEffect>* const InUserData) const override;

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Activity Effect", ToolTip = "Creates a new Activity effect", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline|Effects")
    static UOdinActivityEffect* ConstructActivityEffect(UObject* WorldContextObject);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Audio Pipeline|Events")
    FOdinActivityEffectCallback ActivityCallback;
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Pipeline|State")
    bool                                           HasActivity;
    TOdinCustomEffectUserData<UOdinActivityEffect> UserData;

  private:
    virtual void BeginDestroy() override;
};