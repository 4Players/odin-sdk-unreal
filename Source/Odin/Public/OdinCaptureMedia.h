#pragma once

#include "OdinLibrary/include/odin.h"

#include "AudioCapture.h"
#include "CoreMinimal.h"
#include "OdinMediaBase.h"
#include "UObject/Object.h"

#include "OdinCaptureMedia.generated.h"

UCLASS(BlueprintType, ClassGroup = Odin)
class ODIN_API UOdinCaptureMedia : public UOdinMediaBase
{
    GENERATED_UCLASS_BODY()

  public:
    void           SetAudioCapture(UAudioCapture *audio_capture);
    void           Reset();
    OdinReturnCode ResetOdinStream();

  protected:
    void BeginDestroy() override;

    UPROPERTY(BlueprintReadOnly)
    UAudioCapture *audio_capture_ = nullptr;

    FAudioGeneratorHandle audio_generator_handle_;
};