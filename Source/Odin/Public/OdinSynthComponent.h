#pragma once

#include "Components/SynthComponent.h"
#include "CoreMinimal.h"

#include "OdinPlaybackMedia.h"

#include "OdinSynthComponent.generated.h"

class OdinMediaSoundGenerator;

UCLASS(ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class UOdinSynthComponent : public USynthComponent
{
    GENERATED_BODY()

  public:
    UFUNCTION(BlueprintCallable, Category = "Odin|Function|Sound")
    void Odin_AssignSynthToMedia(UOdinPlaybackMedia *media);

  protected:
    bool Init(int32 &SampleRate) override;
    void BeginDestroy() override;

  protected:
#if ENGINE_MAJOR_VERSION >= 5
    virtual ISoundGeneratorPtr
    CreateSoundGenerator(const FSoundGeneratorInitParams &InParams) override;
#else
    virtual ISoundGeneratorPtr CreateSoundGenerator(int32 InSampleRate,
                                                    int32 InNumChannels) override;
#endif
  private:
    OdinMediaStreamHandle pending_stream_handle_ = 0;

    UPROPERTY()
    UOdinPlaybackMedia *playback_media_ = nullptr;

    TSharedPtr<OdinMediaSoundGenerator, ESPMode::ThreadSafe> sound_generator_;
};