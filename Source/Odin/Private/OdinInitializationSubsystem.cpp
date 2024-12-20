/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinInitializationSubsystem.h"
#include "Odin.h"
#include "odin_sdk.h"

void UOdinInitializationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(Odin, Log, TEXT("Odin initializing with Channel Count %d, Sample Rate %d"), ChannelCount,
           SampleRate);
    IsInitialized = odin_startup_ex(
        ODIN_VERSION, OdinAudioStreamConfig{(uint32_t)SampleRate, (uint8_t)ChannelCount});

    if (!IsOdinInitialized()) {
        UE_LOG(Odin, Warning, TEXT("Odin startup failed."));
    }
}

void UOdinInitializationSubsystem::Deinitialize()
{
    Super::Deinitialize();
    if (IsOdinInitialized()) {
        UE_LOG(Odin, Log, TEXT("Odin Subsystem deinitialized, shutting down Odin."));
        odin_shutdown();
        IsInitialized = false;
    }
}

int32 UOdinInitializationSubsystem::GetSampleRate() const
{
    return SampleRate;
}

int32 UOdinInitializationSubsystem::GetChannelCount() const
{
    return ChannelCount;
}

bool UOdinInitializationSubsystem::IsOdinInitialized() const
{
    return IsInitialized;
}