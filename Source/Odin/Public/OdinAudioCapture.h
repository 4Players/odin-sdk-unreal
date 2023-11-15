/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "AudioCapture.h"
#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Tickable.h"

#if ENGINE_MAJOR_VERSION >= 5
#include "AudioDeviceNotificationSubsystem.h"
#endif

#include "OdinAudioCapture.generated.h"

USTRUCT(BlueprintType)
struct ODIN_API FOdinCaptureDeviceInfo {
    GENERATED_BODY()

    /**
     * @brief The internal id of the device.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AudioCapture")
    FString DeviceId;
    /**
     * @brief Additional device info, like the name and sample rate.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AudioCapture")
    FAudioCaptureDeviceInfo AudioCaptureInfo;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FGetCaptureDeviceDelegate, const TArray<FOdinCaptureDeviceInfo>&,
                                   OutDevices, const FOdinCaptureDeviceInfo&, CurrentDevice);

DECLARE_DYNAMIC_DELEGATE_OneParam(FChangeCaptureDeviceDelegate, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCaptureDeviceChange);

UCLASS(ClassGroup = (Odin), Blueprintable, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinAudioCapture : public UAudioCapture, public FTickableGameObject
{
    GENERATED_BODY()

  public:
    /**
     * @brief IMPORTANT! Use the Async version, in case you experience stuttering. Returns all
     * available capture devices with the device id.
     * @param OutDevices All available capture devices
     */
    UFUNCTION(BlueprintPure, Category = "AudioCapture")
    void GetCaptureDevicesAvailable(TArray<FOdinCaptureDeviceInfo>& OutDevices);

    /**
     * @brief Returns all available capture devices with the device id. Runs async.
     * @param OnResult Callback for when the result is available.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void AsyncGetCaptureDevicesAvailable(FGetCaptureDeviceDelegate OnResult);

    /**
     * @brief Returns info on the current capture device.
     * @param CurrentDevice Info on the current capture device
     */
    UFUNCTION(BlueprintPure, Category = "AudioCapture")
    void GetCurrentAudioCaptureDevice(FOdinCaptureDeviceInfo& CurrentDevice);

    /**
     * @brief IMPORTANT! Use the Async version, in case you experience stuttering. Updates the
     * capture device and restarts the capture stream of the Audio Capture component.
     * @param NewDeviceId The id of the targeted capture device.
     * @param bSuccess  Whether or not the capture device was successfully changed
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void ChangeCaptureDeviceById(FString NewDeviceId, bool& bSuccess);

    /**
     * @brief Updates the capture device and restarts the capture stream of the Audio Capture
     * component. Runs async, therefore not blocking the main thread.
     * @param NewDeviceId The id of the targeted capture device.
     * @param OnChangeCompleted Callback for when the change result is available.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void AsyncChangeCaptureDeviceById(FString                      NewDeviceId,
                                      FChangeCaptureDeviceDelegate OnChangeCompleted);

    /**
     * @brief IMPORTANT! Use the Async version, in case you experience stuttering. Updates the
     * capture device and restarts the capture stream of the Audio Capture component.
     * @param DeviceName The name of the targeted capture device. Needs to be an exact match.
     * @param bSuccess Whether or not the capture device was successfully changed
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void ChangeCaptureDeviceByName(FName DeviceName, bool& bSuccess);

    /**
     * @brief Updates the capture device and restarts the capture stream of the Audio Capture
     * component. Runs async, therefore not blocking the main thread.
     * @param DeviceName The name of the targeted capture device. Needs to be an exact match.
     * @param OnChangeCompleted Callback for when the change result is available.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    void AsyncChangeCaptureDeviceByName(FName                        DeviceName,
                                        FChangeCaptureDeviceDelegate OnChangeCompleted);

    /**
     * @brief Get whether the stream is currently open.
     * @return Returns true if capturing audio
     */
    UFUNCTION(BlueprintPure, Category = "AudioCapture")
    bool IsStreamOpen() const;

    /**
     * @brief Get the stream time of the audio capture stream.
     * @return Time the stream was active.
     */
    UFUNCTION(BlueprintPure, Category = "AudioCapture")
    float GetStreamTime() const;

    /**
     * @brief Restart the stream, using CurrentSelectedDeviceIndex as the new input.
     */
    UFUNCTION(BlueprintCallable, Category = "AudioCapture")
    bool RestartCapturing(bool bAutomaticallyStartCapture = true);

    UFUNCTION(BlueprintInternalUseOnly, BlueprintPure, Category = "AudioCapture")
    bool GetIsPaused() const;

    UFUNCTION(BlueprintInternalUseOnly, BlueprintCallable, Category = "AudioCapture")
    void SetIsPaused(bool newValue);

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

    virtual ETickableTickType GetTickableTickType() const override
    {
        return ETickableTickType::Always;
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

    void RetrieveCurrentSelectedDeviceIndex();

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
    float AllowedTimeWithoutStreamUpdate = 0.25f;

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
     * @brief Will be filled in, once a device was selected by the user.
     * We can't have access to this before the custom selection, because - at least the Windows
     * RtAudio implementation - AudioDeviceInterface does not provide us with a Device Id or Url of
     * the Default Device.
     */
    FOdinCaptureDeviceInfo CustomSelectedDevice;

    double LastStreamTime            = -1.0f;
    double TimeWithoutStreamUpdate   = 0.0f;
    bool   IsCurrentlyChangingDevice = false;
};
