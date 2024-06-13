/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

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

DECLARE_DELEGATE(FOnSubmixBufferListenerError);

class FOdinSubmixBufferListenerImplementation;

UCLASS(ClassGroup = Utility)
class ODIN_API UOdinSubmixListener : public UObject
{
    GENERATED_BODY()

  public:
    UOdinSubmixListener(const class FObjectInitializer& PCIP);

    void StartSubmixListener();
    void StopSubmixListener();
    void SetRoom(OdinRoomHandle handle);

    virtual void BeginDestroy() override;

    bool IsListening() const;

  private:
    OdinRoomHandle CurrentRoomHandle;

    OdinResamplerHandle                                 resampler_handle;
    TSharedPtr<FOdinSubmixBufferListenerImplementation> SubmixBufferListener;
};

class ODIN_API FOdinSubmixBufferListenerImplementation : public ISubmixBufferListener
{
  public:
    FOdinSubmixBufferListenerImplementation();
    virtual ~FOdinSubmixBufferListenerImplementation();

    void Initialize(OdinRoomHandle Handle, int32 SampleRate, int32 Channels,
                    const FOnSubmixBufferListenerError& Callback);
    void Stop();
    bool IsInitialized() const;

  private:
    virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData,
                                   int32 InNumSamples, int32 InNumChannels,
                                   const int32 InSampleRate, double InAudioClock) override;

    FCriticalSection SubmixCS;
    bool             bInitialized = false;

    OdinRoomHandle               CurrentRoomHandle;
    FOnSubmixBufferListenerError ErrorCallback;

    int32 OdinSampleRate = 48000;
    int32 OdinChannels   = 2;
};
