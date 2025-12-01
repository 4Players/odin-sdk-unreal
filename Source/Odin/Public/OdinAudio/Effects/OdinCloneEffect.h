/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinCustomEffect.h"
#include "SampleBuffer.h"
#include "OdinCloneEffect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinCloneEffectResponse, FString, response);

class UOdinPipeline;

/**
 * Codec-Effect for the odin audio pipeline.
 * The effect will invoke Callback function with a copy of samples buffer
 * and if bound to the delegate, a clone of that buffer.
 * Callback will be called on the GameThread by default.
 */
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinCloneEffect : public UOdinCustomEffect
{
    GENERATED_BODY()

  public:
    UOdinCloneEffect(const FObjectInitializer& PCIP);
    virtual void CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent,
                              TOdinCustomEffectUserData<UOdinCustomEffect>* const InUserData) const override;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinDispatchCloneCallbackDelegate, const TArray<float>&, SampleData);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Audio Pipeline|Events")
    FOdinDispatchCloneCallbackDelegate OnDispatchCloneCallbackBP;

    /**
     * Callback function on copy of samples in GameThread
     * @param Buffer the buffer copy of samples
     */
    virtual void Callback(Audio::TSampleBuffer<float> Buffer) const;

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Clone Effect", ToolTip = "Creates a effect that dispatch a copy of samples to the game thread.",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline|Effects")
    static UOdinCloneEffect* ConstructCloneEffect(UObject* WorldContextObject);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinCloneEffectResponse PostResponse;

    UPROPERTY(BlueprintReadWrite, Category = "Odin|Effect")
    int32 SampleRate = 48000;
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Effect")
    bool Stereo = false;

    TOdinCustomEffectUserData<UOdinCloneEffect> UserData;

  protected:
    virtual void DispatchClone(UOdinCloneEffect* Self, const TArrayView<float>& Samples, ENamedThreads::Type Target) const;

  private:
    virtual void BeginDestroy() override;
};