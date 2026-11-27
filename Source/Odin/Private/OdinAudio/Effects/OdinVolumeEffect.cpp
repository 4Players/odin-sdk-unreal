/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinVolumeEffect.h"

#include "OdinVoice.h"

UOdinVolumeEffect::UOdinVolumeEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{ UserData = TOdinCustomEffectUserData(this); }

void UOdinVolumeEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent, TOdinCustomEffectUserData<UOdinCustomEffect> *const InUSerData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinVolumeEffect::CustomEffect);

    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    // decibels to linear gain, or a linear scale raised to the exponent
    float bufferScale = VolumeLog10 ? FMath::Pow(10.0f, SampleScale / 20.0f) : FMath::Pow(SampleScale, ScaleExponent);

    // e.g. a negative scale with a fractional exponent: keep the samples instead of writing NaN into the audio
    if (!FMath::IsFinite(bufferScale)) {
        if (!bWarnedInvalidGain.exchange(true)) {
            ODIN_LOG(Warning, "Volume effect ignores a gain that is not a finite number (SampleScale %f, ScaleExponent %f, VolumeLog10 %d).", SampleScale,
                     ScaleExponent, VolumeLog10 ? 1 : 0);
        }
        bufferScale = 1.0f;
    } else {
        bWarnedInvalidGain = false;
    }

    // gain close to silence: the flag alone does not silence the audio, a decoder plays the samples of silent blocks
    if (FMath::IsNearlyEqual(bufferScale, 0.0f, 0.001f)) {
        bufferScale = 0.0f;
        *bIsSilent  = true;
    }

    // a changed gain fades over this block instead of clicking; the first block starts at the gain
    const float startScale = bHasAppliedScale ? AppliedScale : bufferScale;
    AppliedScale           = bufferScale;
    bHasAppliedScale       = true;

    // silent blocks are scaled too, they are not necessarily zero
    ApplyGainRamp(InSamples, startScale, bufferScale);
}

UOdinVolumeEffect *UOdinVolumeEffect::ConstructVolumeEffect(UObject *WorldContextObject, float scale)
{
    UOdinVolumeEffect *result = NewObject<UOdinVolumeEffect>(WorldContextObject);
    result->SampleScale       = scale;
    return result;
}

void UOdinVolumeEffect::BeginDestroy()
{ Super::BeginDestroy(); }