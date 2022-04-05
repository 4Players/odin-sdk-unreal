#pragma once

#include "OdinLibrary/include/odin.h"

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

    OdinMediaStreamHandle GetMediaHandle()
    {
        return this->stream_handle_;
    }

  protected:
    void BeginDestroy() override;

    OdinMediaStreamHandle stream_handle_ = 0;

    UPROPERTY(BlueprintReadOnly)
    UAudioCapture *audio_capture_ = nullptr;

    FAudioGeneratorHandle audio_generator_handle_;
};