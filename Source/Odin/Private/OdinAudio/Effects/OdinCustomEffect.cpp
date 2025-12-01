/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinCustomEffect.h"

#include "OdinVoice.h"
#include "OdinAudio/OdinPipeline.h"

UOdinCustomEffect::UOdinCustomEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    UserData = TOdinCustomEffectUserData(this);
}

void UOdinCustomEffect::CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent, TOdinCustomEffectUserData<UOdinCustomEffect>* const InUserData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinCustomEffect::CustomEffect);
    bool isSilent = *bIsSilent;

    // skip if data is empty
    if (isSilent)
        return;

    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;
}

bool UOdinCustomEffect::RemoveSelfFromParent()
{
    auto pipeline = GetParent();
    if (pipeline.IsValid() && pipeline.IsStale() == false)
        return pipeline->RemoveEffect(this->EffectId);

    return false;
}

void UOdinCustomEffect::BeginDestroy()
{
    ODIN_LOG(Verbose, "ODIN Destroy: %s", ANSI_TO_TCHAR(__FUNCTION__));

    RemoveSelfFromParent();
    Super::BeginDestroy();
    UserData = nullptr;
}