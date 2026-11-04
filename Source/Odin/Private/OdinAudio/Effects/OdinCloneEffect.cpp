/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinCloneEffect.h"
#include "Async/TaskGraphInterfaces.h"
#include "OdinVoice.h"

UOdinCloneEffect::UOdinCloneEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{ UserData = TOdinCustomEffectUserData(this); }

void UOdinCloneEffect::BeginDestroy()
{
    // removes the effect from the pipeline first, so the audio thread no longer touches the buffers
    Super::BeginDestroy();
    FScopeLock Lock(&PendingSection);
    PendingSamples.Empty();
    RecycledSamples.Empty();
}

UOdinCloneEffect *UOdinCloneEffect::ConstructCloneEffect(UObject *WorldContextObject)
{
    UOdinCloneEffect *result = NewObject<UOdinCloneEffect>(WorldContextObject);
    return result;
}

void UOdinCloneEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent, TOdinCustomEffectUserData<UOdinCustomEffect> *const InUserData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCloneEffect::CustomEffect);

    if (GetParent().IsValid() == false)
        return;

    if (bSkipSilence && *bIsSilent)
        return;

    if (auto effect = InUserData->Root.Get()) {
        DispatchClone(static_cast<UOdinCloneEffect *>(effect), InSamples, ENamedThreads::GameThread);
    }
}

void UOdinCloneEffect::Callback(Audio::TSampleBuffer<float> Buffer) const
{
    if (OnDispatchCloneCallbackBP.IsBound())
        OnDispatchCloneCallbackBP.Broadcast(TArray<float>(Buffer.GetArrayView()));
}

void UOdinCloneEffect::DispatchClone(UOdinCloneEffect *Self, const TArrayView<float> &Samples, ENamedThreads::Type Target) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCloneEffect::CustomEffect - Pipeline Thread);

    // this runs on an audio thread: only copy, the callbacks run on the target thread
    const int32 MaxSamples      = FMath::Max(0, FMath::FloorToInt32(MaxBufferedSeconds * PipelineSampleRate)) * PipelineChannels;
    const int32 IntervalSamples = FMath::Min(FMath::Max(0, FMath::FloorToInt32(Interval * PipelineSampleRate)) * PipelineChannels, MaxSamples);
    bool        bQueueDelivery  = false;
    {
        FScopeLock Lock(&Self->PendingSection);
        if (Self->PendingSamples.Num() + Samples.Num() > MaxSamples) {
            Self->DroppedSamples += Samples.Num();
        } else {
            Self->PendingSamples.Append(Samples.GetData(), Samples.Num());
        }
        if (!Self->bDeliveryQueued && !Self->PendingSamples.IsEmpty() && Self->PendingSamples.Num() >= IntervalSamples) {
            Self->bDeliveryQueued = true;
            bQueueDelivery        = true;
        }
    }

    if (bQueueDelivery) {
        TWeakObjectPtr<UOdinCloneEffect> WeakSelf = Self;
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [WeakSelf]() {
                TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCloneEffect::CustomEffect - Game Thread);
                if (UOdinCloneEffect *Effect = WeakSelf.Get()) {
                    Effect->Deliver();
                }
            },
            TStatId(), nullptr, Target);
    }
}

void UOdinCloneEffect::Flush()
{ Deliver(); }

void UOdinCloneEffect::Deliver()
{
    TArray<float> Batch;
    {
        FScopeLock Lock(&PendingSection);
        Batch          = MoveTemp(PendingSamples);
        PendingSamples = MoveTemp(RecycledSamples);
        PendingSamples.Reset();
        bDeliveryQueued = false;
    }

    if (const int64 Dropped = DroppedSamples.exchange(0)) {
        ODIN_LOG(Warning, "Clone effect dropped %lld samples: the game thread did not keep up within MaxBufferedSeconds (%f).", Dropped, MaxBufferedSeconds);
    }

    if (!Batch.IsEmpty()) {
        Callback(Audio::TSampleBuffer<float>(Batch.GetData(), Batch.Num(), PipelineChannels, PipelineSampleRate));
    }

    // keep the allocation for the next batch
    FScopeLock Lock(&PendingSection);
    RecycledSamples = MoveTemp(Batch);
}
