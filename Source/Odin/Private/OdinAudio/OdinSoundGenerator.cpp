/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinSoundGenerator.h"

#include "OdinFunctionLibrary.h"
#include "OdinVoice.h"
#include "Components/SynthComponent.h"
#include "OdinAudio/OdinDecoder.h"
#include "OdinCore/include/odin.h"

FOdinSoundGenerator::FOdinSoundGenerator()
    : bIsFinished(false)
{ ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__)); }

FOdinSoundGenerator::~FOdinSoundGenerator()
{ ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__)); }

void FOdinSoundGenerator::ResetConnectedDecoder()
{
    FScopeLock HandleAccess(&NativeHandleAccessSection);
    DecoderCell.Reset();
}

void FOdinSoundGenerator::SetOdinDecoder(UOdinDecoder* InDecoder)
{
    if (InDecoder) {
        this->SampleRate   = InDecoder->SampleRate;
        this->ChannelCount = InDecoder->bStereo ? 2 : 1;
        if (InDecoder->GetNativeHandle() == nullptr) {
            ODIN_LOG(Error, "Native Decoder Handle given in SetOdinDecoder is invalid, Generator won't be able to generate Audio.")
        }
        FScopeLock HandleAccess(&NativeHandleAccessSection);
        DecoderCell = InDecoder->GetHandleCell();

    } else {
        ODIN_LOG(Log, "Input decoder pointer was null. Resetting UObject decoder handle and native decoder handle.");
        ResetConnectedDecoder();
    }

    // a new decoder has nothing to catch up on
    FScopeLock Lock(&CatchUpSection);
    PoppedUntilSeconds = FPlatformTime::Seconds();
}

void FOdinSoundGenerator::Close()
{
    ResetConnectedDecoder();
    bIsFinished = true;
}

int32 FOdinSoundGenerator::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(OdinSoundGenerator::OnGenerateAudio)
    ODIN_LOG(VeryVerbose, "OnGenerateAudio called, requested NumSamples %d", NumSamples);
    {
        // the UE engine pulls in real time (no catchup)
        FScopeLock Lock(&CatchUpSection);
        PoppedUntilSeconds = FPlatformTime::Seconds();
    }

    if (!PopDecoder(OutAudio, NumSamples)) {
        return NumSamples;
    }

    {
        TRACE_CPUPROFILER_EVENT_SCOPE(FOdinSoundGenerator::OnGenerateAudio - AudioBufferListener Broadcast);
        FScopeLock Lock(&CriticalSectionAudioBufferListeners);
        for (auto WeakBufferListenerPtr : this->AudioBufferListeners) {
            if (WeakBufferListenerPtr.IsValid()) {
                const auto BufferListener = WeakBufferListenerPtr.Pin();
                BufferListener->OnGeneratedBuffer(OutAudio, NumSamples, ChannelCount);
            }
        }
    }

    return NumSamples;
}

int32 FOdinSoundGenerator::CatchUpToRealTime()
{ return CatchUpToRealTime(FPlatformTime::Seconds()); }

int32 FOdinSoundGenerator::CatchUpToRealTime(double NowSeconds)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinSoundGenerator::CatchUpToRealTime)
    int32 NumFrames = 0;
    {
        FScopeLock Lock(&CatchUpSection);
        PoppedUntilSeconds = FMath::Max(PoppedUntilSeconds, NowSeconds - CatchUpMaxSeconds);
        const double Lag   = NowSeconds - PoppedUntilSeconds;
        if (Lag <= CatchUpThresholdSeconds) {
            return 0;
        }
        NumFrames = FMath::FloorToInt32((Lag - CatchUpTargetLagSeconds) / FrameSeconds);
        PoppedUntilSeconds += NumFrames * FrameSeconds;
    }

    const int32 NumSamples = NumFrames * FMath::RoundToInt32(FrameSeconds * SampleRate) * ChannelCount;
    FScopeLock  BufferLock(&CatchUpBufferSection);
    if (CatchUpDiscardBuffer.Num() < NumSamples) {
        CatchUpDiscardBuffer.SetNumUninitialized(NumSamples);
    }
    return PopDecoder(CatchUpDiscardBuffer.GetData(), NumSamples) ? NumSamples : 0;
}

bool FOdinSoundGenerator::PopDecoder(float* OutAudio, int32 NumSamples)
{
    TSharedPtr<FOdinDecoderHandleCell, ESPMode::ThreadSafe> Cell;
    {
        FScopeLock HandleAccess(&NativeHandleAccessSection);
        Cell = DecoderCell;
    }
    if (!Cell.IsValid()) {
        return false;
    }

    bool      bIsSilence = false;
    OdinError Result;
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(OdinSoundGenerator::PopDecoder - odin_decoder_pop)

        FScopeLock CellLock(&Cell->Lock);
        if (Cell->Handle == nullptr) {
            return false;
        }
        Result = odin_decoder_pop(Cell->Handle, OutAudio, NumSamples, &bIsSilence);
        ODIN_LOG(VeryVerbose, "odin_decoder_pop called,  Result: %d, IsSilence %s", static_cast<int32>(Result), bIsSilence ? TEXT("True") : TEXT("False"));
    }

    if (Result == ODIN_ERROR_SUCCESS) {
        return true;
    }
    if (Result == ODIN_ERROR_ARGUMENT_INVALID_HANDLE) {
        ODIN_LOG(Log, "Aborting Pop due to invalid native Odin Decoder handle: %s. Resetting connected Decoder.",
                 *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(Result), false));
        ResetConnectedDecoder();
    } else {
        ODIN_LOG(Error, "Aborting Pop due to invalid odin_decoder_pop call: %s",
                 *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(Result), false));
    }
    return false;
}

void FOdinSoundGenerator::OnBeginGenerate()
{ ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__)); }

void FOdinSoundGenerator::OnEndGenerate()
{ ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__)); }

bool FOdinSoundGenerator::IsFinished() const
{ return bIsFinished; }

void FOdinSoundGenerator::AddAudioBufferListener(const TWeakPtr<IAudioBufferListener>& InAudioBufferListener)
{
    if (InAudioBufferListener.IsValid()) {
        FScopeLock Lock(&CriticalSectionAudioBufferListeners);
        AudioBufferListeners.AddUnique(InAudioBufferListener);
    }
}

void FOdinSoundGenerator::RemoveAudioBufferListener(const TWeakPtr<IAudioBufferListener>& InAudioBufferListener)
{
    FScopeLock Lock(&CriticalSectionAudioBufferListeners);
    AudioBufferListeners.Remove(InAudioBufferListener);
}

int32 FOdinSoundGenerator::GetDesiredNumSamplesToRenderPerCallback() const
{
    constexpr int MS = 20;
    return SampleRate / 1000 * MS * ChannelCount;
}