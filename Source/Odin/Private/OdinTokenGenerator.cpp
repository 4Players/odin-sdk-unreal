/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinTokenGenerator.h"

#include "OdinJsonObject.h"
#include "OdinVoice.h"
#include "OdinCore/include/odin.h"
#include "OdinNative/OdinUtils.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

UOdinTokenGenerator::UOdinTokenGenerator(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

UOdinTokenGenerator *UOdinTokenGenerator::ConstructTokenGenerator(UObject *WorldContextObject, const FString &AccessKey)
{
    UOdinTokenGenerator *TokenGenerator = NewObject<UOdinTokenGenerator>();
    TokenGenerator->SetAccessKey(AccessKey);

    return TokenGenerator;
}

void UOdinTokenGenerator::SetAccessKey(const FString &AccessKey)
{
    OdinTokenGenerator *GeneratorHandle = this->GetHandle();
    if (GeneratorHandle) {
        odin_token_generator_free(GeneratorHandle);
        this->SetHandle(nullptr);
    }

    OdinTokenGenerator *generator;
    OdinError           OdinResult = odin_token_generator_create(TCHAR_TO_ANSI(*AccessKey), &generator);
    if (ODIN_ERROR_SUCCESS == OdinResult) {
        this->SetHandle(generator);
    } else {
        ODIN_LOG(Error, "Token generation failed, most likely an invalid access key was provided.");
    }
}

void UOdinTokenGenerator::GenerateRoomToken(const FString &RoomId, const FString &UserId, UOdinJsonObject *&JsonAuth, FString &Token)
{
    Token = GenerateRoomTokenEx(RoomId, UserId);

    if (!Token.IsEmpty()) {
        JsonAuth =
            UOdinJsonObject::ConstructJsonObjectFromString(this, FString::Format(*FString(TEXT(R"({ "token": "{0}", "room_id": "{1}" })")), {*Token, *RoomId}));
    }
}

FString UOdinTokenGenerator::GenerateRoomTokenEx(const FString &RoomId, const FString &UserId, const FString &CustomerId, const FString &Audience,
                                                 const FString &Subject, const FString &Address, const FString &Upstream, const int32 Leeway) const
{
    FString result = "";

    if (nullptr != GetHandle()) {
        auto now = time(nullptr);

        TSharedPtr<FJsonObject> bodyObject = MakeShareable(new FJsonObject());
        bodyObject->SetStringField("rid", RoomId);
        bodyObject->SetStringField("uid", UserId);

        if (!CustomerId.IsEmpty())
            bodyObject->SetStringField("cid", CustomerId);
        if (!Audience.IsEmpty())
            bodyObject->SetStringField("aud", Audience);
        if (!Subject.IsEmpty())
            bodyObject->SetStringField("sub", Subject);
        if (!Address.IsEmpty())
            bodyObject->SetStringField("adr", Address);
        if (!Upstream.IsEmpty())
            bodyObject->SetStringField("ups", Upstream);

        bodyObject->SetNumberField("nbf", now);
        bodyObject->SetNumberField("exp", now + Leeway);

        FString                                             jsonBody;
        TSharedRef<OdinUtility::FCondensedJsonStringWriter> Writer = OdinUtility::FCondensedJsonStringWriterFactory::Create(&jsonBody);
        if (FJsonSerializer::Serialize(bodyObject.ToSharedRef(), Writer)) {
            uint32_t  tokenBuffer_size = 1024;
            ANSICHAR *tokenBuffer      = new ANSICHAR[tokenBuffer_size]{0};
            OdinError error            = odin_token_generator_sign(this->GetHandle(), TCHAR_TO_ANSI(*jsonBody), &tokenBuffer[0], &tokenBuffer_size);
            if (error != OdinError::ODIN_ERROR_SUCCESS) {
                FOdinModule::LogErrorCode("Token Generator: Signing the provided Json Body failed with message: %s", error);
            }
            result = FString(tokenBuffer);
            delete[] tokenBuffer;
        }
    } else {
        ODIN_LOG(Error, "Tried generating room token without valid generator handle.");
    }
    return result;
}
