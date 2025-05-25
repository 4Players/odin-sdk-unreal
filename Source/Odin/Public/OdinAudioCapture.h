/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "AudioCapture.h"
#include "CoreMinimal.h"
#include "OdinAudioControl.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Tickable.h"

#if ENGINE_MAJOR_VERSION >= 5
#include "AudioDeviceNotificationSubsystem.h"
#endif

#include "OdinAudioCapture.generated.h"

/**
 * Represents additional information about an audio capture device and makes it available to
 * blueprints.
 */
USTRUCT(BlueprintType)
struct ODIN_API FOdinCaptureDeviceInfo {
    GENERATED_BODY()

    /**
     * @brief The internal id of the device.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AudioCapture")
    FString DeviceId = FString();
    /**
     * @brief Additional device info, like the name and sample rate.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AudioCapture")
    FAudioCaptureDeviceInfo AudioCaptureInfo = FAudioCaptureDeviceInfo();
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FGetCaptureDeviceDelegate, const TArray<FOdinCaptureDeviceInfo>&,
                                   OutDevices, const FOdinCaptureDeviceInfo&, CurrentDevice);

DECLARE_DYNAMIC_DELEGATE_OneParam(FChangeCaptureDeviceDelegate, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCaptureDeviceChange);

UCLASS(ClassGroup = (Odin), Blueprintable, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinAudioCapture : public UAudioCapture,
                                   public FTickableGameObject,
                                   public IOdinAudioControl
{
    GENERATED_BODY()

  public:
    /**
     * @brief Returns all available capture devices with the device id. Only usable in GameThread.
     *
     * IMPORTANT! Should not be used in tick or on a regular basis because it could lead to
     * stuttering.
     *
     * @param OutDevices All available capture devices
     */
    UFUNCTION(BlueprintPure, Category = "AudioCapture")
    void GetCaptureDevicesAvailable(TArray<FOdinCaptureDeviceInfo>& OutDevices);

    /**
     * @brief DEPRECATED: Please use the GetCaptureDevicesAvailable function instead. Calls to the
     * the underlying Audio Capture engine components were identified to be not thread-safe, so Odin
     * will not be able to provide relevant Async Function Calls anymore. Calling this will now
     * simply schedule execution to the Game Thread.
     *
     * Original: Returns all available capture devices with the device id. Runs async.
     * @param OnResult Callback for when the result is available.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture",
              meta = (DeprecatedFunction,
                      DeprecationMessage = "Please use GetCaptureDevicesAvailable instead."))
    void AsyncGetCaptureDevicesAvailable(FGetCaptureDeviceDelegate OnResult);

    /**
     * @brief Returns info on the current capture device. Only usable in GameThread.
     *
     * IMPORTANT! Should not be used in tick or on a regular basis because it could lead to
     * stuttering.
     *
     * @param CurrentDevice Info on the current capture device
     */
    UFUNCTION(BlueprintPure, Category = "AudioCapture")
    void GetCurrentAudioCaptureDevice(FOdinCaptureDeviceInfo& CurrentDevice) const;

