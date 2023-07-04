/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinAudioCapture.h"

#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"

#if ENGINE_MAJOR_VERSION >= 5
#include "AudioDeviceNotificationSubsystem.h"
#endif

#include "Odin.h"

void UOdinAudioCapture::BeginDestroy()
{
    Super::BeginDestroy();
    StopCapturingAudio();
}

#if ENGINE_MAJOR_VERSION >= 5

void UOdinAudioCapture::PostInitProperties()
{
    Super::PostInitProperties();
    if (GetWorld()) {
        UAudioDeviceNotificationSubsystem* AudioDeviceNotificationSubsystem =
            UAudioDeviceNotificationSubsystem::Get();
        if (AudioDeviceNotificationSubsystem) {
            AudioDeviceNotificationSubsystem->DefaultCaptureDeviceChangedNative.AddUObject(
                this, &UOdinAudioCapture::HandleDefaultDeviceChanged);
        } else {
            UE_LOG(Odin, Warning,
                   TEXT("Could not retrieve Audio Device Notification Subsystem, "
                        "can't detect changes of default capture device."));
        }
    }
}

void UOdinAudioCapture::HandleDefaultDeviceChanged(EAudioDeviceChangedRole AudioDeviceChangedRole,
                                                   FString                 DeviceId)
{
    FString    RoleAsString = UEnum::GetValueAsString(AudioDeviceChangedRole);
    const bool bIsEmpty     = CustomSelectedDevice.DeviceId.IsEmpty();
    if (INDEX_NONE == CurrentSelectedDeviceIndex || bIsEmpty) {
        UE_LOG(Odin, Display,
               TEXT("Recognized change in default capture device, reconnecting to new default "
                    "device."));
        RestartStream();
        OnDefaultDeviceChanged.Broadcast();
    }
}

#else

void UOdinAudioCapture::PostInitProperties()
{
    Super::PostInitProperties();
    if (GetWorld()) {
        UE_LOG(Odin, Verbose,
               TEXT("Could not retrieve Audio Device Notification Subsystem, can't detect changes "
                    "of default capture device."));
    }
}

void UOdinAudioCapture::HandleDefaultDeviceChanged(FString DeviceId)
{
    const bool bIsEmpty = CustomSelectedDevice.DeviceId.IsEmpty();
    if (INDEX_NONE == CurrentSelectedDeviceIndex || bIsEmpty) {
        UE_LOG(Odin, Display,
               TEXT("Recognized change in default capture device, reconnecting to new default "
                    "device."));
        RestartStream();
        OnDefaultDeviceChanged.Broadcast();
    }
}

#endif

void UOdinAudioCapture::GetCaptureDevicesAvailable(TArray<FOdinCaptureDeviceInfo>& OutDevices)
{
    TArray<Audio::FCaptureDeviceInfo> CaptureDevices;
    AudioCapture.GetCaptureDevicesAvailable(CaptureDevices);
    for (Audio::FCaptureDeviceInfo CaptureDevice : CaptureDevices) {

        FAudioCaptureDeviceInfo audioCaptureDevice;
        audioCaptureDevice.DeviceName       = FName(*CaptureDevice.DeviceName);
        audioCaptureDevice.NumInputChannels = CaptureDevice.InputChannels;
        audioCaptureDevice.SampleRate       = CaptureDevice.PreferredSampleRate;
        OutDevices.Add(FOdinCaptureDeviceInfo{CaptureDevice.DeviceId, audioCaptureDevice});
    }
}

void UOdinAudioCapture::AsyncGetCaptureDevicesAvailable(FGetCaptureDeviceDelegate Out)
{
    AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this, Out]() {
        TArray<FOdinCaptureDeviceInfo> Devices;
        GetCaptureDevicesAvailable(Devices);

        FOdinCaptureDeviceInfo CurrentDevice;
        if (CurrentSelectedDeviceIndex >= 0 && CurrentSelectedDeviceIndex < Devices.Num()) {
            CurrentDevice = Devices[CurrentSelectedDeviceIndex];
        } else {
            CurrentDevice = Devices[0];
        }

        // We schedule back to the main thread and pass in our params
        AsyncTask(ENamedThreads::GameThread, [Devices, CurrentDevice, Out]() {
            // We execute the delegate along with the param
            Out.Execute(Devices, CurrentDevice);
        });
    });
}

void UOdinAudioCapture::GetCurrentAudioCaptureDevice(FOdinCaptureDeviceInfo& CurrentDevice)
{
    TArray<FOdinCaptureDeviceInfo> allDevices;
    GetCaptureDevicesAvailable(allDevices);
    if (allDevices.Num() > 0) {
        if (allDevices.IsValidIndex(CurrentSelectedDeviceIndex)) {
            CurrentDevice = allDevices[CurrentSelectedDeviceIndex];
        } else {
            CurrentDevice = allDevices[0];
        }
    }
}

void UOdinAudioCapture::ChangeCaptureDeviceById(FString NewDeviceId, bool& bSuccess)
{
    // a lambda for finding a device by DeviceId
    auto DeviceCheck = [NewDeviceId](FOdinCaptureDeviceInfo OdinCaptureDeviceInfo) -> bool {
        return NewDeviceId == OdinCaptureDeviceInfo.DeviceId;
    };
    bSuccess = ChangeCaptureDevice(DeviceCheck);
    if (!bSuccess) {
        UE_LOG(
            Odin, Warning,
            TEXT("Did not find Capture Device with Device Id %s, Capture Device was not changed."),
            *NewDeviceId);
    }
}

