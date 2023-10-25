/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

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
    void           SetRoom(UOdinRoom* connected_room);
    void           RemoveRoom();
    void           SetAudioCapture(UAudioCapture* audio_capture);
    void           Reset();
    OdinReturnCode ResetOdinStream();

  protected:
    virtual void BeginDestroy() override;

    UPROPERTY(BlueprintReadOnly, Category = "Audio Capture")
    UAudioCapture* audio_capture_ = nullptr;

  private:
    void HandleInputDeviceChanges();

    FCriticalSection capture_generator_delegate_;

    FAudioGeneratorHandle audio_generator_handle_;

    TWeakObjectPtr<UOdinRoom> connected_room_;

    int32 stream_sample_rate_  = 48000;
    int32 stream_num_channels_ = 1;

    FThreadSafeBool bIsBeingReset = false;
};
