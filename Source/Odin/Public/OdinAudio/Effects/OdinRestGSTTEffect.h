/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinCustomEffect.h"
#include "SampleBuffer.h"
#include "Sound/SampleBufferIO.h"
#include "Engine/TimerHandle.h"
#include "OdinRestGSTTEffect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinRestGSTTEffectResponse, FString, response);

class UOdinPipeline;

/**
 * Codec-Effect for the odin audio pipeline.
 * The effect will cache audio in a file based on a timer.
 * Used in a remote web sample check for speech-to-text content check.
 * @deprecated This will be included or replaced by the 4Players/ue-filter-plugin extension.
 */
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRestGSTTEffect : public UOdinCustomEffect
{
    GENERATED_BODY()

  public:
    UOdinRestGSTTEffect(const class FObjectInitializer& PCIP);
    virtual void CustomEffect(const TArrayView<float>& InSamples, bool*& bIsSilent,
                              TOdinCustomEffectUserData<UOdinCustomEffect>* const InUserData) const override;

    virtual void Callback(Audio::FAlignedFloatBuffer Buffer, FString Name, FString NameSpace) const {}

    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Construct Generic STT Effect", ToolTip = "Creates a REST generic speech to text effect", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline|Effects")
    static UOdinRestGSTTEffect* ConstructRestGSTTEffect(UObject* WorldContextObject);

    void PostRequest(const FString& Endpoint, const FString& Payload, const FString& ContentType);
    bool Remix(const float* InAudio, int32 NumSamples, int32 NumChannels, int32 SourceSampleRate, int32 TargetSampleRate, Audio::FAlignedFloatBuffer& OutAudio);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinRestGSTTEffectResponse PostResponse;

    UPROPERTY(BlueprintReadWrite, Category = "Odin|Effects")
    int32 SampleRate = 48000;
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Effects")
    int32 ResampleRate = 16000;
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Effects")
    bool bStereo = false;
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Effects")
    float Timer = 3;

    TOdinCustomEffectUserData<UOdinRestGSTTEffect> UserData;

  protected:
    bool         WriteWav = false;
    FTimerHandle TimerHandle;
    void         TimerCallback();

  private:
    Audio::TSampleBuffer<float> AudioBuffer;
    Audio::FSoundWavePCMWriter  Writer;
    virtual void                BeginDestroy() override;
};