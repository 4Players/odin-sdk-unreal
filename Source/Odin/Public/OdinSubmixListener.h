/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "Engine/GameEngine.h"
#include "Runtime/Launch/Resources/Version.h"
#include "odin_sdk.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "ISubmixBufferListener.h"
#else
#include "AudioDevice.h"
#endif

#include "OdinSubmixListener.generated.h"

UCLASS(ClassGroup = Utility)
class ODIN_API UOdinSubmixListener : public UObject, public ISubmixBufferListener
{
    GENERATED_BODY()

  public:
    UOdinSubmixListener(const class FObjectInitializer& PCIP);
    virtual ~UOdinSubmixListener();

    void StartSubmixListener();
    void StopSubmixListener();
    void SetRoom(OdinRoomHandle handle);

  protected:
    int32 OdinSampleRate = 48000;
    int32 OdinChannels   = 2;

  private:
    FCriticalSection    submix_cs_;
    bool                bInitialized;
    OdinRoomHandle      current_room_handle;
    OdinResamplerHandle resampler_handle;

    void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 InNumSamples,
                           int32 InNumChannels, const int32 InSampleRate, double) override;
};
