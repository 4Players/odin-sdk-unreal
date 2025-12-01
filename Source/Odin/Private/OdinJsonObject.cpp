/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinJsonObject.h"

#include "OdinJsonValue.h"

#include "OdinFunctionLibrary.h"
#include "OdinNative/OdinUtils.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

using namespace OdinUtility;

UOdinJsonObject::UOdinJsonObject(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
    Reset();
}

UOdinJsonObject *UOdinJsonObject::ConstructJsonObject(UObject *WorldContextObject)
{
    return NewObject<UOdinJsonObject>();
}

UOdinJsonObject *UOdinJsonObject::ConstructJsonObjectFromString(UObject *WorldContextObject, const FString &Data)
{
    auto obj = NewObject<UOdinJsonObject>();
    obj->DecodeJson(Data);

    return obj;
}

UOdinJsonObject *UOdinJsonObject::ConstructJsonObjectFromBytes(UObject *WorldContextObject, const TArray<uint8> &Data)
{
    auto obj    = NewObject<UOdinJsonObject>();
    auto s_data = UOdinFunctionLibrary::BytesToString(Data);
    obj->DecodeJson(s_data);
    return obj;
}

void UOdinJsonObject::Reset()
{
    if (JsonObj.IsValid()) {
        JsonObj.Reset();
    }

    JsonObj = MakeShareable(new FJsonObject());
}

TSharedPtr<FJsonObject> &UOdinJsonObject::GetRootObject()
{
    return JsonObj;
}

void UOdinJsonObject::SetRootObject(const TSharedPtr<FJsonObject> &JsonObject)
{
    JsonObj = JsonObject;
}

FString UOdinJsonObject::EncodeJson() const
{
    if (!JsonObj.IsValid()) {
        return TEXT("");
    }

    FString                                OutputString;
    TSharedRef<FCondensedJsonStringWriter> Writer = FCondensedJsonStringWriterFactory::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

    return OutputString;
}

TArray<uint8> UOdinJsonObject::EncodeJsonBytes() const
{
    TArray<uint8> data;
    if (!JsonObj.IsValid()) {
        return data;
    }

    const FString OutputString = EncodeJson();
    UOdinFunctionLibrary::OdinStringToBytes(OutputString, data);
    return data;
}

bool UOdinJsonObject::DecodeJson(const FString &JsonString)
{
    TSharedRef<FCondensedJsonStringReader> Reader = FCondensedJsonStringReaderFactory::Create(*JsonString);
    if (FJsonSerializer::Deserialize(Reader, JsonObj) && JsonObj.IsValid()) {
        return true;
    }

    // If we've failed to deserialize the string, we should clear our internal data
    Reset();

    return false;
}

TArray<FString> UOdinJsonObject::GetFieldNames() const
{
    TArray<FString> Result;

    if (!JsonObj.IsValid()) {
        return Result;
    }

    JsonObj->Values.GetKeys(Result);

    return Result;
}

bool UOdinJsonObject::HasField(const FString &FieldName) const
{
    if (!JsonObj.IsValid()) {
        return false;
    }

    return JsonObj->HasField(FieldName);
}

void UOdinJsonObject::RemoveField(const FString &FieldName)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    JsonObj->RemoveField(FieldName);
}

UOdinJsonValue *UOdinJsonObject::GetField(const FString &FieldName) const
{
    if (!JsonObj.IsValid()) {
        return nullptr;
    }

    TSharedPtr<FJsonValue> NewVal = JsonObj->TryGetField(FieldName);

    UOdinJsonValue *NewValue = NewObject<UOdinJsonValue>();
    NewValue->SetRootValue(NewVal);

    return NewValue;
}

void UOdinJsonObject::SetField(const FString &FieldName, UOdinJsonValue *JsonValue)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    JsonObj->SetField(FieldName, JsonValue->GetRootValue());
}

void UOdinJsonObject::SetFieldNull(const FString &FieldName)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    TSharedPtr<FJsonValue> myNull = MakeShareable(new FJsonValueNull());

    JsonObj->SetField(FieldName, myNull);
}

