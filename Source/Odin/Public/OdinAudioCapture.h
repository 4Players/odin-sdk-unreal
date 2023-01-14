// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AudioCapture.h"
#include "OdinAudioCapture.generated.h"


USTRUCT(BlueprintType)
struct ODIN_API FOdinCaptureDeviceInfo
{
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

DECLARE_DYNAMIC_DELEGATE_OneParam(FChangeCaptureDeviceDelegate, bool,
                                  bSuccess);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODIN_API UOdinAudioCapture : public UAudioCapture
{
    GENERATED_BODY()

public:
    /**
     * @brief IMPORTANT! Use the Async version, in case you experience stuttering. Returns all available capture devices with the device id.
     * @param OutDevices All available capture devices
     */
    UFUNCTION(BlueprintPure, Category="AudioCapture")
    void GetCaptureDevicesAvailable(TArray<FOdinCaptureDeviceInfo>& OutDevices);

    /**
     * @brief Returns all available capture devices with the device id. Runs async.
     * @param OnResult Callback for when the result is available.
     */
    UFUNCTION(BlueprintCallable, Category="AudioCapture")
    void AsyncGetCaptureDevicesAvailable(FGetCaptureDeviceDelegate OnResult);

    /**
     * @brief Returns info on the current capture device.
     * @param CurrentDevice Info on the current capture device
     */
    UFUNCTION(BlueprintPure, Category="AudioCapture")
    void GetCurrentAudioCaptureDevice(FOdinCaptureDeviceInfo& CurrentDevice);

    /**
     * @brief IMPORTANT! Use the Async version, in case you experience stuttering. Updates the capture device and restarts the capture stream of the Audio Capture component.
     * @param NewDeviceId The id of the targeted capture device.
     * @param bSuccess  Whether or not the capture device was successfully changed
     */
    UFUNCTION(BlueprintCallable, Category="AudioCapture")
    void ChangeCaptureDeviceById(FString NewDeviceId, bool& bSuccess);

    /**
     * @brief Updates the capture device and restarts the capture stream of the Audio Capture component. Runs async, therefore not blocking the main thread.
     * @param NewDeviceId The id of the targeted capture device.
     * @param OnChangeCompleted Callback for when the change result is available.
     */
    UFUNCTION(BlueprintCallable, Category="AudioCapture")
    void AsyncChangeCaptureDeviceById(FString                      NewDeviceId,
                                      FChangeCaptureDeviceDelegate OnChangeCompleted);

    /**
     * @brief IMPORTANT! Use the Async version, in case you experience stuttering. Updates the capture device and restarts the capture stream of the Audio Capture component.
     * @param DeviceName The name of the targeted capture device. Needs to be an exact match.
     * @param bSuccess Whether or not the capture device was successfully changed
     */
    UFUNCTION(BlueprintCallable, Category="AudioCapture")
    void ChangeCaptureDeviceByName(FName DeviceName, bool& bSuccess);

    /**
     * @brief Updates the capture device and restarts the capture stream of the Audio Capture component. Runs async, therefore not blocking the main thread.
     * @param DeviceName The name of the targeted capture device. Needs to be an exact match.
     * @param OnChangeCompleted Callback for when the change result is available.
     */
    UFUNCTION(BlueprintCallable, Category="AudioCapture")
    void AsyncChangeCaptureDeviceByName(FName                        DeviceName,
                                        FChangeCaptureDeviceDelegate OnChangeCompleted);

protected:
    /**
     * @brief The index of the currently selected device. -1 and 0 both refer to the Default Device.
     */
    UPROPERTY(BlueprintReadOnly, Category="AudioCapture")
    int32 CurrentSelectedDeviceIndex = INDEX_NONE;

    void RestartStream();
};
