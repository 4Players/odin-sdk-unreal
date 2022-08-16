/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#pragma once

#include "AudioCapture.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OdinRoom.h"

#include "OdinFunctionLibrary.generated.h"

UCLASS()
class ODIN_API UOdinFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_UCLASS_BODY()

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Statics")
    static UOdinFunctionLibrary *getOdinFunctionLibrary();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Function")
    static UOdinCaptureMedia *Odin_CreateMedia(UAudioCapture *audioCapture);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Function")
    static FString GenerateAccessKey();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Function")
    static FString FormatError(int32 code);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Function")
    static FString BytesToString(const TArray<uint8> &data);
};
