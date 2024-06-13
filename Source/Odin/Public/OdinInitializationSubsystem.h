// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OdinInitializationSubsystem.generated.h"

/**
 *
 */
UCLASS()
class ODIN_API UOdinInitializationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

  public:
    int32 GetSampleRate() const;
    int32 GetChannelCount() const;

  private:
    int32 SampleRate   = 48000;
    int32 ChannelCount = 2;
};
