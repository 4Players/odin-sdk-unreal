/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinCloneEffect.h"
#include "Async/TaskGraphInterfaces.h"

UOdinCloneEffect::UOdinCloneEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{
    UserData = TOdinCustomEffectUserData(this);
}

void UOdinCloneEffect::BeginDestroy()
{
    Super::BeginDestroy();
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
    TWeakObjectPtr<UOdinCloneEffect> WeakSelf = Self;
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        // uses cctor FMemory::Memcpy
        [WeakSelf, buffer = Audio::TSampleBuffer<float>(Samples.GetData(), Samples.Num(), Self->Stereo ? 2 : 1, Self->SampleRate)]() {
            TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCloneEffect::CustomEffect - Game Thread);
            if (WeakSelf.IsValid()) {
                WeakSelf->Callback(buffer);
            }
        },
        TStatId(), nullptr, Target);
}
