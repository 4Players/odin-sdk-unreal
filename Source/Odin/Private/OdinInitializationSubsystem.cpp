// Fill out your copyright notice in the Description page of Project Settings.

#include "OdinInitializationSubsystem.h"

#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "Odin.h"
#include "odin_sdk.h"

void UOdinInitializationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bool                 bStartupSuccess = false;
    FAudioDeviceManager* DeviceManager   = FAudioDeviceManager::Get();
    if (DeviceManager) {
        FAudioDeviceHandle AudioDevice = DeviceManager->GetActiveAudioDevice();
        SampleRate                     = AudioDevice->SampleRate;
        ChannelCount                   = 2;

        UE_LOG(Odin, Log, TEXT("Odin initialization with sample rate %d and channel count %d."),
               SampleRate, ChannelCount);
        bStartupSuccess = odin_startup_ex(
            ODIN_VERSION, OdinAudioStreamConfig{(uint32_t)SampleRate, (uint8_t)ChannelCount});
    }

    if (!bStartupSuccess) {
        UE_LOG(Odin, Warning, TEXT("Odin Startup aborted, no Active Audio Device available."))
    }
}

void UOdinInitializationSubsystem::Deinitialize()
{
    Super::Deinitialize();
    odin_shutdown();
}

int32 UOdinInitializationSubsystem::GetSampleRate() const
{
    return SampleRate;
}

int32 UOdinInitializationSubsystem::GetChannelCount() const
{
    return ChannelCount;
}