    /**
     * @brief Changes the current capture device to the system default and restarts the capture
     * stream. Only usable in GameThread.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void ChangeToDefaultCaptureDevice();

    /**
     * @brief Updates the capture device and restarts the capture stream of the Audio Capture
     * object. Only usable in GameThread.
     *
     * IMPORTANT! Should not be used in tick or on a regular basis because it could lead to
     * stuttering.
     *
     * @param NewDeviceId The id of the targeted capture device.
     * @param bSuccess  Whether the capture device was successfully changed
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void ChangeCaptureDeviceById(FString NewDeviceId, bool& bSuccess);

    /**
     * @brief Starts Capturing Audio and returns whether capturing was started successfully.
     * @param bSuccess True if capturing was started successfully.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void StartCapturing(bool& bSuccess);

    /**
     * @brief DEPRECATED: Please use the ChangeCaptureDeviceById function instead. Calls to the
     * the underlying Audio Capture engine components were identified to be not thread-safe, so Odin
     * will not be able to provide relevant Async Function Calls anymore. Calling this will now
     * simply schedule execution to the Game Thread.
     *
     * Original description: Updates the capture device and restarts the capture stream of the Audio
     * Capture object. Runs async, therefore not blocking the main thread.
     * @param NewDeviceId The id of the targeted capture device.
     * @param OnChangeCompleted Callback for when the change result is available.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture",
              meta = (DeprecatedFunction,
                      DeprecationMessage = "Please use ChangeCaptureDeviceById instead."))
    void AsyncChangeCaptureDeviceById(FString                      NewDeviceId,
                                      FChangeCaptureDeviceDelegate OnChangeCompleted);

    /**
     * @brief Updates the capture device and restarts the capture stream of the Audio Capture
     * object. Only usable in GameThread.
     *
     * IMPORTANT! Should not be used in tick or on a regular basis because it could lead to
     * stuttering.
     *
     * @param DeviceName The name of the targeted capture device. Needs to be an exact match.
     * @param bSuccess Whether or not the capture device was successfully changed
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void ChangeCaptureDeviceByName(FName DeviceName, bool& bSuccess);

    /**
     * @brief DEPRECATED: Please use the ChangeCaptureDeviceByName function instead. Calls to the
     * the underlying Audio Capture engine components were identified to be not thread-safe, so Odin
     * will not be able to provide relevant Async Function Calls anymore. Calling this will now
     * simply schedule execution to the Game Thread.
     *
     * Original description: Updates the capture device and restarts the capture stream of the Audio
     * Capture object. Runs async, therefore not blocking the main thread.
     * @param DeviceName The name of the targeted capture device. Needs to be an exact match.
     * @param OnChangeCompleted Callback for when the change result is available.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture",
              meta = (DeprecatedFunction,
                      DeprecationMessage = "Please use ChangeCaptureDeviceByName instead."))
    void AsyncChangeCaptureDeviceByName(FName                        DeviceName,
                                        FChangeCaptureDeviceDelegate OnChangeCompleted);

    /**
     * @brief Get whether the stream is currently open. Only usable in GameThread.
     * @return Returns true if capturing audio
     */
    UFUNCTION(BlueprintPure, Category = "AudioCapture")
    bool IsStreamOpen() const;

    /**
     * @brief Get the stream time of the audio capture stream. Only usable in GameThread.
     * @return Time the stream was active.
     */
    UFUNCTION(BlueprintPure, Category = "AudioCapture")
    float GetStreamTime() const;

    /**
     * @brief Restart the stream, using CurrentSelectedDeviceIndex as the new capture device. Only
     * usable in GameThread.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    bool RestartCapturing(bool bAutomaticallyStartCapture = true);

    UFUNCTION(BlueprintInternalUseOnly, BlueprintPure, Category = "AudioCapture",
              meta = (DeprecatedFunction,
                      DeprecationMessage = "Please use GetIsMuted from IOdinAudioControl"))
    bool GetIsPaused() const;
    UFUNCTION(BlueprintInternalUseOnly, BlueprintCallable, Category = "AudioCapture",
              meta = (DeprecatedFunction,
                      DeprecationMessage = "Please use SetIsMuted from IOdinAudioControl"))
    void SetIsPaused(bool bNewValue);

    /**
     * Whether capture device disconnect is being recognized automatically.
     * @return True, if automatic disconnection recognition is active, false otherwise.
     */
    UFUNCTION(BlueprintInternalUseOnly, BlueprintPure, Category = "AudioCapture")
    bool GetTryRecognizingDeviceDisconnected() const;

    /**
     * Sets automatic recognition of capture device disconnects (i.e. if a capture device was
     * removed). Can accidentally recognize disconnects if app experiences large frame times
     * (greater than AllowedTimeWithoutStreamUpdate value)
     * @param bTryRecognizing New value for whether automatic recognition of disconnects is active.
     */
    UFUNCTION(BlueprintInternalUseOnly, BlueprintCallable, Category = "AudioCapture")
    void SetTryRecognizingDeviceDisconnected(bool bTryRecognizing);

    // --- IOdinAudioControl Interface START ---
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    virtual bool GetIsMuted() const override;
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    virtual void SetIsMuted(bool bNewValue) override;
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    virtual float GetVolumeMultiplier() const override;
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    virtual void SetVolumeMultiplier(float NewMultiplierValue) override;
    // --- IOdinAudioControl Interface END ---

    /**
     * @brief Will be called, if ODIN recognizes that the selected capture device does not supply
     * data anymore, i.e. if a microphone was unplugged. ODIN will wait for
     * AllowedTimeWithoutStreamUpdate seconds, before trying a stream restart.
     */
    UPROPERTY(BlueprintAssignable, Category = "AudioCapture")
    FCaptureDeviceChange OnCaptureDeviceReset;

    /**
     * @brief Will be called, if the Default Device is the currently selected and if the Default
     * Device was switched by the system.
     */
    UPROPERTY(BlueprintAssignable, Category = "AudioCapture")
    FCaptureDeviceChange OnDefaultDeviceChanged;

#pragma region FTickableGameObject
    virtual void Tick(float DeltaTime) override;

