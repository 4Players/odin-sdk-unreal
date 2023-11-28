/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinJsonValue.h"
#include "CoreMinimal.h"
#include "OdinJsonObject.h"

UOdinJsonValue::UOdinJsonValue(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

UOdinJsonValue *UOdinJsonValue::ConstructJsonValueNumber(UObject *WorldContextObject, float Number)
{
    TSharedPtr<FJsonValue> NewVal = MakeShareable(new FJsonValueNumber(Number));

    UOdinJsonValue *NewValue = NewObject<UOdinJsonValue>();
    NewValue->SetRootValue(NewVal);

    return NewValue;
}

UOdinJsonValue *UOdinJsonValue::ConstructJsonValueString(UObject       *WorldContextObject,
                                                         const FString &StringValue)
{
    TSharedPtr<FJsonValue> NewVal = MakeShareable(new FJsonValueString(StringValue));

    UOdinJsonValue *NewValue = NewObject<UOdinJsonValue>();
    NewValue->SetRootValue(NewVal);

    return NewValue;
}

UOdinJsonValue *UOdinJsonValue::ConstructJsonValueBool(UObject *WorldContextObject, bool InValue)
{
    TSharedPtr<FJsonValue> NewVal = MakeShareable(new FJsonValueBoolean(InValue));

    UOdinJsonValue *NewValue = NewObject<UOdinJsonValue>();
    NewValue->SetRootValue(NewVal);

    return NewValue;
}

UOdinJsonValue *UOdinJsonValue::ConstructJsonValueArray(UObject *WorldContextObject,
                                                        const TArray<UOdinJsonValue *> &InArray)
{
    TArray<TSharedPtr<FJsonValue>> ValueArray;
    for (auto InVal : InArray) {
        ValueArray.Add(InVal->GetRootValue());
    }

    TSharedPtr<FJsonValue> NewVal = MakeShareable(new FJsonValueArray(ValueArray));

    UOdinJsonValue *NewValue = NewObject<UOdinJsonValue>();
    NewValue->SetRootValue(NewVal);

    return NewValue;
}

UOdinJsonValue *UOdinJsonValue::ConstructJsonValueObject(UObject         *WorldContextObject,
                                                         UOdinJsonObject *JsonObject)
{
    TSharedPtr<FJsonValue> NewVal =
        MakeShareable(new FJsonValueObject(JsonObject->GetRootObject()));

    UOdinJsonValue *NewValue = NewObject<UOdinJsonValue>();
    NewValue->SetRootValue(NewVal);

    return NewValue;
}

TSharedPtr<FJsonValue> &UOdinJsonValue::GetRootValue()
{
    return JsonVal;
}

void UOdinJsonValue::SetRootValue(TSharedPtr<FJsonValue> &JsonValue)
{
    JsonVal = JsonValue;
}

EOJson::Type UOdinJsonValue::GetType() const
{
    if (!JsonVal.IsValid()) {
        return EOJson::None;
    }

    switch (JsonVal->Type) {
        case EJson::None:
            return EOJson::None;

        case EJson::Null:
            return EOJson::Null;

        case EJson::String:
            return EOJson::String;

        case EJson::Number:
            return EOJson::Number;

        case EJson::Boolean:
            return EOJson::Boolean;

        case EJson::Array:
            return EOJson::Array;

        case EJson::Object:
            return EOJson::Object;

        default:
            return EOJson::None;
    }
}

FString UOdinJsonValue::GetTypeString() const
{
    if (!JsonVal.IsValid()) {
        return "None";
    }

    switch (JsonVal->Type) {
        case EJson::None:
            return TEXT("None");

        case EJson::Null:
            return TEXT("Null");

        case EJson::String:
            return TEXT("String");

        case EJson::Number:
            return TEXT("Number");

        case EJson::Boolean:
            return TEXT("Boolean");

        case EJson::Array:
            return TEXT("Array");

        case EJson::Object:
            return TEXT("Object");

        default:
            return TEXT("None");
    }
}

bool UOdinJsonValue::IsNull() const
{
    if (!JsonVal.IsValid()) {
        return true;
    }

    return JsonVal->IsNull();
}

float UOdinJsonValue::AsNumber() const
{
    if (!JsonVal.IsValid()) {
        ErrorMessage(TEXT("Number"));
        return 0.f;
    }

    return JsonVal->AsNumber();
}

FString UOdinJsonValue::AsString() const
{
    if (!JsonVal.IsValid()) {
        ErrorMessage(TEXT("String"));
        return FString();
    }

    return JsonVal->AsString();
}

bool UOdinJsonValue::AsBool() const
{
    if (!JsonVal.IsValid()) {
        ErrorMessage(TEXT("Boolean"));
        return false;
    }

    return JsonVal->AsBool();
}

TArray<UOdinJsonValue *> UOdinJsonValue::AsArray() const
{
    TArray<UOdinJsonValue *> OutArray;

    if (!JsonVal.IsValid()) {
        ErrorMessage(TEXT("Array"));
        return OutArray;
    }

    TArray<TSharedPtr<FJsonValue>> ValArray = JsonVal->AsArray();
    for (auto Value : ValArray) {
        UOdinJsonValue *NewValue = NewObject<UOdinJsonValue>();
        NewValue->SetRootValue(Value);

        OutArray.Add(NewValue);
    }

    return OutArray;
}

UOdinJsonObject *UOdinJsonValue::AsObject()
{
    if (!JsonVal.IsValid()) {
        ErrorMessage(TEXT("Object"));
        return nullptr;
    }

    TSharedPtr<FJsonObject> NewObj = JsonVal->AsObject();

    auto *JsonObj = NewObject<UOdinJsonObject>();
    JsonObj->SetRootObject(NewObj);

    return JsonObj;
}

void UOdinJsonValue::ErrorMessage(const FString &InType) const {}
