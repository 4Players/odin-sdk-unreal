#pragma once

#include "AudioCapture.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "OdinCaptureMedia.generated.h"

UCLASS()
class UOdinCaptureMedia : public UObject
{
    GENERATED_UCLASS_BODY()

  public:
    void SetAudioCapture(UAudioCapture *audio_capture);
    void Reset();

    struct OdinMediaStream *GetMedia()
    {
        return this->stream_;
    }

  protected:
    void BeginDestroy() override;

    struct OdinMediaStream *stream_ = nullptr;

    UPROPERTY()
    UAudioCapture *audio_capture_ = nullptr;

    FAudioGeneratorHandle audio_generator_handle_;
};