/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinAudioCapture.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "AudioDeviceNotificationSubsystem.h"
#include "OdinVoice.h"

void UOdinAudioCapture::BeginDestroy()
{
    ODIN_LOG(Verbose, "ODIN Destroy: %s", ANSI_TO_TCHAR(__FUNCTION__));

    Super::BeginDestroy();
    if (AudioCapture.IsStreamOpen()) {
        AudioCapture.AbortStream();
    }
}

void UOdinAudioCapture::PostInitProperties()
{
    Super::PostInitProperties();
    if (GetWorld()) {
        if (UAudioDeviceNotificationSubsystem* AudioDeviceNotificationSubsystem = UAudioDeviceNotificationSubsystem::Get()) {
            AudioDeviceNotificationSubsystem->DefaultCaptureDeviceChangedNative.AddUObject(this, &UOdinAudioCapture::HandleDefaultDeviceChanged);
        } else {
            ODIN_LOG(Warning, TEXT("Could not retrieve Audio Device Notification Subsystem, "
                                   "can't detect changes of default capture device."));
        }
    }
}

void UOdinAudioCapture::HandleDefaultDeviceChanged(EAudioDeviceChangedRole AudioDeviceChangedRole, FString DeviceId)
{
    const bool bIsCurrentDeviceDefault = CurrentSelectedDevice.DeviceId.Equals(DefaultDeviceId);
    DefaultDeviceId                    = DeviceId;

    const FString RoleAsString = UEnum::GetValueAsString(AudioDeviceChangedRole);
    ODIN_LOG(Display, TEXT("Recognized change in default capture device, new Default Device Id: %s, Role: %s"), *DeviceId, *RoleAsString);
    if (bIsCurrentDeviceDefault) {
        ODIN_LOG(Display, TEXT("Recognized change in default capture device. Current selected device is "
                               "default device, starting reconnect to new default device."));

        bool bSuccess;
        ChangeCaptureDeviceById(DeviceId, bSuccess);
        OnDefaultDeviceChanged.Broadcast();
    }
}

void UOdinAudioCapture::GetCaptureDevicesAvailable(TArray<FOdinCaptureDeviceInfo>& OutDevices)
{
    if (!IsInGameThread()) {
        ODIN_LOG(Error, TEXT("Tried running UOdinAudioCapture::GetCaptureDevicesAvailable in "
                             "non-Game-Thread. This is not supported."));
        return;
    }

    TArray<Audio::FCaptureDeviceInfo> CaptureDevices;
    AudioCapture.GetCaptureDevicesAvailable(CaptureDevices);
    for (Audio::FCaptureDeviceInfo CaptureDevice : CaptureDevices) {

        FAudioCaptureDeviceInfo AudioCaptureDevice;
        AudioCaptureDevice.DeviceName       = FName(*CaptureDevice.DeviceName);
        AudioCaptureDevice.NumInputChannels = CaptureDevice.InputChannels;
        AudioCaptureDevice.SampleRate       = CaptureDevice.PreferredSampleRate;
        OutDevices.Add(FOdinCaptureDeviceInfo{CaptureDevice.DeviceId, AudioCaptureDevice});
    }
}

void UOdinAudioCapture::GetCurrentAudioCaptureDevice(FOdinCaptureDeviceInfo& CurrentDevice) const
{
    CurrentDevice = CurrentSelectedDevice;
}

void UOdinAudioCapture::ChangeToDefaultCaptureDevice()
{
    bool bSuccess;
    ChangeCaptureDeviceById(DefaultDeviceId, bSuccess);
}

void UOdinAudioCapture::ChangeCaptureDeviceById(FString NewDeviceId, bool& bSuccess)
{
    // a lambda for finding a device by DeviceId
    auto DeviceCheck = [NewDeviceId](FOdinCaptureDeviceInfo OdinCaptureDeviceInfo) -> bool { return NewDeviceId == OdinCaptureDeviceInfo.DeviceId; };
    bSuccess         = ChangeCaptureDevice(DeviceCheck);
    if (!bSuccess) {
        ODIN_LOG(Warning, TEXT("Did not find Capture Device with Device Id %s, Capture Device was not changed."), *NewDeviceId);
    }
}

void UOdinAudioCapture::StartCapturing(bool& bSuccess)
{
    if (AudioCapture.IsStreamOpen()) {
        bSuccess = AudioCapture.StartStream();
        return;
    }
    bSuccess = false;
}

