/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinMuteEffect.h"

#include "OdinVoice.h"

UOdinMuteEffect::UOdinMuteEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{
    UserData = TOdinCustomEffectUserData(this);
    MuteFlag = EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_UNCHANGED;
}

void UOdinMuteEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent, TOdinCustomEffectUserData<UOdinCustomEffect> *const InUserData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinMuteEffect::CustomEffect);

    if (MuteFlag == EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_UNCHANGED)
        return;

    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    *bIsSilent = *bIsSilent || MuteFlag == EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_ON;
    if (*bIsSilent) {
        FMemory::Memzero(InSamples.GetData(), InSamples.Num() * sizeof(float));
    }
}

UOdinMuteEffect *UOdinMuteEffect::ConstructMuteEffect(UObject *WorldContextObject, EOdinMuteEffectOptions Toggle)
{
    UOdinMuteEffect *result = NewObject<UOdinMuteEffect>(WorldContextObject);
    result->MuteFlag        = Toggle;
    return result;
}

void UOdinMuteEffect::BeginDestroy()
{ Super::BeginDestroy(); }