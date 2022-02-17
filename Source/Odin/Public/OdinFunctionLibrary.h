#pragma once

#include "AudioCapture.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OdinRoom.h"

#include "OdinFunctionLibrary.generated.h"

UCLASS()
class UOdinFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_UCLASS_BODY()

    UFUNCTION(BlueprintCallable, Category = "Odin|Statics")
    static UOdinFunctionLibrary *getOdinFunctionLibrary();

    UFUNCTION(BlueprintCallable, Category = "Odin|Function")
    static UOdinCaptureMedia *Odin_CreateMedia(UAudioCapture *audioCapture);

    UFUNCTION(BlueprintCallable, Category = "Odin|Function")
    static FString GenerateAccessKey();
};