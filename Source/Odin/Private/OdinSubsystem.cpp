/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinSubsystem.h"
#include "Odin.h"
#include "odin_sdk.h"

void UOdinSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(Odin, Log, TEXT("Odin initializing with Channel Count %d, Sample Rate %d"), ChannelCount,
           SampleRate);
    IsInitialized = odin_startup_ex(
        ODIN_VERSION, OdinAudioStreamConfig{(uint32_t)SampleRate, (uint8_t)ChannelCount});

    if (!IsOdinInitialized()) {
        UE_LOG(Odin, Warning, TEXT("Odin startup failed."));
    }
    if (IsOdinInitialized()) {
        PushDataThread = MakeUnique<FOdinAudioPushDataThread>();
    }
}

void UOdinSubsystem::Deinitialize()
{
    Super::Deinitialize();
    if (IsOdinInitialized()) {
        UE_LOG(Odin, Log, TEXT("Odin Subsystem deinitialized, shutting down Odin."));
        odin_shutdown();
        IsInitialized = false;
    }
    if (PushDataThread.IsValid()) {
        PushDataThread->RequestShutdown();
        PushDataThread.Reset();
    }
}

int32 UOdinSubsystem::GetSampleRate() const
{
    return SampleRate;
}

int32 UOdinSubsystem::GetChannelCount() const
{
    return ChannelCount;
}

bool UOdinSubsystem::IsOdinInitialized() const
{
    return IsInitialized;
}

void UOdinSubsystem::PushAudio(OdinMediaStreamHandle MediaHandle, const float* AudioData,
                               int32 NumSamples)
{
    if (PushDataThread.IsValid()) {
        PushDataThread->PushAudio(MediaHandle, AudioData, NumSamples);
    }
}
