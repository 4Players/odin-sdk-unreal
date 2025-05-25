/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinSubmixListener.h"
#include "Odin.h"
#include "Sound/SoundSubmix.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "AudioDeviceHandle.h"
#else
#include "AudioDeviceManager.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "ISubmixBufferListener.h"
#endif

#include "AudioDevice.h"
#include "OdinFunctionLibrary.h"
#include "OdinSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

using namespace Audio;

UOdinSubmixListener::UOdinSubmixListener(const class FObjectInitializer& PCIP)
    : Super(PCIP)
    , CurrentRoomHandle(0)
{
}

void UOdinSubmixListener::StartSubmixListener()
{
    UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: StartSubmixListener Called."))

    if (!GetWorld()) {
        UE_LOG(Odin, Error, TEXT("OdinSubmixListener: StartSubmixListener failed, invalid World."));
        return;
    }

    if (GetWorld()->IsPlayingReplay()) {
        UE_LOG(Odin, Log, TEXT("OdinSubmixListener: StartSubmixListener aborted, playing replay."));
        return;
    }

    if (IsListening()) {
        UE_LOG(Odin, Verbose,
               TEXT("OdinSubmixListener, StartSubmixListener: Already listening, restarting submix "
                    "listener"));
        StopSubmixListener();
    }

    const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    if (!GameInstance) {
        UE_LOG(Odin, Error,
               TEXT("OdinSubmixListener: StartSubmixListener failed, GameInstance is invalid."));
        return;
    }

    const UOdinSubsystem* OdinInitializationSubsystem =
        GameInstance->GetSubsystem<UOdinSubsystem>();
    if (!OdinInitializationSubsystem) {
        UE_LOG(Odin, Error,
               TEXT("OdinSubmixListener: StartSubmixListener failed, UOdinInitializationSubsystem "
                    "is invalid."));
        return;
    }

    const int32 OdinSampleRate = OdinInitializationSubsystem->GetSampleRate();
    const int32 OdinChannels   = OdinInitializationSubsystem->GetChannelCount();

    UE_LOG(Odin, Verbose,
           TEXT("OdinSubmixListener: Starting Submix Listener with OdinSampleRate %d and "
                "OdinChannels %d"),
           OdinSampleRate, OdinChannels);

    FAudioDeviceHandle AudioDeviceHandle = GetWorld()->GetAudioDevice();
    RegisteredDeviceId                   = AudioDeviceHandle->DeviceID;

    UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: Retrieved Audio Device with Handle Id: %d"),
           AudioDeviceHandle.GetDeviceID());
    if (!AudioDeviceHandle.IsValid()) {
        UE_LOG(Odin, Error,
               TEXT("OdinSubmixListener: StartSubmixListener failed, no Active AudioDevice "
                    "available."));
        return;
    }
    const int32 SampleRate = AudioDeviceHandle->SampleRate;
    if (SampleRate != OdinSampleRate) {
        UE_LOG(Odin, Warning,
               TEXT("OdinSubmixListener: Detected difference in sample rate: %d In Sample Rate and "
                    "%d Odin Sample "
                    "Rate. Echo Cancellation will not work correctly!"),
               SampleRate, OdinSampleRate);
    }

    const FOnSubmixBufferListenerError ErrorDelegate =
        FOnSubmixBufferListenerError::CreateUObject(this, &UOdinSubmixListener::StopSubmixListener);

    if (!SubmixBufferListener.IsValid())
        SubmixBufferListener = MakeShared<FOdinSubmixBufferListenerImplementation>();

    SubmixBufferListener->Initialize(CurrentRoomHandle, OdinSampleRate, OdinChannels,
                                     ErrorDelegate);
    UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: Registering SubmixBufferListener Called."))
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
    ConnectedSubmix = &AudioDeviceHandle->GetMainSubmixObject();
    AudioDeviceHandle->RegisterSubmixBufferListener(SubmixBufferListener.ToSharedRef(),
                                                    *ConnectedSubmix.Get());
#else
    AudioDeviceHandle->RegisterSubmixBufferListener(SubmixBufferListener.Get());
#endif
}

