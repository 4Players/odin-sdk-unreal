﻿/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

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
    UE_LOG(Odin, Display,
           TEXT("Recognized change in default capture device, new Default Device Id: %s"),
           *DeviceId);

    if (bIsEmpty || CustomSelectedDevice.DeviceId.Equals(DeviceId)) {
        UE_LOG(Odin, Display,
               TEXT("Recognized change in default capture device. Current selected device is "
                    "default device, starting reconnect to new default device."));
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
    if (!IsInGameThread()) {
        UE_LOG(Odin, Error,
               TEXT("Tried running UOdinAudioCapture::GetCaptureDevicesAvailable in "
                    "non-Game-Thread. This is not supported."));
        return;
    }

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
    // Calling this will now simply schedule execution to the Game Thread.
    AsyncTask(ENamedThreads::GameThread, [WeakThisPtr, Out = MoveTemp(Out)]() {
        if (!UOdinFunctionLibrary::Check(WeakThisPtr,
                                         "UOdinAudioCapture: AsyncGetCaptureDevicesAvailable")) {
            return;
        }

        TArray<FOdinCaptureDeviceInfo> Devices;
        WeakThisPtr->GetCaptureDevicesAvailable(Devices);

        FOdinCaptureDeviceInfo CurrentDevice       = FOdinCaptureDeviceInfo();
        int32                  SelectedDeviceIndex = WeakThisPtr->CurrentSelectedDeviceIndex;
        if (Devices.IsValidIndex(SelectedDeviceIndex)) {
            CurrentDevice = Devices[SelectedDeviceIndex];
        } else if (Devices.Num() > 0) {
            CurrentDevice = Devices[0];
        }

        if (!UOdinFunctionLibrary::Check(WeakThisPtr,
                                         "UOdinAudioCapture: AsyncGetCaptureDevicesAvailable")) {
            return;
        }
        // We execute the delegate along with the param
        if (Out.IsBound()) {
            Out.Execute(Devices, CurrentDevice);
        }
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
            UE_LOG(Odin, Warning,
                   TEXT("Currently in the process of changing the Capture Device, ignoring "
                        "repeated Change Device Request."))
            OnChangeCompleted.Execute(false);
        }
        return;
    }
    IsCurrentlyChangingDevice = true;
    if (IsInGameThread()) {
        ChangeDeviceFunction();
    } else {
        AsyncTask(ENamedThreads::GameThread, MoveTemp(ChangeDeviceFunction));
    }
}

void UOdinAudioCapture::FinalizeCaptureDeviceChange(FChangeCaptureDeviceDelegate OnChangeCompleted,
                                                    bool&                        bSuccess)
{
    if (IsInGameThread()) {
        IsCurrentlyChangingDevice = false;
        // We execute the delegate along with the param
        if (OnChangeCompleted.IsBound()) {
            OnChangeCompleted.Execute(bSuccess);
        }
    } else {
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

        UE_LOG(Odin, Verbose, TEXT("Selected index: %d with device id: %s"),
               CurrentSelectedDeviceIndex, *CustomSelectedDevice.DeviceId);

        if (IsInGameThread()) {
            RestartCapturing();
        } else {
            TWeakObjectPtr<UOdinAudioCapture> WeakThisPtr = this;
            AsyncTask(ENamedThreads::GameThread, [WeakThisPtr]() {
                if (WeakThisPtr.IsValid()) {
                    WeakThisPtr->RestartCapturing();
                }
            });
        }
    }
    return bSuccess;
}

bool UOdinAudioCapture::IsStreamOpen() const
{
    if (!IsInGameThread()) {
        UE_LOG(Odin, Error,
               TEXT("Tried running UOdinAudioCapture::IsStreamOpen in non-Game-Thread. This is not "
                    "supported. Returning false by default."));
        return false;
    }
    return AudioCapture.IsStreamOpen();
}

