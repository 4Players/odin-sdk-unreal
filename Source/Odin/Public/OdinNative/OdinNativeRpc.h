/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once
#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "OdinJsonObject.h"
#include "OdinUtils.h"
#include "OdinVoice.h"
#include "OdinAudio/OdinChannelMask.h"

#include "OdinNativeRpc.generated.h"

// event responses

USTRUCT(BlueprintType)
struct FOdinRoomStatusChanged {
    GENERATED_BODY()

  public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    FString message;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    FString status;

    static const FString Name;

    static const FString JoinedStatus;
    static const FString JoiningStatus;
    static const FString ClosedStatus;
};

USTRUCT(BlueprintType)
struct FOdinNewReconnectToken {
    GENERATED_BODY()

  public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    FString token;

    static const FString Name;
};

USTRUCT(BlueprintType)
struct FOdinMessageReceived {
    GENERATED_BODY()

  public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    int64 sender_peer_id = 0;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    TArray<uint8> message;

    static const FString Name;
};

USTRUCT(BlueprintType)
struct FOdinJoined {
    GENERATED_BODY()

  public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    int64 own_peer_id = -1;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    FString room_id;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    FString customer;

    static const FString Name;
};

USTRUCT(BlueprintType)
struct FOdinPeerJoined {
    GENERATED_BODY()

  public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    int64 peer_id = 0;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    FString user_id;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    TArray<uint8> user_data;

    static const FString Name;
};

USTRUCT(BlueprintType)
struct FOdinPeerChanged {
    GENERATED_BODY()

  public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    int64 peer_id = 0;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    TArray<uint8> user_data;
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Events")
    TArray<FString> tags;
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Events")
    FString parameters;

    static const FString Name;
};

USTRUCT(BlueprintType)
struct FOdinPeerLeft {
    GENERATED_BODY()

  public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Odin|Events")
    int64 peer_id = 0;

    static const FString Name;
};

// rpc requests

USTRUCT(BlueprintType)
struct FOdinChangeSelf {
    GENERATED_BODY()

  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin|Events")
    TArray<uint8> user_data;
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Events")
    TArray<FString> tags;
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Events")
    FString parameters;

    static const FString Name;
    FString              ToJson() const
    {
        FString Result;
        FJsonObjectConverter::UStructToJsonObjectString(FOdinChangeSelf::StaticStruct(), this, Result, 0, EPropertyFlags::CPF_BlueprintReadOnly);
        return Result;
    }
    FString AsJson() const
    {
        const auto Payload = this->ToJson();
        if (Payload.IsEmpty())
            return "{}";

        return FString::Format(*FString(TEXT(R"({ "{0}": {1} })")), {*FOdinChangeSelf::Name, *Payload});
    }
    UOdinJsonObject* AsJObject() const
    {
        auto JsonObj = NewObject<UOdinJsonObject>();
        JsonObj->DecodeJson(*this->AsJson());
        return JsonObj;
    }
};

USTRUCT(BlueprintType)
struct FOdinSetChannelMasks {
    GENERATED_BODY()

  public:
    FOdinSetChannelMasks();

    FOdinSetChannelMasks(TMap<int64, uint64> InMasks, bool bReset);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Masks", ToolTip = "Set peerids with masks [{peerid:mask}, ...]"),
              Category = "Odin|Events")
    TMap<int64, FOdinChannelMask> masks = {};
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin|Events")
    bool reset = false;

    static const FString Name;
    FString              ToJson() const
    {
        // masks: Vec<(k,v)> => [[k,v],...]
        TArray<TSharedPtr<FJsonValue>> MaskArray;
        for (const auto& Element : masks) {
            TArray<TSharedPtr<FJsonValue>> PairArray;
            PairArray.Add(MakeShared<FJsonValueNumber>(Element.Key));
            PairArray.Add(MakeShared<FJsonValueNumberString>(LexToString(Element.Value.GetChannelMask())));
            MaskArray.Add(MakeShared<FJsonValueArray>(PairArray));

            FString AsBinary;
            for (int32 i = 63; i >= 0; --i) {
                bool bIsSet = (static_cast<uint64>(1) << i) & Element.Value.GetChannelMask();
                AsBinary.Append(bIsSet ? TEXT("1") : TEXT("0"));
            }
            ODIN_LOG(Verbose, TEXT("Mask in Binary: %s"), *AsBinary);
        }

        TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
        JsonObject->SetArrayField(TEXT("masks"), TArray<TSharedPtr<FJsonValue>>(MaskArray));
        JsonObject->SetBoolField(TEXT("reset"), reset);

        FString                                             Result;
        TSharedRef<OdinUtility::FCondensedJsonStringWriter> Writer = OdinUtility::FCondensedJsonStringWriterFactory::Create(&Result);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        return Result;
    }
    FString AsJson() const
    {
        const auto Payload = this->ToJson();
        if (Payload.IsEmpty())
            return "{}";

        return FString::Format(*FString(TEXT(R"({ "{0}": {1} })")), {*FOdinSetChannelMasks::Name, *Payload});
    }
    UOdinJsonObject* AsJObject()
    {
        auto JsonObj = NewObject<UOdinJsonObject>();
        JsonObj->DecodeJson(*this->AsJson());
        return JsonObj;
    }
};

USTRUCT(BlueprintType)
struct FOdinSendMessage {
    GENERATED_BODY()

  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin|Events")
    TArray<int64> peer_ids;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin|Events")
    TArray<uint8> message;

    static const FString Name;
    FString              ToJson() const
    {
        FString Result;
        FJsonObjectConverter::UStructToJsonObjectString(FOdinSendMessage::StaticStruct(), this, Result);
        return Result;
    }
    FString AsJson() const
    {
        const auto Payload = this->ToJson();
        if (Payload.IsEmpty())
            return "{}";

        return FString::Format(*FString(TEXT(R"({ "{0}": {1} })")), {*FOdinSendMessage::Name, *Payload});
    }
    UOdinJsonObject* AsJObject() const
    {
        auto JsonObj = NewObject<UOdinJsonObject>();
        JsonObj->DecodeJson(*this->AsJson());
        return JsonObj;
    }
};