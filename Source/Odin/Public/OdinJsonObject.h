/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

#include "OdinJsonObject.generated.h"

UCLASS(BlueprintType, Blueprintable, ClassGroup = Odin)
class ODIN_API UOdinJsonObject : public UObject
{
    GENERATED_UCLASS_BODY()

  public:
    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "Construct Json Object", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json")
    static UOdinJsonObject *ConstructJsonObject(UObject *WorldContextObject);

    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "Construct Json Object From String",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json")
    static UOdinJsonObject *ConstructJsonObjectFromString(UObject *WorldContextObject,
                                                          FString  data);

    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "Construct Json Object From Bytes",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Json")
    static UOdinJsonObject *ConstructJsonObjectFromBytes(UObject             *WorldContextObject,
                                                         const TArray<uint8> &data);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void Reset();

    TSharedPtr<FJsonObject> &GetRootObject();

    void SetRootObject(TSharedPtr<FJsonObject> &JsonObject);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    FString EncodeJson() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    TArray<uint8> EncodeJsonBytes() const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    bool DecodeJson(const FString &JsonString);

    UFUNCTION(BlueprintPure, Category = "Odin|Json")
    TArray<FString> GetFieldNames();

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    bool HasField(const FString &FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void RemoveField(const FString &FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    UOdinJsonValue *GetField(const FString &FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetField(const FString &FieldName, UOdinJsonValue *JsonValue);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetFieldNull(const FString &FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    TArray<UOdinJsonValue *> GetArrayField(const FString &FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetArrayField(const FString &FieldName, const TArray<UOdinJsonValue *> &InArray);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void MergeJsonObject(UOdinJsonObject *InJsonObject, bool Overwrite);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    float GetNumberField(const FString &FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetNumberField(const FString &FieldName, float Number);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    FString GetStringField(const FString &FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetStringField(const FString &FieldName, const FString &StringValue);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    bool GetBoolField(const FString &FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetBoolField(const FString &FieldName, bool InValue);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    UOdinJsonObject *GetObjectField(const FString &FieldName) const;

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetObjectField(const FString &FieldName, UOdinJsonObject *JsonObject);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    TArray<float> GetNumberArrayField(const FString &FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetNumberArrayField(const FString &FieldName, const TArray<float> &NumberArray);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    TArray<FString> GetStringArrayField(const FString &FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetStringArrayField(const FString &FieldName, const TArray<FString> &StringArray);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    TArray<bool> GetBoolArrayField(const FString &FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetBoolArrayField(const FString &FieldName, const TArray<bool> &BoolArray);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    TArray<UOdinJsonObject *> GetObjectArrayField(const FString &FieldName);

    UFUNCTION(BlueprintCallable, Category = "Odin|Json")
    void SetObjectArrayField(const FString                   &FieldName,
                             const TArray<UOdinJsonObject *> &ObjectArray);

  private:
    TSharedPtr<FJsonObject> JsonObj;
};
