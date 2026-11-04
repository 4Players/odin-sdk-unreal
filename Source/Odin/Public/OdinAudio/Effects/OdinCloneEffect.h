/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */
#pragma once

#include <atomic>

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
 * The copies are collected into batches of Interval (every block by default) in the pipeline format: interleaved stereo at 48 kHz (see
 * UOdinCustomEffect). On the audio thread the effect only copies, so the callbacks may take their time; it does not change the samples
 * in the chain (UOdinCallbackEffect processes them).
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

    /**
     * Hands the samples collected so far to the callbacks now instead of waiting for the interval, e.g. when push to talk is released.
     * @remarks Game thread only.
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Flush Clone Effect"), Category = "Odin|Audio Pipeline|Effects")
    void Flush();

    /** Seconds of audio per callback; 0 calls back for every block (or for several blocks at once if the game thread is slower). */
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    float Interval = 0.0f;
    /** Most audio held while the game thread does not keep up; blocks beyond it are dropped and the drop is logged as a warning. */
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    float MaxBufferedSeconds = 10.0f;
    /** Skips blocks marked as silent, e.g. by voice activity detection or a mute effect before this one. */
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    bool bSkipSilence = false;

    TOdinCustomEffectUserData<UOdinCloneEffect> UserData;

  protected:
    /** Collects the samples and queues a delivery to the Target thread once the interval is collected; audio thread. */
    virtual void DispatchClone(UOdinCloneEffect* Self, const TArrayView<float>& Samples, ENamedThreads::Type Target) const;

  private:
    virtual void BeginDestroy() override;

    /** Hands the collected samples to Callback. */
    void Deliver();

    /** Guards the sample buffers and the queued flag; the delivery holds it only to swap the arrays. */
    FCriticalSection PendingSection;
    TArray<float>    PendingSamples;
    /** Storage of the last delivered batch, reused as the next pending one so collecting does not allocate on the audio thread. */
    TArray<float>      RecycledSamples;
    bool               bDeliveryQueued = false;
    std::atomic<int64> DroppedSamples{0};
};