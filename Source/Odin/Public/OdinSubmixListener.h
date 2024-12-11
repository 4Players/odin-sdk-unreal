/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "odin_sdk.h"
#include "SampleBuffer.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "ISubmixBufferListener.h"
#else
#include "AudioDevice.h"
#endif

#include "OdinSubmixListener.generated.h"

DECLARE_DELEGATE(FOnSubmixBufferListenerError);

class FOdinSubmixBufferListenerImplementation;
class FAudioDeviceHandle;

UCLASS(ClassGroup = Utility, BlueprintType)
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

    /**
     * Handles the activation or deactivation of recording the submix output to a .wav file. This
     * can be used to verify, that the listener is actually connected to the correct submix. If it
     * is connected to the right submix, you'll be able to hear all in-game sounds in the .wav file,
     * including the Voice Chat.
     *
     * @param bNewActive If true, submix output will be recorded, if false it will not be recorded
     * @remark Please make sure to only use this for short recordings during testing. Because the
     * .wav file contains raw data, it can become very large.
     * @remark The .wav file will be saved in the project/ game folder under
     * /Saved/BouncedWavFiles/SubmixOutput.wav
     * @remark Please make sure to call StopRecording() before closing exiting your application. In
     * a packaged build, this can lead to a crash during the closing of your application.
     * @ref SetRecordSubmixOutput
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Debug")
    void SetRecordSubmixOutput(bool bNewActive);

    /**
     * If Submix output is currently recorded, will stop the recording and start outputting the .wav
     * to a file. SetRecordSubmixOutput needs to have been called previously with a value of true
     * for this function to do anything.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Debug")
    void StopRecording();

  private:
    OdinRoomHandle CurrentRoomHandle;

    TSharedPtr<FOdinSubmixBufferListenerImplementation> SubmixBufferListener;
    TWeakObjectPtr<USoundSubmix>                        ConnectedSubmix;
};

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
class ODIN_API FOdinSubmixBufferListenerImplementation : public ISubmixBufferListener
#else
class ODIN_API FOdinSubmixBufferListenerImplementation
    : public TSharedFromThis<FOdinSubmixBufferListenerImplementation>,
      public ISubmixBufferListener
#endif
{
  public:
    FOdinSubmixBufferListenerImplementation();
    virtual ~FOdinSubmixBufferListenerImplementation();

    void Initialize(OdinRoomHandle Handle, int32 SampleRate, int32 Channels,
                    const FOnSubmixBufferListenerError& Callback,
                    bool                                ShouldRecordSubmixOutput = false);
    void StopListener();

    bool IsInitialized() const;

    void SetRecordSubmixOutput(bool bShouldRecord);
    void StopSubmixRecording();
    void ResetOdinResampler();
    void PerformResampling(float*& BufferToUse, int32& NumSamplesToProcess);
    void PerformRemixing(float* AudioData, int32 InNumSamples, int32 InNumChannels,
                         int32 InSampleRate);

    virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData,
                                   int32 InNumSamples, int32 InNumChannels,
                                   const int32 InSampleRate, double InAudioClock) override;

    bool bInitialized = false;

    OdinRoomHandle               CurrentRoomHandle;
    FOnSubmixBufferListenerError ErrorCallback;

    int32 OdinSampleRate  = 48000;
    int32 OdinNumChannels = 1;

    int32 FromSampleRate  = 0;
    int32 FromNumChannels = 0;

    OdinResamplerHandle ResamplerHandle;

    bool                   bRecordSubmixOutput;
    Audio::TSampleBuffer<> RecordingBuffer;

    Audio::TSampleBuffer<float> ChannelMixBuffer;

    float* RawBuffer;
    int32  RawBufferSize;
};