void UOdinAudioCapture::AsyncChangeCaptureDeviceById(FString                      NewDeviceId,
                                                     FChangeCaptureDeviceDelegate OnChangeCompleted)
{
    AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this, NewDeviceId, OnChangeCompleted]() {
        bool bSuccess;
        ChangeCaptureDeviceById(NewDeviceId, bSuccess);

        AsyncTask(ENamedThreads::GameThread, [OnChangeCompleted, bSuccess]() {
            // We execute the delegate along with the param.
            OnChangeCompleted.Execute(bSuccess);
        });
    });
}

void UOdinAudioCapture::ChangeCaptureDeviceByName(FName DeviceName, bool& bSuccess)
{
    // a lambda for finding a device by DeviceName
    auto DeviceCheck = [DeviceName](FOdinCaptureDeviceInfo OdinCaptureDeviceInfo) -> bool {
        return DeviceName == OdinCaptureDeviceInfo.AudioCaptureInfo.DeviceName;
    };
    bSuccess = ChangeCaptureDevice(DeviceCheck);
    if (!bSuccess) {
        UE_LOG(Odin, Warning,
               TEXT("Did not find Capture Device with name %s, Capture Device was not changed."),
               *DeviceName.ToString());
    }
}

void UOdinAudioCapture::AsyncChangeCaptureDeviceByName(
    FName DeviceName, FChangeCaptureDeviceDelegate OnChangeCompleted)
{
    AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this, DeviceName, OnChangeCompleted]() {
        bool bSuccess;
        ChangeCaptureDeviceByName(DeviceName, bSuccess);

        AsyncTask(ENamedThreads::GameThread, [OnChangeCompleted, bSuccess]() {
            // We execute the delegate along with the param
            OnChangeCompleted.Execute(bSuccess);
        });
    });
}

template <typename DeviceCheck>
bool UOdinAudioCapture::ChangeCaptureDevice(const DeviceCheck& DeviceCheckFunction)
{
    TArray<FOdinCaptureDeviceInfo> allDevices;
    GetCaptureDevicesAvailable(allDevices);

    bool bSuccess = false;

    // look for the name of the selected device.
    for (int32 i = 0; i < allDevices.Num(); ++i) {
        const FOdinCaptureDeviceInfo OdinCaptureDeviceInfo = allDevices[i];
        if (DeviceCheckFunction(OdinCaptureDeviceInfo)) {
            CurrentSelectedDeviceIndex = i;
            CustomSelectedDevice       = OdinCaptureDeviceInfo;
            bSuccess                   = true;
            break;
        }
    }

    if (bSuccess) {
        UE_LOG(Odin, VeryVerbose, TEXT("Selected index: %d with device id: %s"),
               CurrentSelectedDeviceIndex, *CustomSelectedDevice.DeviceId);
        RestartStream();
    }
    return bSuccess;
}

bool UOdinAudioCapture::IsStreamOpen() const
{
    return AudioCapture.IsStreamOpen();
}

float UOdinAudioCapture::GetStreamTime() const
{
    double streamTime;
    AudioCapture.GetStreamTime(streamTime);
    return (float)streamTime;
}

void UOdinAudioCapture::Tick(float DeltaTime)
{
    // Check if the stream is still active.
    // We have to use the stream time, because AudioCapture.IsCapturing() or
    // AudioCapture.IsStreamOpen() do NOT recognize that the underlying capture device was removed.
    if (IsValidLowLevel() && AudioCapture.IsCapturing()) {
        double CurrentStreamTime;
        AudioCapture.GetStreamTime(CurrentStreamTime);
        if (!FMath::IsNearlyEqual(CurrentStreamTime, LastStreamTime)) {
            TimeWithoutStreamUpdate = 0.0f;
            LastStreamTime          = CurrentStreamTime;
        } else {
            TimeWithoutStreamUpdate += DeltaTime;

            const bool bIsStreamSettingUp = CurrentStreamTime < AllowedTimeForStreamSetup;
            const bool bIsStreamOffline = TimeWithoutStreamUpdate > AllowedTimeWithoutStreamUpdate;
            if (bIsStreamOffline && !bIsStreamSettingUp) {
                UE_LOG(Odin, Log,
                       TEXT("Recognized disconnected Capture Device, restarting Capture Stream "
                            "with Default Device..."));
                TimeWithoutStreamUpdate    = 0.0f;
                LastStreamTime             = 0.0f;
                CurrentSelectedDeviceIndex = INDEX_NONE;
                RestartStream();
                OnCaptureDeviceReset.Broadcast();
            }
        }
    }
}

void UOdinAudioCapture::RestartStream()
{
    // Below here is basically a copy of the UAudioCapture::OpenDefaultAudioStream() implementation,
    // except for setting the Params.DeviceIndex.
    Audio::FOnCaptureFunction OnCapture = [this](const float* AudioData, int32 NumFrames,
                                                 int32 InNumChannels, int32 InSampleRate,
                                                 double StreamTime, bool bOverFlow) {
        OnGeneratedAudio(AudioData, NumFrames * InNumChannels);
    };

    Audio::FAudioCaptureDeviceParams Params;
    Params.DeviceIndex = CurrentSelectedDeviceIndex;
    // OpenCaptureStream automatically closes the capture stream, if it's already active.
    if (AudioCapture.OpenCaptureStream(Params, MoveTemp(OnCapture), 1024)) {
        // If we opened the capture stream successfully, get the capture device info and initialize
        // the UAudioGenerator.
        Audio::FCaptureDeviceInfo Info;
        if (AudioCapture.GetCaptureDeviceInfo(Info, CurrentSelectedDeviceIndex)) {
            Init(Info.PreferredSampleRate, Info.InputChannels);
            UE_LOG(Odin, Display, TEXT("Switched to input device %s"), *Info.DeviceName);
        }
    }

    // Restart the audio capture stream.
    AudioCapture.StartStream();
}