float UOdinAudioCapture::GetStreamTime() const
{
    if (!IsInGameThread()) {
        UE_LOG(Odin, Error,
               TEXT("Tried running UOdinAudioCapture::GetStreamTime in non-Game-Thread. This is "
                    "not supported."));
        return 0.0f;
    }

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
                UE_LOG(Odin, Warning,
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

bool UOdinAudioCapture::IsTickable() const
{
    return GetTryRecognizingDeviceDisconnected();
}

void UOdinAudioCapture::InitializeGenerator()
{
    TArray<FOdinCaptureDeviceInfo> Devices;
    GetCaptureDevicesAvailable(Devices);
    if (Devices.Num() > 0) {
        FOdinCaptureDeviceInfo CurrentDevice;
        if (Devices.IsValidIndex(CurrentSelectedDeviceIndex)) {
            CurrentDevice = Devices[CurrentSelectedDeviceIndex];
        } else {
            CurrentDevice = Devices[0];
        }

        const FAudioCaptureDeviceInfo AudioCaptureDeviceInfo = CurrentDevice.AudioCaptureInfo;
        Init(AudioCaptureDeviceInfo.SampleRate, AudioCaptureDeviceInfo.NumInputChannels);
        UE_LOG(Odin, Display,
               TEXT("Starting up generator with input device %s, Sample Rate: %d, Channels: %d"),
               *AudioCaptureDeviceInfo.DeviceName.ToString(), AudioCaptureDeviceInfo.SampleRate,
               AudioCaptureDeviceInfo.NumInputChannels);
    } else {
        UE_LOG(Odin, Error,
               TEXT("Could not find any available capture devices, InitializeGenerator failed."));
    }
}

void UOdinAudioCapture::TryRetrieveCurrentSelectedDeviceIndex()
{
    if (!IsInGameThread()) {
        UE_LOG(Odin, Error,
               TEXT("Tried running UOdinAudioCapture::TryRetrieveCurrentSelectedDeviceIndex in "
                    "non-Game-Thread. This is not supported."));
        return;
    }

    Audio::FCaptureDeviceInfo Current;
    AudioCapture.GetCaptureDeviceInfo(Current);
    UE_LOG(
        Odin, Warning,
        TEXT("Using Default Device during Restart Stream, name: %s, samplerate: %d, channels: %d"),
        *Current.DeviceName, Current.PreferredSampleRate, Current.InputChannels);

    CurrentSelectedDeviceIndex = INDEX_NONE;
    TArray<Audio::FCaptureDeviceInfo> OutDevices;
    AudioCapture.GetCaptureDevicesAvailable(OutDevices);
    for (int i = 0; i < OutDevices.Num(); ++i) {
        if (OutDevices[i].DeviceId == Current.DeviceId) {
            CurrentSelectedDeviceIndex = i;
        }
    }
}

bool UOdinAudioCapture::GetTryRecognizingDeviceDisconnected() const
{
    return bTryRecognizingDeviceDisconnect;
}

void UOdinAudioCapture::SetTryRecognizingDeviceDisconnected(bool bTryRecognizing)
{
    bTryRecognizingDeviceDisconnect = bTryRecognizing;
}

bool UOdinAudioCapture::RestartCapturing(bool bAutomaticallyStartCapture)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinAudioCapture::RestartCapturing)

    if (!IsInGameThread()) {
        UE_LOG(Odin, Error,
               TEXT("Tried running UOdinAudioCapture::RestartCapturing in non-Game-Thread. This is "
                    "not supported, "
                    "aborting restart."));
        return false;
    }

    if (AudioCapture.IsStreamOpen()) {
        AudioCapture.CloseStream();
    }
    // Below here is basically a copy of the UAudioCapture::OpenDefaultAudioStream() implementation,
    // except for setting the Params.DeviceIndex.

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

    Audio::FOnAudioCaptureFunction OnCapture = [this](const void* AudioData, int32 NumFrames,
                                                      int32 InNumChannels, int32 InSampleRate,
                                                      double StreamTime, bool bOverFlow) {
        OnCaptureCallback(static_cast<const float*>(AudioData), NumFrames, InNumChannels);
    };
#else
    // Below here is basically a copy of the UAudioCapture::OpenDefaultAudioStream() implementation,
    // except for setting the Params.DeviceIndex.
    Audio::FOnCaptureFunction OnCapture = [this](const float* AudioData, int32 NumFrames,
                                                 int32 InNumChannels, int32 InSampleRate,
                                                 double StreamTime, bool bOverFlow) {
        OnCaptureCallback(static_cast<const float*>(AudioData), NumFrames, InNumChannels);
    };
#endif

    if (CurrentSelectedDeviceIndex < 0) {
        TryRetrieveCurrentSelectedDeviceIndex();
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
            StartCapturingAudio();
        }
    }
    return bSuccess;
}

void UOdinAudioCapture::OnCaptureCallback(const float* AudioData, int32 NumFrames,
                                          int32 InNumChannels)
{
    if (!GetIsMuted()) {
        const int32 NumSamples = NumFrames * InNumChannels;
        if (AdjustedAudio.Num() != NumSamples) {
            AdjustedAudio.SetNumZeroed(NumSamples);
        }
        for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex) {
            AdjustedAudio[SampleIndex] =
                AudioData[SampleIndex] * FMath::Pow(GetVolumeMultiplier(), 3);
        }

        OnGeneratedAudio(AdjustedAudio.GetData(), NumSamples);
    }
}

bool UOdinAudioCapture::GetIsPaused() const
{
    return GetIsMuted();
}

void UOdinAudioCapture::SetIsPaused(bool bNewValue)
{
    SetVolumeMultiplier(bNewValue);
}

bool UOdinAudioCapture::GetIsMuted() const
{
    return bIsCapturingPaused;
}

void UOdinAudioCapture::SetIsMuted(bool bNewValue)
{
    bIsCapturingPaused = bNewValue;
}

float UOdinAudioCapture::GetVolumeMultiplier() const
{
    return VolumeMultiplier;
}

void UOdinAudioCapture::SetVolumeMultiplier(float NewMultiplierValue)
{
    VolumeMultiplier = NewMultiplierValue;
}