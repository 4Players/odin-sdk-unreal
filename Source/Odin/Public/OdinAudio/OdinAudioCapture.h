/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once

#include "AudioCapture.h"
#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Tickable.h"

#include "AudioDeviceNotificationSubsystem.h"

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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Odin|Audio Capture")
    FString DeviceId = FString();
    /**
     * @brief Additional device info, like the name and sample rate.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Odin|Audio Capture")
    FAudioCaptureDeviceInfo AudioCaptureInfo = FAudioCaptureDeviceInfo();
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FGetCaptureDeviceDelegate, const TArray<FOdinCaptureDeviceInfo>&, OutDevices, const FOdinCaptureDeviceInfo&, CurrentDevice);

DECLARE_DYNAMIC_DELEGATE_OneParam(FChangeCaptureDeviceDelegate, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCaptureDeviceChange);

UCLASS(ClassGroup = (Odin), Blueprintable, BlueprintType)
class ODIN_API UOdinAudioCapture : public UAudioCapture, public FTickableGameObject
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
    UFUNCTION(BlueprintPure, Category = "Odin|Audio Capture")
    void GetCaptureDevicesAvailable(TArray<FOdinCaptureDeviceInfo>& OutDevices);

    /**
     * @brief Returns info on the current capture device. Only usable in GameThread.
     *
     * IMPORTANT! Should not be used in tick or on a regular basis because it could lead to
     * stuttering.
     *
     * @param CurrentDevice Info on the current capture device
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Audio Capture")
    void GetCurrentAudioCaptureDevice(FOdinCaptureDeviceInfo& CurrentDevice) const;

    /**
     * @brief Changes the current capture device to the system default and restarts the capture
     * stream. Only usable in GameThread.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture")
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
    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture")
    void ChangeCaptureDeviceById(FString NewDeviceId, bool& bSuccess);

    /**
     * @brief Starts Capturing Audio and returns whether capturing was started successfully.
     * @param bSuccess True if capturing was started successfully.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture")
    void StartCapturing(bool& bSuccess);

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
    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture")
    void ChangeCaptureDeviceByName(FName DeviceName, bool& bSuccess);

    /**
     * @brief Get whether the stream is currently open. Only usable in GameThread.
     * @return Returns true if capturing audio
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Audio Capture")
    bool IsStreamOpen() const;

    /**
     * @brief Get the stream time of the audio capture stream. Only usable in GameThread.
     * @return Time the stream was active.
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Audio Capture")
    float GetStreamTime() const;

    /**
     * @brief Restart the stream, using CurrentSelectedDeviceIndex as the new capture device. Only
     * usable in GameThread.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture")
    bool RestartCapturing(bool bAutomaticallyStartCapture = true);

    /**
     * Whether capture device disconnect is being recognized automatically.
     * @return True, if automatic disconnection recognition is active, false otherwise.
     */
    UFUNCTION(BlueprintInternalUseOnly, BlueprintPure, Category = "Odin|Audio Capture")
    bool GetTryRecognizingDeviceDisconnected() const;

    /**
     * Sets automatic recognition of capture device disconnects (i.e. if a capture device was
     * removed). Can accidentally recognize disconnects if app experiences large frame times
     * (greater than AllowedTimeWithoutStreamUpdate value)
     * @param bTryRecognizing New value for whether automatic recognition of disconnects is active.
     */
    UFUNCTION(BlueprintInternalUseOnly, BlueprintCallable, Category = "Odin|Audio Capture")
    void SetTryRecognizingDeviceDisconnected(bool bTryRecognizing);

    /**
     * @brief Will be called, if ODIN recognizes that the selected capture device does not supply
     * data anymore, i.e. if a microphone was unplugged. ODIN will wait for
     * AllowedTimeWithoutStreamUpdate seconds, before trying a stream restart.
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Audio Capture")
    FCaptureDeviceChange OnCaptureDeviceReset;

    /**
     * @brief Will be called, if the Default Device is the currently selected and if the Default
     * Device was switched by the system.
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Audio Capture")
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

    void HandleDefaultDeviceChanged(EAudioDeviceChangedRole AudioDeviceChangedRole, FString DeviceId);

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
    template <typename DeviceCheck> bool ChangeCaptureDevice(const DeviceCheck& DeviceCheckFunction);

    void InitializeGenerator();

    void TryRunAsyncChangeDeviceRequest(FChangeCaptureDeviceDelegate OnChangeCompleted, TFunction<void()> ChangeDeviceFunction);
    void FinalizeCaptureDeviceChange(FChangeCaptureDeviceDelegate OnChangeCompleted, bool& bSuccess);

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
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Capture")
    int32 CurrentSelectedDeviceIndex = INDEX_NONE;

    /**
     * @brief The amount of time in seconds the capture object waits for a stream to resume, before
     * restarting with default device.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin|Audio Capture")
    float AllowedTimeWithoutStreamUpdate = 1.0f;

    /**
     * @brief The amount of time in seconds a capture device is allowed to try and set up the
     * stream. This is especially helpful for i.e. Bluetooth-Connected Headsets.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin|Audio Capture")
    float AllowedTimeForStreamSetup = 3.0f;

    /**
     * Activates / Decativates automatically trying to recognize, if a capture device was removed or
     * not. Can accidentally recognize disconnects if app experiences large frame times (greater
     * than AllowedTimeWithoutStreamUpdate value).
     */
    UPROPERTY(BlueprintGetter = GetTryRecognizingDeviceDisconnected, BlueprintSetter = SetTryRecognizingDeviceDisconnected, Category = "Odin|Audio Capture")
    bool bTryRecognizingDeviceDisconnect = true;

    /**
     * @brief Device Id of the currently known Default Capture Device.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Capture")
    FString DefaultDeviceId;

    /**
     * @brief Will be filled in, once a device was selected by the user.
     * We can't have access to this before the custom selection, because - at least the Windows
     * RtAudio implementation - AudioDeviceInterface does not provide us with a Device Id or Url of
     * the Default Device.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Capture")
    FOdinCaptureDeviceInfo CurrentSelectedDevice;

    double          LastStreamTime            = -1.0f;
    double          TimeWithoutStreamUpdate   = 0.0f;
    FThreadSafeBool IsCurrentlyChangingDevice = false;
};