float UOdinJsonObject::GetNumberField(const FString &FieldName) const
{
    if (!JsonObj.IsValid()) {
        return 0.0f;
    }

    return JsonObj->GetNumberField(FieldName);
}

void UOdinJsonObject::SetNumberField(const FString &FieldName, float Number)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    JsonObj->SetNumberField(FieldName, Number);
}

FString UOdinJsonObject::GetStringField(const FString &FieldName) const
{
    if (!JsonObj.IsValid()) {
        return TEXT("");
    }

    return JsonObj->GetStringField(FieldName);
}

void UOdinJsonObject::SetStringField(const FString &FieldName, const FString &StringValue)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    JsonObj->SetStringField(FieldName, StringValue);
}

bool UOdinJsonObject::GetBoolField(const FString &FieldName) const
{
    if (!JsonObj.IsValid()) {
        return false;
    }

    return JsonObj->GetBoolField(FieldName);
}

void UOdinJsonObject::SetBoolField(const FString &FieldName, bool InValue)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    JsonObj->SetBoolField(FieldName, InValue);
}

TArray<UOdinJsonValue *> UOdinJsonObject::GetArrayField(const FString &FieldName) const
{
    TArray<UOdinJsonValue *> OutArray;
    if (!JsonObj.IsValid()) {
        return OutArray;
    }

    TArray<TSharedPtr<FJsonValue>> ValArray = JsonObj->GetArrayField(FieldName);
    for (auto Value : ValArray) {
        UOdinJsonValue *NewValue = NewObject<UOdinJsonValue>();
        NewValue->SetRootValue(Value);

        OutArray.Add(NewValue);
    }

    return OutArray;
}

void UOdinJsonObject::SetArrayField(const FString &FieldName, const TArray<UOdinJsonValue *> &InArray)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    TArray<TSharedPtr<FJsonValue>> ValArray;

    for (auto InVal : InArray) {
        TSharedPtr<FJsonValue> JsonVal = InVal->GetRootValue();

        switch (InVal->GetType()) {
            case EOJson::None:
                break;

            case EOJson::Null:
                ValArray.Add(MakeShareable(new FJsonValueNull()));
                break;

            case EOJson::String:
                ValArray.Add(MakeShareable(new FJsonValueString(JsonVal->AsString())));
                break;

            case EOJson::Number:
                ValArray.Add(MakeShareable(new FJsonValueNumber(JsonVal->AsNumber())));
                break;

            case EOJson::Boolean:
                ValArray.Add(MakeShareable(new FJsonValueBoolean(JsonVal->AsBool())));
                break;

            case EOJson::Array:
                ValArray.Add(MakeShareable(new FJsonValueArray(JsonVal->AsArray())));
                break;

            case EOJson::Object:
                ValArray.Add(MakeShareable(new FJsonValueObject(JsonVal->AsObject())));
                break;

            default:
                break;
        }
    }

    JsonObj->SetArrayField(FieldName, ValArray);
}

void UOdinJsonObject::MergeJsonObject(UOdinJsonObject *InJsonObject, bool Overwrite)
{
    TArray<FString> Keys = InJsonObject->GetFieldNames();

    for (auto Key : Keys) {
        if (Overwrite == false && HasField(Key)) {
            continue;
        }

        SetField(Key, InJsonObject->GetField(Key));
    }
}

UOdinJsonObject *UOdinJsonObject::GetObjectField(const FString &FieldName) const
{
    if (!JsonObj.IsValid()) {
        return nullptr;
    }

    TSharedPtr<FJsonObject> JsonObjField = JsonObj->GetObjectField(FieldName);

    UOdinJsonObject *OutRestJsonObj = NewObject<UOdinJsonObject>();
    OutRestJsonObj->SetRootObject(JsonObjField);

    return OutRestJsonObj;
}

