// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OdinAudioControl.generated.h"

// This class does not need to be modified.
UINTERFACE(NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UOdinAudioControl : public UInterface
{
    GENERATED_BODY()
};

/**
 * The IOdinAudioControl interface provides a set of audio control functions.
 * This interface is designed to manage audio settings, such as muting and volume adjustment.
 * Classes implementing this interface are expected to define the behavior for these functions.
 */
class ODIN_API IOdinAudioControl
{
    GENERATED_BODY()

  public:
    /**
     * Checks whether the audio is currently muted.
     *
     * @return Indicates whether the audio is muted (true) or not (false).
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    virtual bool GetIsMuted() const;
    /**
     * Sets the muted state.
     *
     * @param bNewIsMuted Specifies whether to mute (true) or unmute (false) the audio.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    virtual void SetIsMuted(bool bNewIsMuted);

    /**
     * Returns the current volume multiplier for the audio.
     *
     * @return A float value representing the current volume multiplier.
     *         A value of 1.0 represents default volume.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    virtual float GetVolumeMultiplier() const;

    /**
     * Sets the volume multiplier for the audio.
     *
     * @param NewMultiplierValue A float value specifying the new volume multiplier.
     *                           1.0 for normal volume.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    virtual void SetVolumeMultiplier(float NewMultiplierValue);
};