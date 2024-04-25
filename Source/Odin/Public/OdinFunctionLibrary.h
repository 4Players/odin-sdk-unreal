/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "AudioCapture.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OdinRoom.h"

#include "OdinFunctionLibrary.generated.h"

UCLASS()
class ODIN_API UOdinFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_UCLASS_BODY()

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Static")
    static UOdinFunctionLibrary *getOdinFunctionLibrary();

    UFUNCTION(BlueprintCallable, BlueprintPure,
              meta = (Category = "Odin|Sound", DisplayName = "Construct Local Media"))
    static UOdinCaptureMedia *Odin_CreateMedia(UPARAM(ref) UAudioCapture *&audioCapture);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Authentication")
    static FString GenerateAccessKey();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Error Handling")
    static FString FormatOdinError(int64 code, bool ueTrace);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Error Handling")
    static FString FormatError(int32 code, bool ueTrace);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Custom Data")
    static FString BytesToString(const TArray<uint8> &data);

    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture",
              meta = (WorldContext = "WorldContextObject"))
    static class UOdinAudioCapture *CreateOdinAudioCapture(UObject *WorldContextObject);

    static bool Check(const TWeakObjectPtr<UObject> ObjectToCheck,
                      const FString                &CheckReferenceName);
};