void UOdinAudioCapture::ChangeCaptureDeviceByName(FName DeviceName, bool& bSuccess)
{
    // a lambda for finding a device by DeviceName
    auto DeviceCheck = [DeviceName](FOdinCaptureDeviceInfo OdinCaptureDeviceInfo) -> bool {
        return DeviceName == OdinCaptureDeviceInfo.AudioCaptureInfo.DeviceName;
    };
    bSuccess = ChangeCaptureDevice(DeviceCheck);
    if (!bSuccess) {
        ODIN_LOG(Warning, TEXT("Did not find Capture Device with name %s, Capture Device was not changed."), *DeviceName.ToString());
    }
}

void UOdinAudioCapture::TryRunAsyncChangeDeviceRequest(FChangeCaptureDeviceDelegate OnChangeCompleted, TFunction<void()> ChangeDeviceFunction)
{
    if (IsCurrentlyChangingDevice) {
        if (OnChangeCompleted.IsBound()) {
            ODIN_LOG(Warning, TEXT("Currently in the process of changing the Capture Device, ignoring "
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

void UOdinAudioCapture::FinalizeCaptureDeviceChange(FChangeCaptureDeviceDelegate OnChangeCompleted, bool& bSuccess)
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
            if (!WeakThisPtr.IsValid()) {
                ODIN_LOG(Error, "Aborting FinalizeCaptureDeviceChange due "
                                "to invalid object ptr.");
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

template <typename DeviceCheck> bool UOdinAudioCapture::ChangeCaptureDevice(const DeviceCheck& DeviceCheckFunction)
{
    TArray<FOdinCaptureDeviceInfo> AllDevices;
    GetCaptureDevicesAvailable(AllDevices);

    bool bSuccess = false;
    // look for the name of the selected device.
    for (int32 i = 0; i < AllDevices.Num(); ++i) {
        const FOdinCaptureDeviceInfo OdinCaptureDeviceInfo = AllDevices[i];
        if (DeviceCheckFunction(OdinCaptureDeviceInfo)) {
            if (OdinCaptureDeviceInfo.DeviceId == CurrentSelectedDevice.DeviceId) {
                ODIN_LOG(Log, TEXT("Tried changing to the current selected Device. Doing nothing."));
                return true;
            } else {
                CurrentSelectedDeviceIndex = i;
                CurrentSelectedDevice      = OdinCaptureDeviceInfo;
                bSuccess                   = true;
            }
            break;
        }
    }

    if (bSuccess) {

        ODIN_LOG(Verbose, TEXT("Selected index: %d with device id: %s"), CurrentSelectedDeviceIndex, *CurrentSelectedDevice.DeviceId);

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
        ODIN_LOG(Error, TEXT("Tried running UOdinAudioCapture::IsStreamOpen in non-Game-Thread. This is not "
                             "supported. Returning false by default."));
        return false;
    }
    return AudioCapture.IsStreamOpen();
}

float UOdinAudioCapture::GetStreamTime() const
{
    if (!IsInGameThread()) {
        ODIN_LOG(Error, TEXT("Tried running UOdinAudioCapture::GetStreamTime in non-Game-Thread. This is "
                             "not supported."));
        return 0.0f;
    }

    double StreamTime;
    AudioCapture.GetStreamTime(StreamTime);
    return static_cast<float>(StreamTime);
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
            const bool bIsStreamOffline   = TimeWithoutStreamUpdate > AllowedTimeWithoutStreamUpdate;
            if (bIsStreamOffline && !bIsStreamSettingUp) {
                ODIN_LOG(Warning, TEXT("Recognized disconnected Capture Device, restarting Capture Stream "
                                       "with Default Device..."));

                TimeWithoutStreamUpdate = 0.0f;
                LastStreamTime          = 0.0f;

                // Reset current device info, in case the user switched the capture device format
                // instead of changing the capture device.
                CurrentSelectedDeviceIndex = INDEX_NONE;
                CurrentSelectedDevice      = FOdinCaptureDeviceInfo();
                ChangeToDefaultCaptureDevice();
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
    FOdinCaptureDeviceInfo CurrentDevice;
    GetCurrentAudioCaptureDevice(CurrentDevice);
    int32 CurrentSampleRate = CurrentDevice.AudioCaptureInfo.SampleRate;
    int32 CurrentChannels   = CurrentDevice.AudioCaptureInfo.NumInputChannels;
    if (CurrentSampleRate >= 8000 && CurrentChannels >= 1) {
        Init(CurrentSampleRate, CurrentChannels);
        ODIN_LOG(Display,
                 TEXT("Starting up generator with input device %s, Device Id %s,  Sample Rate: %d, "
                      "Channels: %d"),
                 *CurrentDevice.AudioCaptureInfo.DeviceName.ToString(), *CurrentDevice.DeviceId, CurrentSampleRate, CurrentChannels);
    } else {
        ODIN_LOG(Error,
                 TEXT("Current Capture Device Data is invalid, Sample Rate: %d, Input Channels: %d, "
                      "InitializeGenerator failed."),
                 CurrentSampleRate, CurrentChannels);
    }
}

void UOdinAudioCapture::TryRetrieveDefaultDevice()
{
    if (!IsInGameThread()) {
        ODIN_LOG(Error, TEXT("Tried running UOdinAudioCapture::TryRetrieveDefaultDeviceIndex in "
                             "non-Game-Thread. This is not supported."));
        return;
    }

    Audio::FCaptureDeviceInfo Current;
    const bool                bSuccess = AudioCapture.GetCaptureDeviceInfo(Current);
    if (bSuccess) {
        ODIN_LOG(Log,
                 TEXT("Using Default Device during Restart Stream, Name: %s, Samplerate: %d, "
                      "Channels: %d"),
                 *Current.DeviceName, Current.PreferredSampleRate, Current.InputChannels);

        CurrentSelectedDeviceIndex = INDEX_NONE;
        // Default init with available data, works even if platform does not return valid device
        // ids.
        CurrentSelectedDevice                  = FOdinCaptureDeviceInfo();
        CurrentSelectedDevice.DeviceId         = Current.DeviceId;
        CurrentSelectedDevice.AudioCaptureInfo = FAudioCaptureDeviceInfo{FName(Current.DeviceName), Current.InputChannels, Current.PreferredSampleRate};
        DefaultDeviceId                        = CurrentSelectedDevice.DeviceId;
        ODIN_LOG(Log, TEXT("Retrieved Current Default Device with Id %s"), *Current.DeviceId);

        // Try to get actual data
        TArray<FOdinCaptureDeviceInfo> OutDevices;
        GetCaptureDevicesAvailable(OutDevices);
        for (int32 i = 0; i < OutDevices.Num(); ++i) {
            if (OutDevices[i].DeviceId == Current.DeviceId) {
                CurrentSelectedDeviceIndex = i;
                CurrentSelectedDevice      = OutDevices[CurrentSelectedDeviceIndex];
                DefaultDeviceId            = CurrentSelectedDevice.DeviceId;
                break;
            }
        }
    } else {
        ODIN_LOG(Error, TEXT("Error when trying to retrieve Default Device Index. This could happen if "
                             "there is no available Capture Device connected."));
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
        ODIN_LOG(Error, TEXT("Tried running UOdinAudioCapture::RestartCapturing in non-Game-Thread. This is "
                             "not supported, "
                             "aborting restart."));
        return false;
    }

    if (AudioCapture.IsStreamOpen()) {
        AudioCapture.CloseStream();
    }
    // Below here is basically a copy of the UAudioCapture::OpenDefaultAudioStream() implementation,
    // except for setting the Params.DeviceIndex.
    Audio::FOnAudioCaptureFunction OnCapture = [this](const void* AudioData, int32 NumFrames, int32 InNumChannels, int32 InSampleRate, double StreamTime,
                                                      bool bOverFlow) { OnCaptureCallback(static_cast<const float*>(AudioData), NumFrames, InNumChannels); };

    if (CurrentSelectedDeviceIndex < 0) {
        TryRetrieveDefaultDevice();
    }

    Audio::FAudioCaptureDeviceParams Params;
    Params.DeviceIndex = CurrentSelectedDeviceIndex;

    const int32 SelectedDeviceOptimalFrames =
        CurrentSelectedDevice.AudioCaptureInfo.SampleRate * CurrentSelectedDevice.AudioCaptureInfo.NumInputChannels * (20.0f / 1000.0f);

    ODIN_LOG(Verbose, "Selected Device Optimal Frames: %d, Choosing ODIN-preferred NumFramesDesired: %d", SelectedDeviceOptimalFrames,
             SelectedDeviceOptimalFrames);
    bool bSuccess = AudioCapture.OpenAudioCaptureStream(Params, MoveTemp(OnCapture), SelectedDeviceOptimalFrames);
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

void UOdinAudioCapture::OnCaptureCallback(const float* AudioData, int32 NumFrames, int32 InNumChannels)
{
    const int32 NumSamples = NumFrames * InNumChannels;
    ODIN_LOG(VeryVerbose, "OnCaptureCallback with Num Samples: %d", NumSamples);
    OnGeneratedAudio(AudioData, NumSamples);
}