void UOdinSubmixListener::StopSubmixListener()
{
    if (!GetWorld() || !IsListening() || !SubmixBufferListener.IsValid()
        || !FAudioDeviceManager::Get())
        return;

    FAudioDeviceHandle AudioDeviceHandle =
        FAudioDeviceManager::Get()->GetAudioDevice(RegisteredDeviceId);
    if (!AudioDeviceHandle.IsValid()) {
        return;
    }

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
    if (!ConnectedSubmix.IsValid()) {
        UE_LOG(
            Odin, Error,
            TEXT("UOdinSubmixListener: StopSubmixListener failed, Connected Submix is invalid."));
        return;
    }
    AudioDeviceHandle->UnregisterSubmixBufferListener(SubmixBufferListener.ToSharedRef(),
                                                      *ConnectedSubmix.Get());
#else
    AudioDeviceHandle->UnregisterSubmixBufferListener(SubmixBufferListener.Get());
#endif
    SubmixBufferListener->StopListener();
    UE_LOG(Odin, Verbose, TEXT("OdinSubmixListener: UnregisterSubmixBufferListener Called."))
}

void UOdinSubmixListener::SetRoom(OdinRoomHandle room)
{
    CurrentRoomHandle = room;
}

void UOdinSubmixListener::BeginDestroy()
{
    Super::BeginDestroy();
    if (IsListening())
        StopSubmixListener();
}

bool UOdinSubmixListener::IsListening() const
{
    return SubmixBufferListener.IsValid() && SubmixBufferListener->IsInitialized();
}

void UOdinSubmixListener::SetRecordSubmixOutput(bool bNewActive)
{
    if (SubmixBufferListener.IsValid()) {
        SubmixBufferListener->SetRecordSubmixOutput(bNewActive);
    }
}

void UOdinSubmixListener::StopRecording()
{
    if (SubmixBufferListener.IsValid()) {
        SubmixBufferListener->StopSubmixRecording();
    }
}

FOdinSubmixBufferListenerImplementation::FOdinSubmixBufferListenerImplementation()
    : CurrentRoomHandle(0)
    , ResamplerHandle(0)
    , bRecordSubmixOutput(false)
    , RawBuffer(nullptr)
    , RawBufferSize(0)
{
}

FOdinSubmixBufferListenerImplementation::~FOdinSubmixBufferListenerImplementation()
{
    delete[] RawBuffer;
    RawBuffer = nullptr;
}

void FOdinSubmixBufferListenerImplementation::Initialize(
    OdinRoomHandle Handle, int32 SampleRate, int32 Channels,
    const FOnSubmixBufferListenerError& Callback, bool ShouldRecordSubmixOutput)
{
    CurrentRoomHandle   = Handle;
    OdinSampleRate      = SampleRate;
    OdinNumChannels     = Channels;
    ErrorCallback       = Callback;
    bInitialized        = true;
    bRecordSubmixOutput = ShouldRecordSubmixOutput;

    RecordingBuffer.Reset();
    RawBufferSize = SampleRate * Channels;

    delete[] RawBuffer;
    RawBuffer = nullptr;
    RawBuffer = new float[RawBufferSize];
}

void FOdinSubmixBufferListenerImplementation::StopListener()
{
    bInitialized = false;
    StopSubmixRecording();
    delete[] RawBuffer;
    RawBuffer     = nullptr;
    RawBufferSize = 0;
}

bool FOdinSubmixBufferListenerImplementation::IsInitialized() const
{
    return bInitialized;
}

void FOdinSubmixBufferListenerImplementation::SetRecordSubmixOutput(bool bShouldRecord)
{
    bRecordSubmixOutput = bShouldRecord;
}

void FOdinSubmixBufferListenerImplementation::StopSubmixRecording()
{
    if (bRecordSubmixOutput) {
        const FString       FileName = "SubmixOutput";
        FString             FilePath = "";
        FSoundWavePCMWriter Writer;
        Writer.BeginWriteToWavFile(RecordingBuffer, FileName, FilePath, [FileName]() {
            UE_LOG(Odin, Log,
                   TEXT("FOdinSubmixBufferListenerImplementation: Successfully wrote SubMix Output "
                        "to /Saved/BouncedWavFiles/%s.wav"),
                   *FileName);
        });
        bRecordSubmixOutput = false;
    }
}

