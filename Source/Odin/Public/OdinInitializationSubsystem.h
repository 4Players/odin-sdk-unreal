﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OdinInitializationSubsystem.generated.h"

/**
 * UOdinInitializationSubsystem is a subclass of UGameInstanceSubsystem responsible for initializing
 * and deinitializing the native Odin Library with the correct sample rate and channel count.
 */
UCLASS()
class ODIN_API UOdinInitializationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Returns the sample rate configured for the Odin Native Library.
     */
  public:
    int32 GetSampleRate() const;
    /**
     * Returns the number of audio channels configured for the Odin Native Library.
     */
    int32 GetChannelCount() const;

  private:
    int32 SampleRate   = 48000;
    int32 ChannelCount = 2;
};
