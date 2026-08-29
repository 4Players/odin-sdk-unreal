/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinRmsEffect.h"

#include "OdinVoice.h"

UOdinRmsEffect::UOdinRmsEffect(const FObjectInitializer &PCIP)
    : Super(PCIP)
{
    UserData = TOdinCustomEffectUserData(this);
    RmsDbfs  = 0;
}

void UOdinRmsEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent, TOdinCustomEffectUserData<UOdinCustomEffect> *const InUserData) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinRmsEffect::CustomEffect);

    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    const int32 NumSamples = InSamples.Num();
    if (NumSamples <= 0)
        return;

    // 20.0 * (f32::sqrt(squared_sum / Block::LENGTH as f32)).max(f32::MIN_POSITIVE).log10()
    float squaredSum = 0.0f;
    for (const float Sample : InSamples)
        squaredSum += Sample * Sample;

    const float rms = FMath::Sqrt(squaredSum / static_cast<float>(NumSamples));

    if (auto effect = InUserData->Root.Get()) {
        auto self = static_cast<UOdinRmsEffect *>(effect);

        self->RmsDbfs = rms > SMALL_NUMBER ? 20.0f * FMath::LogX(10.0f, rms) : -144.0f; // effective floor for >1.e-8f silence
    }
}

UOdinRmsEffect *UOdinRmsEffect::ConstructRmsEffect(UObject *WorldContextObject)
{
    UOdinRmsEffect *result = NewObject<UOdinRmsEffect>(WorldContextObject);
    result->RmsDbfs        = 0;
    return result;
}

void UOdinRmsEffect::BeginDestroy()
{ Super::BeginDestroy(); }
