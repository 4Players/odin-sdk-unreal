/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinVolumeEffect.h"

UOdinVolumeEffect::UOdinVolumeEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{
    UserData = TOdinCustomEffectUserData(this);
}

void UOdinVolumeEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent, TOdinCustomEffectUserData<UOdinCustomEffect> *const InUSerData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinVolumeEffect::CustomEffect);

    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    // already silent content
    if (*bIsSilent)
        return;

    // scale close to silence
    if (FMath::IsNearlyEqual(SampleScale, 0.0, 0.001)) {
        *bIsSilent = true;
        return;
    }

    // Log10(2.0)*20 || 2.0 ^ 1.0
    float bufferScale = VolumeLog10 ? FMath::LogX(10, SampleScale) * 20.0 : FMath::Pow(SampleScale, ScaleExponent);

    for (int32 i = 0; i < InSamples.Num(); i++)
        InSamples[i] *= bufferScale;
}

UOdinVolumeEffect *UOdinVolumeEffect::ConstructVolumeEffect(UObject *WorldContextObject, float scale)
{
    UOdinVolumeEffect *result = NewObject<UOdinVolumeEffect>(WorldContextObject);
    result->SampleScale       = scale;
    return result;
}

void UOdinVolumeEffect::BeginDestroy()
{
    Super::BeginDestroy();
}