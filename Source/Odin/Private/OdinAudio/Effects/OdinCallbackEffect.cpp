/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinCallbackEffect.h"

namespace OdinCallbackEffect
{
// duration of one pipeline block
constexpr float BlockSeconds =
    static_cast<float>(UOdinCustomEffect::PipelineBlockSamples) / (UOdinCustomEffect::PipelineSampleRate * UOdinCustomEffect::PipelineChannels);
// queued blocks older than this are stale (e.g. capture was stopped) and not output anymore
constexpr double StaleSeconds = 1.0;
} // namespace OdinCallbackEffect

UOdinCallbackEffect::UOdinCallbackEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

void UOdinCallbackEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent,
                                       TOdinCustomEffectUserData<UOdinCustomEffect> *const InUserData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCallbackEffect::CustomEffect);

    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    if (auto effect = InUserData->Root.Get()) {
        static_cast<UOdinCallbackEffect *>(effect)->ExchangeBlock(InSamples, *bIsSilent);
    }
}

void UOdinCallbackEffect::ExchangeBlock(const TArrayView<float> &Samples, bool &bIsSilent)
{
    using namespace OdinCallbackEffect;

    const int32  MaxDelayBlocks = FMath::Max(1, FMath::FloorToInt32(MaxDelay / BlockSeconds + 0.001f));
    const double Now            = FPlatformTime::Seconds();
    bool         bOutputSilence = true;
    {
        // this runs on an audio thread: only bookkeeping and copies here, ProcessSamples runs on the worker
        FScopeLock Lock(&QueueSection);
        if (Now - LastCallSeconds > StaleSeconds) {
            while (!Queue.IsEmpty()) {
                DropOldestBlock();
            }
            DelayBlocks = 1;
        }
        LastCallSeconds = Now;

        FBlock *Input = AcquireBlock();
        Input->Original.SetNumUninitialized(Samples.Num());
        FMemory::Memcpy(Input->Original.GetData(), Samples.GetData(), Samples.Num() * sizeof(float));
        Input->bOriginalSilent = bIsSilent;
        Input->State           = EBlockState::Pending;
        Queue.Add(Input);

        // MaxDelay may have been lowered
        DelayBlocks = FMath::Min(DelayBlocks, MaxDelayBlocks);
        while (Queue.Num() > DelayBlocks + 1) {
            DropOldestBlock();
        }

        if (Queue.Num() > DelayBlocks) {
            FBlock *Due = Queue[0];
            if (Due->State == EBlockState::Done) {
                FMemory::Memcpy(Samples.GetData(), Due->Processed.GetData(), FMath::Min(Samples.Num(), Due->Processed.Num()) * sizeof(float));
                bIsSilent      = Due->bProcessedSilent;
                bOutputSilence = false;
                DropOldestBlock();
                // remove extra delay while the audio is silent
                if (bIsSilent && DelayBlocks > 1 && !Queue.IsEmpty() && Queue[0]->State == EBlockState::Done && Queue[0]->bProcessedSilent) {
                    DropOldestBlock();
                    --DelayBlocks;
                }
            } else if (Fallback == EOdinCallbackEffectFallback::Original) {
                FMemory::Memcpy(Samples.GetData(), Due->Original.GetData(), FMath::Min(Samples.Num(), Due->Original.Num()) * sizeof(float));
                bIsSilent      = Due->bOriginalSilent;
                bOutputSilence = false;
                DropOldestBlock();
            } else if (DelayBlocks < MaxDelayBlocks) {
                // late: wait one more block for it
                ++DelayBlocks;
            } else {
                // late at MaxDelay: give up on it
                DropOldestBlock();
            }
        }
    }

    if (bOutputSilence) {
        FMemory::Memzero(Samples.GetData(), Samples.Num() * sizeof(float));
        bIsSilent = true;
    }

    Pipe.Launch(TEXT("OdinCallbackEffect"), [this]() { ProcessNextBlock(); });
}

void UOdinCallbackEffect::ProcessNextBlock()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCallbackEffect::ProcessNextBlock);

    FBlock *Block = nullptr;
    {
        FScopeLock Lock(&QueueSection);
        for (FBlock *Queued : Queue) {
            if (Queued->State == EBlockState::Pending) {
                Block = Queued;
                break;
            }
        }
        if (Block == nullptr) {
            return;
        }
        Block->State = EBlockState::Processing;
    }

    // the worker owns Processed while the block is processing; Original does not change while the block is queued
    Block->Processed        = Block->Original;
    Block->bProcessedSilent = Block->bOriginalSilent;
    ProcessSamples(MakeArrayView(Block->Processed), Block->bProcessedSilent);

    FScopeLock Lock(&QueueSection);
    if (Block->State == EBlockState::Dropped) {
        ReleaseBlock(Block);
    } else {
        Block->State = EBlockState::Done;
    }
}

void UOdinCallbackEffect::ProcessSamples(TArrayView<float> Samples, bool &bIsSilent)
{
    FScopeLock Lock(&ProcessSection);
    if (ProcessFunction) {
        ProcessFunction(Samples, bIsSilent);
    }
}

void UOdinCallbackEffect::SetProcessFunction(FProcessFunction InProcessFunction)
{
    FScopeLock Lock(&ProcessSection);
    ProcessFunction = MoveTemp(InProcessFunction);
}

bool UOdinCallbackEffect::WaitUntilProcessed(FTimespan Timeout)
{ return Pipe.WaitUntilEmpty(Timeout); }

float UOdinCallbackEffect::GetDelay() const
{
    FScopeLock Lock(&QueueSection);
    return DelayBlocks * OdinCallbackEffect::BlockSeconds;
}

UOdinCallbackEffect::FBlock *UOdinCallbackEffect::AcquireBlock()
{
    if (!FreeBlocks.IsEmpty()) {
        return FreeBlocks.Pop();
    }
    return Blocks.Add_GetRef(MakeUnique<FBlock>()).Get();
}

void UOdinCallbackEffect::DropOldestBlock()
{
    FBlock *Oldest = Queue[0];
    Queue.RemoveAt(0);
    if (Oldest->State == EBlockState::Processing) {
        // the worker releases it when done
        Oldest->State = EBlockState::Dropped;
    } else {
        ReleaseBlock(Oldest);
    }
}

void UOdinCallbackEffect::ReleaseBlock(FBlock *Block)
{ FreeBlocks.Add(Block); }

UOdinCallbackEffect *UOdinCallbackEffect::ConstructCallbackEffect(UObject *WorldContextObject, float InMaxDelay, EOdinCallbackEffectFallback InFallback)
{
    UOdinCallbackEffect *Result = NewObject<UOdinCallbackEffect>(WorldContextObject);
    Result->MaxDelay            = InMaxDelay;
    Result->Fallback            = InFallback;
    return Result;
}

void UOdinCallbackEffect::BeginDestroy()
{
    // removes the effect from the pipeline first, so no new blocks are queued, then waits for the worker that uses this object
    Super::BeginDestroy();
    Pipe.WaitUntilEmpty();

    FScopeLock Lock(&QueueSection);
    Queue.Empty();
    FreeBlocks.Empty();
    Blocks.Empty();
}
