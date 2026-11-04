/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinCustomEffect.h"
#include "Tasks/Pipe.h"

#include "OdinCallbackEffect.generated.h"

/** What a callback effect outputs for a block that is not processed in time */
UENUM(BlueprintType)
enum class EOdinCallbackEffectFallback : uint8 {
    /** Silence, the delay grows instead (up to MaxDelay): the unprocessed audio is never output, e.g. for a voice changer */
    Silence UMETA(DisplayName = "Silence"),
    /** The unprocessed block, the delay does not grow: no gaps, but the original audio is heard while processing is late */
    Original UMETA(DisplayName = "Original"),
};

/**
 * Codec-Effect for the odin audio pipeline that processes the audio with C++ code on a worker thread and writes the result back into the
 * chain, e.g. a voice changer.
 * Effects run on an audio thread (see UOdinCustomEffect), where slow processing stalls the audio. This effect only copies the block
 * there: ProcessSamples processes it on a worker thread, one block after another and in order, and a later call writes it back. This
 * delays the audio by one block (20 ms) and by more while processing is late, up to MaxDelay.
 * A block that is not processed when it is due is output as set by Fallback. While the audio is silent, extra delay is removed again by
 * dropping silent blocks. The first block after the start (or after the effect was not called for a second) is silence.
 * @remarks Processing is C++ only: override ProcessSamples or set a function with SetProcessFunction. Blueprints cannot run on the worker
 * thread; use UOdinCloneEffect to receive copies of the audio on the game thread.
 *
 * Example with the Switchboard Audio SDK, e.g. a voice changer graph in its Manual engine, which processes buffers on demand
 * (see https://docs.switchboard.audio/engines/manual/ for the engine and graph config and the exact parameter types):
 * @code
 * // EngineId: the engine created once with switchboard::Switchboard::createEngine from a Manual engine config with the voice changer graph
 * CallbackEffect->SetProcessFunction([EngineId, Output = TArray<float>()](TArrayView<float> Samples, bool& bIsSilent) mutable {
 *     Output.SetNumUninitialized(Samples.Num());
 *     switchboard::Switchboard::callAction(EngineId, "process",
 *                                          {{"inputBuffer", Samples.GetData()},
 *                                           {"outputBuffer", Output.GetData()},
 *                                           {"numChannels", static_cast<uint32>(UOdinCustomEffect::PipelineChannels)},
 *                                           {"numFrames", static_cast<uint32>(Samples.Num() / UOdinCustomEffect::PipelineChannels)},
 *                                           {"sampleRate", static_cast<uint32>(UOdinCustomEffect::PipelineSampleRate)},
 *                                           {"isInterleaved", true}});
 *     FMemory::Memcpy(Samples.GetData(), Output.GetData(), Samples.Num() * sizeof(float));
 * });
 * @endcode
 *
 * VoiceMod cannot be used with this effect (EoL as of 2026): the Voicemod SDK extension are end-of-life. The Voicemod
 * Desktop App changes the voice before the audio reaches the game instead: capture its virtual microphone, and switch voices through the
 * app's local WebSocket API. Its latency then adds to the capture, not to the pipeline.
 */
UCLASS(ClassGroup = Odin, BlueprintType)
class ODIN_API UOdinCallbackEffect : public UOdinCustomEffect
{
    GENERATED_BODY()

  public:
    using FProcessFunction = TFunction<void(TArrayView<float> Samples, bool& bIsSilent)>;

    UOdinCallbackEffect(const FObjectInitializer& PCIP);

    virtual void CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent,
                              TOdinCustomEffectUserData<UOdinCustomEffect>* const InUserData) const override;

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Callback Effect", ToolTip = "Creates an effect that processes the audio with C++ code on a worker thread",
                          HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline|Effects")
    static UOdinCallbackEffect* ConstructCallbackEffect(UObject* WorldContextObject, float InMaxDelay = 0.2f,
                                                        EOdinCallbackEffectFallback InFallback = EOdinCallbackEffectFallback::Silence);

    /**
     * Processes one block in place on the worker thread. Blocks arrive one at a time and in order.
     * The default calls the function set with SetProcessFunction, or leaves the block unchanged.
     * @param Samples   one block in the pipeline format (interleaved stereo at 48 kHz)
     * @param bIsSilent   whether the block is marked as silent; may be changed
     * @remarks Has to return: destroying the effect waits for running processing.
     */
    virtual void ProcessSamples(TArrayView<float> Samples, bool& bIsSilent);

    /** Sets the function the default ProcessSamples calls; takes effect with the next block. */
    void SetProcessFunction(FProcessFunction InProcessFunction);

    /**
     * Waits until the blocks handed to the worker are processed.
     * @return false if the timeout passed first
     */
    bool WaitUntilProcessed(FTimespan Timeout = FTimespan::MaxValue());

    /** Current delay of the audio in seconds */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Callback Effect Delay"), Category = "Odin|Audio Pipeline|State")
    float GetDelay() const;

    /** Most delay in seconds; at least one block (0.02). Only grows to it with the Silence fallback. */
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    float MaxDelay = 0.2f;
    /** What is output for a block that is not processed in time */
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Audio Pipeline|State")
    EOdinCallbackEffectFallback Fallback = EOdinCallbackEffectFallback::Silence;

  protected:
    virtual void BeginDestroy() override;

  private:
    enum class EBlockState : uint8 { Pending, Processing, Done, Dropped };

    struct FBlock {
        /** Samples as they entered the effect; not changed while the block is queued */
        TArray<float> Original;
        bool          bOriginalSilent = false;
        /** Result of ProcessSamples, written by the worker */
        TArray<float> Processed;
        bool          bProcessedSilent = false;
        EBlockState   State            = EBlockState::Pending;
    };

    /** Queues a block and writes the due one back; audio thread. */
    void ExchangeBlock(const TArrayView<float>& Samples, bool& bIsSilent);
    /** Processes the oldest pending block; worker thread. */
    void ProcessNextBlock();
    /** Takes a free block or creates one; QueueSection must be held. */
    FBlock* AcquireBlock();
    /** Removes the oldest block from the queue; QueueSection must be held. */
    void DropOldestBlock();
    /** Returns a block that is no longer used; QueueSection must be held. */
    void ReleaseBlock(FBlock* Block);

    /** Guards the blocks, the queue and the delay; held only for short bookkeeping and copies. */
    mutable FCriticalSection   QueueSection;
    TArray<TUniquePtr<FBlock>> Blocks;
    /** Inline storage, so taking and returning blocks does not allocate on the audio thread at the default MaxDelay */
    TArray<FBlock*, TInlineAllocator<16>> FreeBlocks;
    /** Queued blocks, oldest first */
    TArray<FBlock*, TInlineAllocator<16>> Queue;
    /** Blocks the output lags behind the input */
    int32  DelayBlocks     = 1;
    double LastCallSeconds = 0.0;

    FCriticalSection ProcessSection;
    FProcessFunction ProcessFunction;

    UE::Tasks::FPipe Pipe{TEXT("OdinCallbackEffect")};
};
