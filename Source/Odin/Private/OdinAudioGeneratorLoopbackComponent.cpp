// Fill out your copyright notice in the Description page of Project Settings.

#include "OdinAudioGeneratorLoopbackComponent.h"

#include "Odin.h"
#include "SampleBuffer.h"
#include "Generators/AudioGenerator.h"

UOdinAudioGeneratorLoopbackComponent::UOdinAudioGeneratorLoopbackComponent(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ConnectedGenerator(nullptr)
    , InitialSampleRate(ODIN_DEFAULT_SAMPLE_RATE)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.05f;
}

void UOdinAudioGeneratorLoopbackComponent::SetLoopbackGenerator(UAudioGenerator* Generator)
{
    if (!Generator) {
        UE_LOG(Odin, Error,
               TEXT("UOdinAudioGeneratorLoopbackComponent::SetLoopbackGenerator: Supplied loopback "
                    "generator pointer is not valid."));
        return;
    }

    // Check if generator handle was previously set and remove delegate if necessary
    RemoveGeneratorDelegate();

    ConnectedGenerator = Generator;

    TWeakObjectPtr<UOdinAudioGeneratorLoopbackComponent> WeakThisPtr = this;
    FOnAudioGenerate GenerateFunction = [WeakThisPtr](const float* AudioData,
                                                      signed int   NumSamples) {
        if (WeakThisPtr.IsValid()) {
            WeakThisPtr->AudioBuffer.Push(AudioData, NumSamples);
        }
    };
    GeneratorHandle = ConnectedGenerator->AddGeneratorDelegate(GenerateFunction);
}

void UOdinAudioGeneratorLoopbackComponent::RemoveGeneratorDelegate()
{
    if (INDEX_NONE != GeneratorHandle.Id && ConnectedGenerator) {
        ConnectedGenerator->RemoveGeneratorDelegate(GeneratorHandle);
    }
    GeneratorHandle    = FAudioGeneratorHandle();
    ConnectedGenerator = nullptr;
}

float UOdinAudioGeneratorLoopbackComponent::GetBufferSize() const
{
    return BufferSize;
}

void UOdinAudioGeneratorLoopbackComponent::SetBufferSize(float NewSize)
{
    BufferSize = NewSize;
    UpdateAudioBufferCapacity();
}

void UOdinAudioGeneratorLoopbackComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (ConnectedGenerator) {
        if (InitialSampleRate != ConnectedGenerator->GetSampleRate()) {
            float PitchMultiplier =
                static_cast<float>(ConnectedGenerator->GetSampleRate()) / InitialSampleRate;
            GetAudioComponent()->SetPitchMultiplier(PitchMultiplier);
        } else if (GetAudioComponent()->PitchMultiplier != 1.0f) {
            GetAudioComponent()->SetPitchMultiplier(1.0f);
        }
    }
}

void UOdinAudioGeneratorLoopbackComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);
    RemoveGeneratorDelegate();
}

bool UOdinAudioGeneratorLoopbackComponent::Init(int32& SampleRate)
{
    // UE_LOG(Odin, Warning, TEXT("Init was called with sample rate %d"), SampleRate);
    NumChannels       = 1; // up mix if necessary
    SampleRate        = ODIN_DEFAULT_SAMPLE_RATE;
    InitialSampleRate = SampleRate;
    UpdateAudioBufferCapacity();
    return Super::Init(SampleRate);
}

int32 UOdinAudioGeneratorLoopbackComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    int32 NumSamplesRead = FMath::Min(AudioBuffer.Num(), static_cast<uint32>(NumSamples));
    if (NumSamplesRead > 0) {
        if (ConnectedGenerator) {
            const int32 GeneratorChannels = ConnectedGenerator->GetNumChannels();
            if (GeneratorChannels != NumChannels) {
                if (TemporaryBuffer.Num() < NumSamplesRead) {

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
                    TemporaryBuffer.SetNumZeroed(NumSamplesRead, EAllowShrinking::No);
#else
                    TemporaryBuffer.SetNumZeroed(NumSamplesRead, false);
#endif
                }

                AudioBuffer.Pop(TemporaryBuffer.GetData(), NumSamplesRead);

                RemixBuffer.Reset();
                RemixBuffer.Append(TemporaryBuffer.GetData(), NumSamplesRead, GeneratorChannels,
                                   ConnectedGenerator->GetSampleRate());
                RemixBuffer.MixBufferToChannels(NumChannels);

                NumSamplesRead =
                    RemixBuffer.GetNumSamples(); // don't forget to update the number of samples has
                                                 // changed after remixing
                FMemory::Memcpy(OutAudio, RemixBuffer.GetData(), NumSamplesRead * sizeof(float));
            } else {
                AudioBuffer.Pop(OutAudio, NumSamplesRead);
            }
        } else {
            NumSamplesRead = 0;
        }
    }
    if (NumSamplesRead > NumSamples) {
        UE_LOG(Odin, Error,
               TEXT("UOdinAudioGeneratorLoopbackComponent::OnGenerateAudio - Something went wrong "
                    "during remix, read too many samples."));
        NumSamplesRead = NumSamples;
    }

    return NumSamplesRead;
}

void UOdinAudioGeneratorLoopbackComponent::UpdateAudioBufferCapacity()
{
    if (InitialSampleRate > 0 && NumChannels > 0 && GetBufferSize() > 0) {
        AudioBuffer.SetCapacity(InitialSampleRate * NumChannels * GetBufferSize());
    } else {
        UE_LOG(Odin, Warning,
               TEXT("UOdinAudioGeneratorLoopbackComponent::UpdateAudioBufferCapacity - Tried "
                    "initializing Audio Buffer Capacity with invalid values, SampleRate: %d, "
                    "NumChannels: %d, BufferSize (seconds): %f. Reverting to default capacity."),
               InitialSampleRate, NumChannels, GetBufferSize());
        AudioBuffer.SetCapacity(ODIN_DEFAULT_SAMPLE_RATE * ODIN_DEFAULT_CHANNEL_COUNT * 3);
    }
}
