/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

#include "OdinJsonObject.generated.h"

class UOdinJsonValue;

UCLASS(BlueprintType, Blueprintable, ClassGroup = Odin)
class ODIN_API UOdinJsonObject : public UObject
{
    GENERATED_BODY()

  public:
    UOdinJsonObject(const class FObjectInitializer& PCIP);

    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Construct Json Object", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json Utilities")
    static UOdinJsonObject* ConstructJsonObject(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Json Object From String", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json Utilities")
    static UOdinJsonObject* ConstructJsonObjectFromString(UObject* WorldContextObject, const FString& Data);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Json Object From Bytes", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json Utilities")
    static UOdinJsonObject* ConstructJsonObjectFromBytes(UObject* WorldContextObject, const TArray<uint8>& Data);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void Reset();

    TSharedPtr<FJsonObject>& GetRootObject();

    void SetRootObject(const TSharedPtr<FJsonObject>& JsonObject);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    FString EncodeJson() const;

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    TArray<uint8> EncodeJsonBytes() const;

    /**
     * Decodes a JSON string. If the provided string is successfully parsed into a valid JSON object, it updates the internal structure of this object.
     * If parsing fails, the internal state is reset to clear any existing data.
     *
     * @param JsonString The JSON formatted string to be decoded.
     * @return true if the JSON string was successfully parsed and decoded, false if the decoding failed.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    bool DecodeJson(const FString& JsonString);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    TArray<FString> GetFieldNames() const;

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    bool HasField(const FString& FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void RemoveField(const FString& FieldName);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    UOdinJsonValue* GetField(const FString& FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetField(const FString& FieldName, UOdinJsonValue* JsonValue);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetFieldNull(const FString& FieldName);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    TArray<UOdinJsonValue*> GetArrayField(const FString& FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetArrayField(const FString& FieldName, const TArray<UOdinJsonValue*>& InArray);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void MergeJsonObject(UOdinJsonObject* InJsonObject, bool Overwrite);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    float GetNumberField(const FString& FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetNumberField(const FString& FieldName, float Number);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    FString GetStringField(const FString& FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetStringField(const FString& FieldName, const FString& StringValue);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    bool GetBoolField(const FString& FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetBoolField(const FString& FieldName, bool InValue);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    UOdinJsonObject* GetObjectField(const FString& FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetObjectField(const FString& FieldName, UOdinJsonObject* JsonObject);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    TArray<float> GetNumberArrayField(const FString& FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetNumberArrayField(const FString& FieldName, const TArray<float>& NumberArray);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    TArray<FString> GetStringArrayField(const FString& FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetStringArrayField(const FString& FieldName, const TArray<FString>& StringArray);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    TArray<bool> GetBoolArrayField(const FString& FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetBoolArrayField(const FString& FieldName, const TArray<bool>& BoolArray);

    UFUNCTION(BlueprintPure, Category = "Odin|Json Utilities")
    TArray<UOdinJsonObject*> GetObjectArrayField(const FString& FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json Utilities")
    void SetObjectArrayField(const FString& FieldName, const TArray<UOdinJsonObject*>& ObjectArray);

  private:
    TSharedPtr<FJsonObject> JsonObj;
};