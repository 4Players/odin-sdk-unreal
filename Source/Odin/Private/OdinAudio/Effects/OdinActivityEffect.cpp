/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinActivityEffect.h"
#include "Async/TaskGraphInterfaces.h"

UOdinActivityEffect::UOdinActivityEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{
    UserData = TOdinCustomEffectUserData(this);
}

void UOdinActivityEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent,
                                       TOdinCustomEffectUserData<UOdinCustomEffect> *const InUserData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinActivityEffect::CustomEffect);
    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    bool bIsActive = !*bIsSilent;

    if (ActivityCallback.IsBound() && this->HasActivity != bIsActive) {
        TRACE_CPUPROFILER_EVENT_SCOPE(UOdinActivityEffect::CustomEffect - Broadcast Update on Pipeline Thread);
        if (auto effect = InUserData->Root.Get()) {
            auto self         = static_cast<UOdinActivityEffect *>(effect);
            self->HasActivity = bIsActive;
        }

        TWeakObjectPtr<const UOdinActivityEffect> WeakThis = this;
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [WeakThis, bIsActive]() {
                TRACE_CPUPROFILER_EVENT_SCOPE(UOdinActivityEffect::CustomEffect - Broadcast Update on Game Thread);
                if (WeakThis.IsValid()) {
                    WeakThis->ActivityCallback.Broadcast(bIsActive, WeakThis.Get());
                }
            },
            TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

UOdinActivityEffect *UOdinActivityEffect::ConstructActivityEffect(UObject *WorldContextObject)
{
    UOdinActivityEffect *result = NewObject<UOdinActivityEffect>(WorldContextObject);
    return result;
}

void UOdinActivityEffect::BeginDestroy()
{
    ActivityCallback.Clear();
    Super::BeginDestroy();
}