// Fill out your copyright notice in the Description page of Project Settings.


#include "OdinAudioCapture.h"
#include "Odin.h"


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
        AsyncTask(ENamedThreads::GameThread, [Devices,CurrentDevice, Out]() {
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
        if (CurrentSelectedDeviceIndex >= 0 && CurrentSelectedDeviceIndex < allDevices.Num()) {
            CurrentDevice = allDevices[CurrentSelectedDeviceIndex];
        } else {
            CurrentDevice = allDevices[0];
        }
    }
}

void UOdinAudioCapture::ChangeCaptureDeviceById(FString NewDeviceId, bool& bSuccess)
{
    TArray<FOdinCaptureDeviceInfo> allDevices;
    GetCaptureDevicesAvailable(allDevices);

    bSuccess = false;

    // look for the index of the selected device.
    for (int32 i = 0; i < allDevices.Num(); ++i) {
        FOdinCaptureDeviceInfo deviceInfo = allDevices[i];
        if (deviceInfo.DeviceId == NewDeviceId) {
            CurrentSelectedDeviceIndex = i;
            bSuccess                   = true;
            break;
        }
    }

    if (bSuccess) {
        RestartStream();
    } else {
        UE_LOG(Odin, Warning,
               TEXT("Did not find Capture Device with name %s, Capture Device was not changed."));
    }
}

void UOdinAudioCapture::AsyncChangeCaptureDeviceById(FString                      NewDeviceId,
                                                     FChangeCaptureDeviceDelegate OnChangeCompleted)
{
    AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this, NewDeviceId, OnChangeCompleted]() {
        bool bSuccess;
        ChangeCaptureDeviceById(NewDeviceId, bSuccess);

        AsyncTask(ENamedThreads::GameThread, [OnChangeCompleted, bSuccess]() {
            // We execute the delegate along with the param
            OnChangeCompleted.Execute(bSuccess);
        });
    });

}

void UOdinAudioCapture::ChangeCaptureDeviceByName(FName DeviceName, bool& bSuccess)
{
    TArray<FOdinCaptureDeviceInfo> allDevices;
    GetCaptureDevicesAvailable(allDevices);

    bSuccess = false;
    // look for the name of the selected device.
    for (int32 i = 0; i < allDevices.Num(); ++i) {
        FOdinCaptureDeviceInfo deviceInfo = allDevices[i];
        if (deviceInfo.AudioCaptureInfo.DeviceName == DeviceName) {
            CurrentSelectedDeviceIndex = i;
            bSuccess                   = true;
            break;
        }
    }

    if (bSuccess) {
        RestartStream();
    } else {
        UE_LOG(Odin, Warning,
               TEXT("Did not find Capture Device with name %s, Capture Device was not changed."));
    }
}

void UOdinAudioCapture::AsyncChangeCaptureDeviceByName(FName DeviceName,
                                                       FChangeCaptureDeviceDelegate
                                                       OnChangeCompleted)
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
        // If we opened the capture stream succesfully, get the capture device info and initialize the UAudioGenerator
        Audio::FCaptureDeviceInfo Info;
        if (AudioCapture.GetCaptureDeviceInfo(Info)) {
            Init(Info.PreferredSampleRate, Info.InputChannels);
            UE_LOG(Odin, Display, TEXT("Switched to input device %s"), *Info.DeviceName);
        }
    }

    // Restart the audio capture stream.
    AudioCapture.StartStream();
}
