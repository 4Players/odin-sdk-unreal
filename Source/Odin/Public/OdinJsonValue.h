/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"
#include "OdinJsonValue.generated.h"

class UOdinJsonObject;

/**
 * Represents all the types a Json Value can be.
 */
UENUM(BlueprintType)
namespace EOJson
{
enum Type {
    None,
    Null,
    String,
    Number,
    Boolean,
    Array,
    Object,
};
}

/**
 * Blueprintable FJsonValue wrapper
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = Odin)
class ODIN_API UOdinJsonValue : public UObject
{
    GENERATED_UCLASS_BODY()

    UFUNCTION(BlueprintPure,
              meta = (DisplayName = "Construct Json Number Value", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json Utilities")
    static UOdinJsonValue *ConstructJsonValueNumber(UObject *WorldContextObject, float Number);

    UFUNCTION(BlueprintPure,
              meta = (DisplayName = "Construct Json String Value", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json Utilities")
    static UOdinJsonValue *ConstructJsonValueString(UObject       *WorldContextObject,
                                                    const FString &StringValue);

    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "Construct Json Bool Value", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json Utilities")
    static UOdinJsonValue *ConstructJsonValueBool(UObject *WorldContextObject, bool InValue);

    UFUNCTION(BlueprintPure,
              meta = (DisplayName = "Construct Json Array Value", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json Utilities")
    static UOdinJsonValue *ConstructJsonValueArray(UObject *WorldContextObject,
                                                   const TArray<UOdinJsonValue *> &InArray);

    UFUNCTION(BlueprintPure,
              meta = (DisplayName = "Construct Json Object Value", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json Utilities")
    static UOdinJsonValue *ConstructJsonValueObject(UObject         *WorldContextObject,
                                                    UOdinJsonObject *JsonObject);

    static UOdinJsonValue *ConstructJsonValue(UObject                      *WorldContextObject,
                                              const TSharedPtr<FJsonValue> &InValue);

    TSharedPtr<FJsonValue> &GetRootValue();

    void SetRootValue(TSharedPtr<FJsonValue> &JsonValue);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    EOJson::Type GetType() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    FString GetTypeString() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    bool IsNull() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    float AsNumber() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    FString AsString() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    bool AsBool() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    TArray<UOdinJsonValue *> AsArray() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    UOdinJsonObject *AsObject();

  private:
    TSharedPtr<FJsonValue> JsonVal;

  protected:
    /** Simple error logger */
    void ErrorMessage(const FString &InType) const;
};