void FOdinSubmixBufferListenerImplementation::ResetOdinResampler()
{
    if (ResamplerHandle) {
        OdinReturnCode DestroyResult = odin_resampler_destroy(ResamplerHandle);
        if (odin_is_error(DestroyResult)) {
            const FString DestroyErrorString =
                UOdinFunctionLibrary::FormatOdinError(DestroyResult, false);
            UE_LOG(Odin, Error,
                   TEXT("FOdinSubmixBufferListenerImplementation::OnNewSubmixBuffer: Error "
                        "while trying to destroy existing resampler: %s"),
                   *DestroyErrorString);
        }
        ResamplerHandle = 0;
    }

    UE_LOG(Odin, VeryVerbose,
           TEXT("OdinSubmixListener: Creating New Resampler, FromSampleRate: %d, "
                "OdinSampleRate: %d, OdinNumChannels: %d"),
           FromSampleRate, OdinSampleRate, OdinNumChannels);
    ResamplerHandle = odin_resampler_create(FromSampleRate, OdinSampleRate, OdinNumChannels);
}

void FOdinSubmixBufferListenerImplementation::PerformResampling(float*& BufferToUse,
                                                                int32&  NumSamplesToProcess)
{
    NumSamplesToProcess = 0;
    if (!ResamplerHandle || nullptr == RawBuffer) {
        UE_LOG(Odin, Error,
               TEXT("Aborted resampling for odin audio process reverse due to invalid input."));
        return;
    }

    size_t         OutputCapacity = RawBufferSize;
    OdinReturnCode ResampleResult =
        odin_resampler_process(ResamplerHandle, ChannelMixBuffer.GetData(),
                               ChannelMixBuffer.GetNumSamples(), RawBuffer, &OutputCapacity);
    if (odin_is_error(ResampleResult)) {
        FString FormattedError = UOdinFunctionLibrary::FormatOdinError(ResampleResult, false);
        UE_LOG(Odin, Error, TEXT("Aborted odin audio process reverse due to Error: %s"),
               *FormattedError);
    } else if (OutputCapacity != ResampleResult) {
        UE_LOG(Odin, Error,
               TEXT("Aborted odin audio process reverse due to insufficient "
                    "Capacity in the resampler output buffer, required min capacity is %llu"),
               OutputCapacity);
    } else {
        BufferToUse         = RawBuffer;
        NumSamplesToProcess = ResampleResult;
    }
}

void FOdinSubmixBufferListenerImplementation::PerformRemixing(float* AudioData, int32 InNumSamples,
                                                              int32       InNumChannels,
                                                              const int32 InSampleRate)
{
    ChannelMixBuffer.Reset();
    ChannelMixBuffer.Append(AudioData, InNumSamples, InNumChannels, InSampleRate);
    ChannelMixBuffer.MixBufferToChannels(OdinNumChannels);
}

void FOdinSubmixBufferListenerImplementation::OnNewSubmixBuffer(
    const USoundSubmix* OwningSubmix, float* AudioData, int32 InNumSamples, int32 InNumChannels,
    const int32 InSampleRate, double InAudioClock)
{
    if (!IsInitialized())
        return;

    if (InSampleRate != FromSampleRate || InNumChannels != FromNumChannels) {
        FromSampleRate  = InSampleRate;
        FromNumChannels = InNumChannels;
        ResetOdinResampler();
    }

    float* BufferToUse         = AudioData;
    int32  NumSamplesToProcess = InNumSamples;
    if (FromSampleRate != OdinSampleRate || FromNumChannels != OdinNumChannels) {
        PerformRemixing(AudioData, InNumSamples, InNumChannels, InSampleRate);
        PerformResampling(BufferToUse, NumSamplesToProcess);
    }

    if (NumSamplesToProcess < 1 || nullptr == BufferToUse) {
        return;
    }
    const OdinReturnCode Result =
        odin_audio_process_reverse(CurrentRoomHandle, BufferToUse, NumSamplesToProcess);

    if (bRecordSubmixOutput && InNumSamples > 0) {
        RecordingBuffer.Append(AudioData, InNumSamples);
    }

    if (odin_is_error(Result)) {
        UE_LOG(Odin, VeryVerbose, TEXT("OdinSubmixListener: odin_audio_process_reverse result: %d"),
               Result);
        UE_LOG(Odin, VeryVerbose, TEXT("OdinSubmixListener: OnNewSubmixBuffer on %s "),
               *OwningSubmix->GetFName().ToString());

        ErrorCallback.ExecuteIfBound();
    }
}
