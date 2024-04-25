/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinAudioCapture.h"

#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"

#if ENGINE_MAJOR_VERSION >= 5
#include "AudioDeviceNotificationSubsystem.h"
#endif

#include "Odin.h"
#include "OdinFunctionLibrary.h"

void UOdinAudioCapture::BeginDestroy()
{
    Super::BeginDestroy();
    AudioCapture.CloseStream();
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
        bool Success;
        ChangeCaptureDeviceById(DeviceId, Success);
        CurrentSelectedDeviceIndex = INDEX_NONE;
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
        bool Success;
        ChangeCaptureDeviceById(DeviceId, Success);
        CurrentSelectedDeviceIndex = INDEX_NONE;
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
    TWeakObjectPtr<UOdinAudioCapture> WeakThisPtr = this;
    AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [WeakThisPtr, Out]() {
        if (!UOdinFunctionLibrary::Check(WeakThisPtr,
                                         "UOdinAudioCapture: AsyncGetCaptureDevicesAvailable")) {
            return;
        }

        TArray<FOdinCaptureDeviceInfo> Devices;
        WeakThisPtr->GetCaptureDevicesAvailable(Devices);

        FOdinCaptureDeviceInfo CurrentDevice;
        if (WeakThisPtr->CurrentSelectedDeviceIndex >= 0
            && WeakThisPtr->CurrentSelectedDeviceIndex < Devices.Num()) {
            CurrentDevice = Devices[WeakThisPtr->CurrentSelectedDeviceIndex];
        } else {
            CurrentDevice = Devices[0];
        }

        // We schedule back to the main thread and pass in our params
        AsyncTask(ENamedThreads::GameThread, [WeakThisPtr, Devices, CurrentDevice, Out]() {
            if (!UOdinFunctionLibrary::Check(
                    WeakThisPtr, "UOdinAudioCapture: AsyncGetCaptureDevicesAvailable")) {
                return;
            }
            // We execute the delegate along with the param
            if (Out.IsBound()) {
                Out.Execute(Devices, CurrentDevice);
            }
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
    TWeakObjectPtr<UOdinAudioCapture> WeakThisPtr = this;
    TryRunAsyncChangeDeviceRequest(
        OnChangeCompleted, [NewDeviceId, OnChangeCompleted, WeakThisPtr]() {
            if (!UOdinFunctionLibrary::Check(WeakThisPtr,
                                             "UOdinAudioCapture::AsyncChangeCaptureDeviceById")) {
                return;
            }
            bool bSuccess;
            WeakThisPtr->ChangeCaptureDeviceById(NewDeviceId, bSuccess);
            WeakThisPtr->FinalizeCaptureDeviceChange(OnChangeCompleted, bSuccess);
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
    TWeakObjectPtr<UOdinAudioCapture> WeakThisPtr = this;
    TryRunAsyncChangeDeviceRequest(
        OnChangeCompleted, [DeviceName, OnChangeCompleted, WeakThisPtr]() {
            if (!UOdinFunctionLibrary::Check(WeakThisPtr,
                                             "UOdinAudioCapture::AsyncChangeCaptureDeviceByName")) {
                return;
            }
            bool bSuccess;
            WeakThisPtr->ChangeCaptureDeviceByName(DeviceName, bSuccess);
            WeakThisPtr->FinalizeCaptureDeviceChange(OnChangeCompleted, bSuccess);
        });
}

void UOdinAudioCapture::TryRunAsyncChangeDeviceRequest(
    FChangeCaptureDeviceDelegate OnChangeCompleted, TFunction<void()> ChangeDeviceFunction)
{
    if (IsCurrentlyChangingDevice) {
        if (OnChangeCompleted.IsBound()) {
            OnChangeCompleted.Execute(false);
        }
        return;
    }
    IsCurrentlyChangingDevice = true;
    AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, ChangeDeviceFunction);
}

void UOdinAudioCapture::FinalizeCaptureDeviceChange(FChangeCaptureDeviceDelegate OnChangeCompleted,
                                                    bool&                        bSuccess)
{
    TWeakObjectPtr<UOdinAudioCapture> WeakThisPtr = this;
    AsyncTask(ENamedThreads::GameThread, [OnChangeCompleted, bSuccess, WeakThisPtr]() {
        if (!UOdinFunctionLibrary::Check(WeakThisPtr,
                                         "UOdinAudioCapture::FinalizeCaptureDeviceChange")) {
            return;
        }
        WeakThisPtr->IsCurrentlyChangingDevice = false;
        // We execute the delegate along with the param
        if (OnChangeCompleted.IsBound()) {
            OnChangeCompleted.Execute(bSuccess);
        }
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
        RestartCapturing();
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
    return static_cast<float>(streamTime);
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
                RestartCapturing();
                OnCaptureDeviceReset.Broadcast();
            }
        }
    }
}

void UOdinAudioCapture::InitializeGenerator()
{
    TArray<FOdinCaptureDeviceInfo> Devices;
    GetCaptureDevicesAvailable(Devices);
    if (Devices.Num() > 0) {
        FOdinCaptureDeviceInfo CurrentDevice;
        if (CurrentSelectedDeviceIndex >= 0 && CurrentSelectedDeviceIndex < Devices.Num()) {
            CurrentDevice = Devices[CurrentSelectedDeviceIndex];
        } else {
            CurrentDevice = Devices[0];
        }
        const FAudioCaptureDeviceInfo AudioCaptureDeviceInfo = CurrentDevice.AudioCaptureInfo;
        Init(AudioCaptureDeviceInfo.SampleRate, AudioCaptureDeviceInfo.NumInputChannels);
        UE_LOG(Odin, Display, TEXT("Switched to input device %s, Sample Rate: %d, Channels: %d"),
               *AudioCaptureDeviceInfo.DeviceName.ToString(), AudioCaptureDeviceInfo.SampleRate,
               AudioCaptureDeviceInfo.NumInputChannels);
    }
}

void UOdinAudioCapture::RetrieveCurrentSelectedDeviceIndex()
{
    Audio::FCaptureDeviceInfo Current;
    AudioCapture.GetCaptureDeviceInfo(Current);
    UE_LOG(
        Odin, Warning,
        TEXT("Using Default Device during Restart Stream, name: %s, samplerate: %d, channels: %d"),
        *Current.DeviceName, Current.PreferredSampleRate, Current.InputChannels);

    TArray<Audio::FCaptureDeviceInfo> OutDevices;
    AudioCapture.GetCaptureDevicesAvailable(OutDevices);
    for (int i = 0; i < OutDevices.Num(); ++i) {
        if (OutDevices[i].DeviceId == Current.DeviceId) {
            CurrentSelectedDeviceIndex = i;
        }
    }
}

bool UOdinAudioCapture::GetIsPaused() const
{
    return bIsCapturingPaused;
}

void UOdinAudioCapture::SetIsPaused(bool newValue)
{
    bIsCapturingPaused = newValue;
}

bool UOdinAudioCapture::RestartCapturing(bool bAutomaticallyStartCapture)
{
    if (AudioCapture.IsStreamOpen()) {
        AudioCapture.CloseStream();
    }
    // Below here is basically a copy of the UAudioCapture::OpenDefaultAudioStream() implementation,
    // except for setting the Params.DeviceIndex.

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

    Audio::FOnAudioCaptureFunction OnCapture = [this](const void* AudioData, int32 NumFrames,
                                                      int32 InNumChannels, int32 InSampleRate,
                                                      double StreamTime, bool bOverFlow) {
        if (!bIsCapturingPaused) {
            OnGeneratedAudio(static_cast<const float*>(AudioData), NumFrames * InNumChannels);
        }
    };
#else
    // Below here is basically a copy of the UAudioCapture::OpenDefaultAudioStream() implementation,
    // except for setting the Params.DeviceIndex.
    Audio::FOnCaptureFunction OnCapture = [this](const float* AudioData, int32 NumFrames,
                                                 int32 InNumChannels, int32 InSampleRate,
                                                 double StreamTime, bool bOverFlow) {
        if (!bIsCapturingPaused) {
            OnGeneratedAudio(AudioData, NumFrames * InNumChannels);
        }
    };
#endif

    if (CurrentSelectedDeviceIndex < 0) {
        RetrieveCurrentSelectedDeviceIndex();
    }

    Audio::FAudioCaptureDeviceParams Params;
    Params.DeviceIndex = CurrentSelectedDeviceIndex;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
    bool bSuccess = AudioCapture.OpenAudioCaptureStream(Params, MoveTemp(OnCapture), 1024);
#else
    bool bSuccess = AudioCapture.OpenCaptureStream(Params, MoveTemp(OnCapture), 1024);
#endif
    // OpenCaptureStream automatically closes the capture stream, if it's already active.
    if (bSuccess) {
        // If we opened the capture stream successfully, get the capture device info and initialize
        // the UAudioGenerator.
        InitializeGenerator();
        // Restart the audio capture stream.
        if (bAutomaticallyStartCapture) {
            AudioCapture.StartStream();
        }
    }
    return bSuccess;
}