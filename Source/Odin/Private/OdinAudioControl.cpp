// Fill out your copyright notice in the Description page of Project Settings.

#include "OdinAudioControl.h"

bool IOdinAudioControl::GetIsMuted() const
{
    return false;
}

void IOdinAudioControl::SetIsMuted(bool bNewIsMuted) {}

float IOdinAudioControl::GetVolumeMultiplier() const
{
    return 1.0f;
}

void IOdinAudioControl::SetVolumeMultiplier(float NewMultiplierValue) {}