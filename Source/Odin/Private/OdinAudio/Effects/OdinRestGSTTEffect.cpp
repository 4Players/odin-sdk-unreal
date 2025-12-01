/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/Effects/OdinRestGSTTEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HttpFwd.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Runtime/Launch/Resources/Version.h"

UOdinRestGSTTEffect::UOdinRestGSTTEffect(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
    UserData = TOdinCustomEffectUserData(this);
}

void UOdinRestGSTTEffect::CustomEffect(const TArrayView<float> &InSamples, bool *&bIsSilent,
                                       TOdinCustomEffectUserData<UOdinCustomEffect> *const InUserData) const
{
    // nothing to do if there is no pipeline anymore
    if (GetParent().IsValid() == false)
        return;

    if (auto effect = InUserData->Root.Get()) {
        auto self = static_cast<UOdinRestGSTTEffect *>(effect);

        if (self->TimerHandle.IsValid())
            self->AudioBuffer.Append(InSamples.GetData(), InSamples.Num(), bStereo ? 2 : 1, SampleRate);
        else {
            GetWorld()->GetTimerManager().SetTimer(self->TimerHandle, self, &UOdinRestGSTTEffect::TimerCallback, self->Timer, false);
        }
    }
}

UOdinRestGSTTEffect *UOdinRestGSTTEffect::ConstructRestGSTTEffect(UObject *WorldContextObject)
{
    UOdinRestGSTTEffect *result = NewObject<UOdinRestGSTTEffect>(WorldContextObject);
    return result;
}

void UOdinRestGSTTEffect::TimerCallback()
{

    Audio::FAlignedFloatBuffer buffer;
    if (Remix(AudioBuffer.GetData(), AudioBuffer.GetNumSamples(), AudioBuffer.GetNumChannels(), AudioBuffer.GetSampleRate(), ResampleRate, buffer)) {
        FString name = "buffer", path = "GSTT";
        if (WriteWav) {
            const Audio::FSampleBuffer wav = Audio::TSampleBuffer<int16>(buffer, 1, ResampleRate);
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
            Writer.SynchronouslyWriteToWavFile(wav, name, path);
#else
            Writer.SynchronouslyWriteSoundWave(wav, &name, &path);
#endif
        }
        Callback(buffer, name, path);
    }

    AudioBuffer.Reset();
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
}

void UOdinRestGSTTEffect::BeginDestroy()
{
    Super::BeginDestroy();
    AudioBuffer.Reset();
}

/// <summary>
/// POST Rest API ai-service
/// </summary>
/// <param name="Endpoint">service location i.e 'https://example.api/speechtotext/transcriptions:transcribe?'</param>
/// <param name="Payload">request content i.e 'audio=@"AudioFile"%26definition="{ "locales" : ["en-US"]}"'</param>
/// <param name="ContentType">i.e 'application/x-www-form-urlencoded'</param>
void UOdinRestGSTTEffect::PostRequest(const FString &Endpoint, const FString &Payload, const FString &ContentType)
{
    FHttpModule                                  &httpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest   = httpModule.CreateRequest();
    pRequest->SetVerb(TEXT("POST"));
    pRequest->SetHeader(TEXT("Content-Type"), ContentType);

    pRequest->SetContentAsString(Payload);
    pRequest->SetURL(Endpoint);

    // raw response
    pRequest->OnProcessRequestComplete().BindLambda([&](FHttpRequestPtr completedRequest, FHttpResponsePtr pResponse, bool connectedSuccessfully) mutable {
        if (connectedSuccessfully) {
            PostResponse.Broadcast(pResponse->GetContentAsString());
        } else {
            switch (completedRequest->GetStatus()) {
                case EHttpRequestStatus::Failed:
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
                    if (completedRequest->GetFailureReason() == EHttpFailureReason::ConnectionError) {
                        UE_LOG(LogTemp, Warning, TEXT("UOdinRestGSTTEffect: Connection failed."));
                    }
#else
                    if (completedRequest->GetStatus() == EHttpRequestStatus::Failed_ConnectionError) {
                        UE_LOG(LogTemp, Warning, TEXT("UOdinRestGSTTEffect: Connection failed."));
                    }
#endif
                default:
                    UE_LOG(LogTemp, Warning, TEXT("UOdinRestGSTTEffect: Request failed."));
            }
        }
    });
    pRequest->ProcessRequest();
}

bool UOdinRestGSTTEffect::Remix(const float *InAudio, int32 NumSamples, int32 NumChannels, int32 SourceSampleRate, int32 TargetSampleRate,
                                Audio::FAlignedFloatBuffer &OutAudio)
{
    if (!NumSamples) {
        UE_LOG(LogTemp, Error, TEXT("UOdinRestGSTTEffect: No samples to downmix/resample."));
        return false;
    }

    // downmix
    Audio::FAlignedFloatBuffer monoSamples = Audio::FAlignedFloatBuffer();
    monoSamples.SetNum((NumSamples / NumChannels));
    for (int32 sampleCnt = 0; sampleCnt < NumSamples; sampleCnt += NumChannels) {
        float sum = 0.f;
        for (int32 channelCnt = 0; channelCnt < NumChannels; ++channelCnt)
            sum += InAudio[sampleCnt + channelCnt];
        monoSamples.Add(sum / NumChannels);
    }

    // resample
    const double ratio  = double(SourceSampleRate) / TargetSampleRate;
    const int32  newNum = FMath::CeilToInt(monoSamples.Num() / ratio);
    OutAudio.SetNum(newNum);
    for (int32 i = 0; i < newNum; ++i) {
        double srcIdx = i * ratio;
        int32  index  = FMath::FloorToInt(srcIdx);

        float sampleValueAtStart = monoSamples[FMath::Clamp(index, 0, monoSamples.Num() - 1)];
        float sampleValueAtEnd   = monoSamples[FMath::Clamp(index + 1, 0, monoSamples.Num() - 1)];

        double frac = srcIdx - index;
        OutAudio[i] = (sampleValueAtStart + (sampleValueAtEnd - sampleValueAtStart) * frac);
    }

    return true;
}