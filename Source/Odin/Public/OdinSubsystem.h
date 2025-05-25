/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "Odin.h"
#include "OdinAudioPushDataThread.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OdinSubsystem.generated.h"

/**
 * UOdinSubsystem is a subclass of UGameInstanceSubsystem responsible for initializing
 * and deinitializing the native Odin Library with the correct sample rate and channel count.
 *
 * Handles pushing capture data to odin in a seperate Thread.
 */
UCLASS()
class ODIN_API UOdinSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
  public:
    /**
     * Returns the sample rate configured for the Odin Native Library.
     */
    int32 GetSampleRate() const;
    /**
     * Returns the number of audio channels configured for the Odin Native Library.
     */
    int32 GetChannelCount() const;

    /**
     * Verifies if the Odin library has been successfully initialized.
     *
     * This function checks the internal state of the UOdinSubsystem instance to determine
     * whether the Odin Native Library is currently initialized and operational.
     *
     * @return True if the Odin library is initialized, false otherwise.
     */
    bool IsOdinInitialized() const;

    /**
     * Pushes audio sample data to the Odin audio processing thread for the specified media stream.
     *
     * This method forwards audio data to the internal thread responsible for processing
     * and transmitting audio to the Odin network.
     *
     * @param MediaHandle The handle identifying the media stream to which the audio data belongs.
     * @param AudioData A pointer to an array containing the floating-point audio sample data.
     * @param NumSamples The number of audio samples present in the provided data array.
     */
    void PushAudio(OdinMediaStreamHandle MediaHandle, const float* AudioData, int32 NumSamples);

  protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

  private:
    TUniquePtr<FOdinAudioPushDataThread> PushDataThread;

    int32 SampleRate    = ODIN_DEFAULT_SAMPLE_RATE;
    int32 ChannelCount  = ODIN_DEFAULT_CHANNEL_COUNT;
    bool  IsInitialized = false;
};
