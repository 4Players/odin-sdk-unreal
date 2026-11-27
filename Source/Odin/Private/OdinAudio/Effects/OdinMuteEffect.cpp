/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

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

    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    const bool bMuted = MuteFlag == EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_ON;
    if (bMuted)
        *bIsSilent = true;

    // clearing starts and stops with a fade over one block, cutting the voice off clicks; the first block starts cleared or not
    const float Gain      = bMuted && bClearSamples ? 0.0f : 1.0f;
    const float StartGain = bHasAppliedGain ? AppliedGain : Gain;
    AppliedGain           = Gain;
    bHasAppliedGain       = true;
    ApplyGainRamp(InSamples, StartGain, Gain);
}

UOdinMuteEffect *UOdinMuteEffect::ConstructMuteEffect(UObject *WorldContextObject, EOdinMuteEffectOptions Toggle, bool bInClearSamples)
{
    UOdinMuteEffect *result = NewObject<UOdinMuteEffect>(WorldContextObject);
    result->MuteFlag        = Toggle;
    result->bClearSamples   = bInClearSamples;
    return result;
}

void UOdinMuteEffect::BeginDestroy()
{ Super::BeginDestroy(); }