    virtual bool IsTickable() const override;

    virtual ETickableTickType GetTickableTickType() const override
    {
        return ETickableTickType::Conditional;
    }

    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UOdinAudioCapture, STATGROUP_Tickables);
    }

    virtual bool IsTickableWhenPaused() const override
    {
        return true;
    }

    virtual bool IsTickableInEditor() const override
    {
        return false;
    }
#pragma endregion

  protected:
    virtual void BeginDestroy() override;
    virtual void PostInitProperties() override;

#if ENGINE_MAJOR_VERSION >= 5
    void HandleDefaultDeviceChanged(EAudioDeviceChangedRole AudioDeviceChangedRole,
                                    FString                 DeviceId);
#else
    void HandleDefaultDeviceChanged(FString DeviceId);
#endif

    /**
     * @brief Actual capture device implementation. Will take a device check function as input.
     * This function should take a FOdinCaptureDeviceInfo as input and return a bool. It should
     * return true, if the input is the device we'd like to change to.
     *
     * @tparam DeviceCheck Function taking a FOdinCaptureDeviceInfo as input and returning bool.
     * @param DeviceCheckFunction This is used to check, whether the currently looked at device is
     * the device we'd like to change to.
     * @return True, if the device was changed successfully
     */
    template <typename DeviceCheck>
    bool ChangeCaptureDevice(const DeviceCheck& DeviceCheckFunction);

    void InitializeGenerator();

    void TryRunAsyncChangeDeviceRequest(FChangeCaptureDeviceDelegate OnChangeCompleted,
                                        TFunction<void()>            ChangeDeviceFunction);
    void FinalizeCaptureDeviceChange(FChangeCaptureDeviceDelegate OnChangeCompleted,
                                     bool&                        bSuccess);

    void TryRetrieveDefaultDevice();

    /**
     * Handles the audio generation logic triggered by the native Audio Capture Implementation
     * callback.
     *
     * @param AudioData Pointer to the raw audio data buffer.
     * @param NumFrames Number of audio frames in the buffer.
     * @param InNumChannels Number of channels in the audio stream.
     */
    void OnCaptureCallback(const float* AudioData, int32 NumFrames, int32 InNumChannels);

    /**
     * @brief The index of the currently selected device. -1 and 0 both refer to the Default Device.
     */
    UPROPERTY(BlueprintReadOnly, Category = "AudioCapture")
    int32 CurrentSelectedDeviceIndex = INDEX_NONE;

    /**
     * @brief The amount of time in seconds the capture object waits for a stream to resume, before
     * restarting with default device.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AudioCapture")
    float AllowedTimeWithoutStreamUpdate = 1.0f;

    /**
     * @brief The amount of time in seconds a capture device is allowed to try and set up the
     * stream. This is especially helpful for i.e. Bluetooth-Connected Headsets.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AudioCapture")
    float AllowedTimeForStreamSetup = 3.0f;

    UPROPERTY(BlueprintGetter = GetIsPaused, BlueprintSetter = SetIsPaused,
              Category = "AudioCapture")
    bool bIsCapturingPaused;

    /**
     * Activates / Decativates automatically trying to recognize, if a capture device was removed or
     * not. Can accidentally recognize disconnects if app experiences large frame times (greater
     * than AllowedTimeWithoutStreamUpdate value).
     */
    UPROPERTY(BlueprintGetter = GetTryRecognizingDeviceDisconnected,
              BlueprintSetter = SetTryRecognizingDeviceDisconnected, Category = "AudioCapture")
    bool bTryRecognizingDeviceDisconnect = true;

    /**
     * @brief Device Id of the currently known Default Capture Device.
     */
    UPROPERTY(BlueprintReadOnly, Category = "AudioCapture")
    FString DefaultDeviceId;

    /**
     * @brief Will be filled in, once a device was selected by the user.
     * We can't have access to this before the custom selection, because - at least the Windows
     * RtAudio implementation - AudioDeviceInterface does not provide us with a Device Id or Url of
     * the Default Device.
     */
    UPROPERTY(BlueprintReadOnly, Category = "AudioCapture")
    FOdinCaptureDeviceInfo CurrentSelectedDevice;

    /**
     * Currently set Volume Multiplier for this Audio Capture
     */
    float VolumeMultiplier = 1.0f;

    /**
     * Buffer for audio that has volume multiplier applied.
     */
    TArray<float> AdjustedAudio;

    double          LastStreamTime            = -1.0f;
    double          TimeWithoutStreamUpdate   = 0.0f;
    FThreadSafeBool IsCurrentlyChangingDevice = false;
};
