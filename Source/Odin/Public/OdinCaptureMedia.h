/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "AudioCapture.h"
#include "CoreMinimal.h"
#include "OdinMediaBase.h"
#include "odin_sdk.h"

#include "OdinCaptureMedia.generated.h"

UCLASS(BlueprintType, ClassGroup = Odin)
class ODIN_API UOdinCaptureMedia : public UOdinMediaBase
{
    GENERATED_UCLASS_BODY()
  public:
    /**
     * @brief Gives the capture media access to the connected room. Required for resetting
     * the Odin Media Stream on Device Sample Rate or Channel Count Changes.
     * @param connected_room the odin room the capture media is connected to
     */
    void SetRoom(UOdinRoom* connected_room);

    /**
     * @brief Reset the connected room.
     */
    void RemoveRoom();
    /**
     * @brief Set the audio capture object used for capturing microphone data and setup
     * odin media stream based on the capture object sample rate and channel count.
     * @param audio_capture The audio capture object used for capturing microphone data
     */
    void SetAudioCapture(UAudioCapture* audio_capture);
    /**
     * @brief Reset auf audio capture and media stream
     */
    void Reset();
    /**
     * @brief Resets the audio capture and media stream
     * @return Result of the media stream destruction request
     */
    OdinReturnCode ResetOdinStream();

    /**
     * @brief The capture devices input volume will be increased by this multiplier. The multiplier
     * will be in range of [0, MaxVolumeMultiplier].
     * @return The current volume multiplier
     */
    UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
    float GetVolumeMultiplier() const;
    /**
     * @brief Set the volume multiplier of the capture devices input volume. The new value will be
     * capped to the range [0, MaxVolumeMultiplier].
     * @param newValue The new volume multiplier.
     */
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
    void SetVolumeMultiplier(const float newValue);
    /**
     * @brief Get the current maximum volume multiplier value. The Volume Multiplier will be capped
     * to the range [0, MaxVolumeMultiplier].
     * @return The current max volume Multiplier.
     */
    UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
    float GetMaxVolumeMultiplier() const;
    /**
     * @brief Sets the new maximum volume multiplier. The Volume Multiplier will be capped
     * to the range [0, MaxVolumeMultiplier].
     * @param newValue The new max volume multiplier value.
     */
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
    void SetMaxVolumeMultiplier(const float newValue);

    void Reconnect();

  protected:
    virtual void BeginDestroy() override;

    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Capture")
    UAudioCapture* audio_capture_ = nullptr;

    UPROPERTY(BlueprintGetter = GetVolumeMultiplier, BlueprintSetter = SetVolumeMultiplier,
              Category = "Odin|Audio Capture")
    float volume_multiplier_ = 1.0f;
    UPROPERTY(BlueprintGetter = GetMaxVolumeMultiplier, BlueprintSetter = SetMaxVolumeMultiplier,
              Category = "Odin|Audio Capture")
    float max_volume_multiplier_ = 3.0f;

  private:
    static void ReconnectCaptureMedia(TWeakObjectPtr<UOdinCaptureMedia> CaptureMedia);
    float       GetVolumeMultiplierAdjusted() const;

    FCriticalSection capture_generator_delegate_;

    FAudioGeneratorHandle audio_generator_handle_;

    TWeakObjectPtr<UOdinRoom> connected_room_;

    int32 stream_sample_rate_  = 48000;
    int32 stream_num_channels_ = 1;

    FThreadSafeBool bIsBeingReset = false;

    float* volume_adjusted_audio_      = nullptr;
    int32  volume_adjusted_audio_size_ = 0;
};
