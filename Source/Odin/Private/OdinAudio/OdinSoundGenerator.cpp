/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinSoundGenerator.h"

#include "OdinFunctionLibrary.h"
#include "OdinVoice.h"
#include "Components/SynthComponent.h"
#include "OdinAudio/OdinDecoder.h"
#include "OdinCore/include/odin.h"

FOdinSoundGenerator::FOdinSoundGenerator()
    : NativeDecoderHandle(nullptr)
    , bIsFinished(false)
{
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

FOdinSoundGenerator::~FOdinSoundGenerator()
{
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

void FOdinSoundGenerator::SetOdinDecoder(UOdinDecoder* InDecoder)
{
    if (InDecoder) {
        OdinDecoderHandle  = InDecoder->GetHandle();
        this->SampleRate   = InDecoder->SampleRate;
        this->ChannelCount = InDecoder->bStereo ? 2 : 1;
        if (OdinDecoderHandle.IsValid()) {
            FScopeLock HandleAccess(&NativeHandleAccessSection);
            NativeDecoderHandle = reinterpret_cast<OdinDecoder*>(OdinDecoderHandle->GetHandle());
        } else {
            ODIN_LOG(Error, "Native Decoder Handle given in SetOdinDecoder is invalid, Generator won't be able to generate Audio.")
        }

    } else {
        ODIN_LOG(Error, "Tried using a nullptr as input. Please provide a valid UOdinDecoder pointer.");
    }
}

void FOdinSoundGenerator::Close()
{
    OdinDecoderHandle.Reset();
    {
        FScopeLock HandleAccess(&NativeHandleAccessSection);
        NativeDecoderHandle = nullptr;
    }

    bIsFinished = true;
}

int32 FOdinSoundGenerator::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(OdinSoundGenerator::OnGenerateAudio)
    ODIN_LOG(VeryVerbose, "OnGenerateAudio called, requested NumSamples %d", NumSamples);
    if (nullptr == NativeDecoderHandle) {
        return 0;
    }

    bool      bIsSilence = false;
    OdinError Result;
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(OdinSoundGenerator::OnGenerateAudio - odin_decoder_pop)

        FScopeLock HandleAccess(&NativeHandleAccessSection);
        Result = odin_decoder_pop(NativeDecoderHandle, OutAudio, NumSamples, &bIsSilence);
        ODIN_LOG(VeryVerbose, "odin_decoder_pop called,  Result: %d, IsSilence %s", static_cast<int32>(Result), bIsSilence ? TEXT("True") : TEXT("False"));
    }

    int32 NumGeneratedSamples;
    if (Result == ODIN_ERROR_SUCCESS) {
        NumGeneratedSamples = NumSamples;
    } else {
        if (Result != ODIN_ERROR_NO_DATA) {
            ODIN_LOG(Error, "Aborting Pop due to invalid odin_decoder_pop call: %s",
                     *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(Result), false));
        }
        NumGeneratedSamples = 0;
    }

    if (NumGeneratedSamples > NumSamples) {
        ODIN_LOG(Verbose, "Error while reading data from Odin in OdinSoundGenerator::OnGenerateAudio "
                          "number of read samples returned by Odin is larger than requested number of "
                          "samples.")
        return 0;
    }

    if (NumGeneratedSamples > 0) {
        TRACE_CPUPROFILER_EVENT_SCOPE(FOdinSoundGenerator::OnGenerateAudio - AudioBufferListener Broadcast);
        FScopeLock Lock(&CriticalSectionAudioBufferListeners);
        for (auto WeakBufferListenerPtr : this->AudioBufferListeners) {
            if (WeakBufferListenerPtr.IsValid()) {
                const auto BufferListener = WeakBufferListenerPtr.Pin();
                BufferListener->OnGeneratedBuffer(OutAudio, NumSamples, ChannelCount);
            }
        }
    }

    return NumGeneratedSamples;
}

void FOdinSoundGenerator::OnBeginGenerate()
{
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

void FOdinSoundGenerator::OnEndGenerate()
{
    ODIN_LOG(Verbose, "%s", ANSI_TO_TCHAR(__FUNCTION__));
}

bool FOdinSoundGenerator::IsFinished() const
{
    return bIsFinished;
}

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