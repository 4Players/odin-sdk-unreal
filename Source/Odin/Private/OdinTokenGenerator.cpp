/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinTokenGenerator.h"

#include "odin_sdk.h"

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

    this->TokenGenerator = odin_token_generator_create(StringCast<ANSICHAR>(*AccessKey).Get());
}

FString UOdinTokenGenerator::GenerateRoomToken(const FString &RoomId, const FString &UserId,
                                               EOdinTokenAudience TokenAudience)
{
    char             buf[512] = {0};
    OdinTokenOptions options  = OdinTokenOptions();
    options.customer          = "<no customer>";
    options.audience          = TokenAudience == EOdinTokenAudience::SingleServer
                                    ? OdinTokenAudience_Sfu
                                    : OdinTokenAudience_Gateway;
    options.lifetime          = 300;
    odin_token_generator_create_token_ex(this->TokenGenerator, StringCast<ANSICHAR>(*RoomId).Get(),
                                         StringCast<ANSICHAR>(*UserId).Get(), &options, buf,
                                         sizeof buf);

#if ENGINE_MAJOR_VERSION >= 5
    return FString(512, buf);
#else
    return ANSI_TO_TCHAR(buf);
#endif
}
