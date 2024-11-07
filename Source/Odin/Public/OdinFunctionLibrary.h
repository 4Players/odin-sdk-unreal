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
    /**
     * @brief Returns the singleton instance of UOdinFunctionLibrary.
     *
     * @return UOdinFunctionLibrary* - The singleton instance of UOdinFunctionLibrary.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Static")
    static UOdinFunctionLibrary *getOdinFunctionLibrary();

    /**
     * @brief Creates an Odin media instance with the provided audio generator object. Sets up the
     * Odin media stream based on the generator object's sample rate and channel count.
     *
     * @param audioCapture The audio generator object used (i.e. an Audio Capture object for
     * capturing microphone data) for setting up Odin media stream based on the generator object's
     * sample rate and channel count.
     *
     * @return The created UOdinCaptureMedia instance.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure,
              meta = (Category = "Odin|Sound", DisplayName = "Construct Local Media"))
    static UOdinCaptureMedia *Odin_CreateMedia(UPARAM(ref) UAudioGenerator *&audioCapture);

    /**
     * @brief Generate a new access key using the Odin SDK.
     *
     * This method generates a new access key using the Odin SDK. The access key is a unique
     * identifier that can be used for authentication purposes.
     *
     * @warning It is ok for development and testing purposes to create access keys room tokens on
     * client side - but in production you should switch to server side generation of room tokens!
     *
     * @return The generated access key as a FString. The format of the access key may vary
     * depending on the engine version.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Authentication")
    static FString GenerateAccessKey();

    /**
     * Formats an Odin error message based on the given error code and additional flag to include
     * Unreal Engine stack trace.
     *
     * @param code The error code to be formatted.
     * @param ueTrace Flag indicating whether to include Unreal Engine stack trace in the error
     * message.
     *
     * @return Formatted error message as a FString.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Error Handling")
    static FString FormatOdinError(int64 code, bool ueTrace);

    /**
     * @param code The error code to be formatted.
     * @param ueTrace Indicates whether to include UE stack trace in the log.
     * @return The formatted error message as a string.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Error Handling")
    static FString FormatError(int32 code, bool ueTrace);

    /**
     * Converts an array of bytes to a string representation.
     *
     * @param data The array of bytes to convert to a string.
     * @return A string representation of the data.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Custom Data")
    static FString BytesToString(const TArray<uint8> &data);

    /**
     * @param WorldContextObject The world context object from which to create the OdinAudioCapture
     * instance.
     * @return A pointer to the created OdinAudioCapture instance if successful, nullptr otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture",
              meta = (WorldContext = "WorldContextObject"))
    static class UOdinAudioCapture *CreateOdinAudioCapture(UObject *WorldContextObject);

    /**
     * @param ObjectToCheck A weak object pointer to check for validity.
     * @param CheckReferenceName The reference name associated with the object being checked. Used
     * for debug logging, if the checked Reference Name is invalid.
     * @return True if the object is valid, false if it is invalid.
     */
    static bool Check(const TWeakObjectPtr<UObject> ObjectToCheck,
                      const FString                &CheckReferenceName);
};
