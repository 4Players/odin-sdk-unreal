/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinVolumeEffect.h"

UOdinVolumeEffect::UOdinVolumeEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{ UserData = TOdinCustomEffectUserData(this); }

void UOdinVolumeEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent, TOdinCustomEffectUserData<UOdinCustomEffect> *const InUSerData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinVolumeEffect::CustomEffect);

    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    // already silent content
    if (*bIsSilent)
        return;

    // decibels to linear gain, or a linear scale raised to the exponent
    float bufferScale = VolumeLog10 ? FMath::Pow(10.0f, SampleScale / 20.0f) : FMath::Pow(SampleScale, ScaleExponent);

    // gain close to silence
    if (FMath::IsNearlyEqual(bufferScale, 0.0f, 0.001f)) {
        *bIsSilent = true;
        return;
    }

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
{ Super::BeginDestroy(); }