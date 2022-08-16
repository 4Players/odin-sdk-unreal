/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#include "OdinTokenGenerator.h"

#include "OdinCore/include/odin.h"

UOdinTokenGenerator::UOdinTokenGenerator(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

UOdinTokenGenerator *UOdinTokenGenerator::ConstructTokenGenerator(UObject       *WorldContextObject,
                                                                  const FString &AccessKey)
{
    UOdinTokenGenerator *TokenGenerator = NewObject<UOdinTokenGenerator>();
    TokenGenerator->SetAccessKey(AccessKey);

    return TokenGenerator;
}

void UOdinTokenGenerator::SetAccessKey(const FString &AccessKey)
{
    if (this->TokenGenerator) {
        odin_token_generator_destroy(this->TokenGenerator);
        this->TokenGenerator = nullptr;
    }

    this->TokenGenerator = odin_token_generator_create(TCHAR_TO_ANSI(*AccessKey));
}

FString UOdinTokenGenerator::GenerateRoomToken(const FString &RoomId, const FString &UserId)
{
    char buf[512] = {0};
    odin_token_generator_create_token(this->TokenGenerator, TCHAR_TO_ANSI(*RoomId),
                                      TCHAR_TO_ANSI(*UserId), buf, sizeof buf);

    return ANSI_TO_TCHAR(buf);
}