void UOdinJsonObject::SetObjectField(const FString &FieldName, UOdinJsonObject *JsonObject)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    JsonObj->SetObjectField(FieldName, JsonObject->GetRootObject());
}

TArray<float> UOdinJsonObject::GetNumberArrayField(const FString &FieldName)
{
    TArray<float> NumberArray;

    if (!JsonObj.IsValid()) {
        return NumberArray;
    }

    TArray<TSharedPtr<FJsonValue>> JsonArrayValues = JsonObj->GetArrayField(FieldName);
    for (FCondensedJsonArrayIterator It(JsonArrayValues); It; ++It) {
        NumberArray.Add((*It)->AsNumber());
    }

    return NumberArray;
}

void UOdinJsonObject::SetNumberArrayField(const FString &FieldName, const TArray<float> &NumberArray)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    TArray<TSharedPtr<FJsonValue>> EntriesArray;

    for (auto Number : NumberArray) {
        EntriesArray.Add(MakeShareable(new FJsonValueNumber(Number)));
    }

    JsonObj->SetArrayField(FieldName, EntriesArray);
}

TArray<FString> UOdinJsonObject::GetStringArrayField(const FString &FieldName)
{
    TArray<FString> StringArray;

    if (!JsonObj.IsValid()) {
        return StringArray;
    }

    TArray<TSharedPtr<FJsonValue>> JsonArrayValues = JsonObj->GetArrayField(FieldName);
    for (FCondensedJsonArrayIterator It(JsonArrayValues); It; ++It) {
        StringArray.Add((*It)->AsString());
    }

    return StringArray;
}

void UOdinJsonObject::SetStringArrayField(const FString &FieldName, const TArray<FString> &StringArray)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    TArray<TSharedPtr<FJsonValue>> EntriesArray;

    for (auto String : StringArray) {
        EntriesArray.Add(MakeShareable(new FJsonValueString(String)));
    }

    JsonObj->SetArrayField(FieldName, EntriesArray);
}

TArray<bool> UOdinJsonObject::GetBoolArrayField(const FString &FieldName)
{
    TArray<bool> BoolArray;

    if (!JsonObj.IsValid()) {
        return BoolArray;
    }

    TArray<TSharedPtr<FJsonValue>> JsonArrayValues = JsonObj->GetArrayField(FieldName);
    for (FCondensedJsonArrayIterator It(JsonArrayValues); It; ++It) {
        BoolArray.Add((*It)->AsBool());
    }

    return BoolArray;
}

void UOdinJsonObject::SetBoolArrayField(const FString &FieldName, const TArray<bool> &BoolArray)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    TArray<TSharedPtr<FJsonValue>> EntriesArray;

    for (auto Boolean : BoolArray) {
        EntriesArray.Add(MakeShareable(new FJsonValueBoolean(Boolean)));
    }

    JsonObj->SetArrayField(FieldName, EntriesArray);
}

TArray<UOdinJsonObject *> UOdinJsonObject::GetObjectArrayField(const FString &FieldName) const
{
    TArray<UOdinJsonObject *> OutArray;

    if (!JsonObj.IsValid()) {
        return OutArray;
    }

    TArray<TSharedPtr<FJsonValue>> ValArray = JsonObj->GetArrayField(FieldName);
    for (auto Value : ValArray) {
        TSharedPtr<FJsonObject> NewObj = Value->AsObject();

        UOdinJsonObject *NewJson = NewObject<UOdinJsonObject>();
        NewJson->SetRootObject(NewObj);

        OutArray.Add(NewJson);
    }

    return OutArray;
}

void UOdinJsonObject::SetObjectArrayField(const FString &FieldName, const TArray<UOdinJsonObject *> &ObjectArray)
{
    if (!JsonObj.IsValid()) {
        return;
    }

    TArray<TSharedPtr<FJsonValue>> EntriesArray;

    for (auto Value : ObjectArray) {
        EntriesArray.Add(MakeShareable(new FJsonValueObject(Value->GetRootObject())));
    }

    JsonObj->SetArrayField(FieldName, EntriesArray);
}
