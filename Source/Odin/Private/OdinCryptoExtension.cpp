/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinCryptoExtension.h"
#include "OdinVoice.h"
#include "odin_crypto.h"

UOdinCrypto::UOdinCrypto(const class FObjectInitializer& PCIP)
    : Super(PCIP)
{
}

void UOdinCrypto::BeginDestroy()
{
    ODIN_LOG(Verbose, "ODIN Destroy: %s", ANSI_TO_TCHAR(__FUNCTION__));

    Super::BeginDestroy();
}

UOdinCrypto* UOdinCrypto::ConstructCrypto(UObject* WorldContextObject, OdinCipher* handle)
{
    UOdinCrypto* result = NewObject<UOdinCrypto>(WorldContextObject);
    result->SetHandle(handle);

    return result;
}

UOdinCrypto* UOdinCrypto::ConstructCrypto(UObject* WorldContextObject, const TArray<uint8>& Secret)
{
    UOdinCrypto* Result = NewObject<UOdinCrypto>(WorldContextObject);

#if ODIN_USE_CRYPTO
    OdinCipher* handle = odin_crypto_create(ODIN_CRYPTO_VERSION);
    if (nullptr == handle) {
        ODIN_LOG(Error, "Aborting ConstructCrypto due to invalid handle from odin_crypto_create.");
        return Result;
    }
    Result->SetHandle(handle);
#else
    ODIN_LOG(Warning, "Internal crypto not supported! OdinModule loaded without extension "
                      "features. No cipher applied: ODIN_CRYPTO_PEER_STATUS_UNENCRYPTED");
    Result->SetHandle(nullptr);
#endif

    Result->SetSecret(Secret);

    return Result;
}

TArray<uint8> UOdinCrypto::GetSecret() const
{
    return this->Secret;
}

void UOdinCrypto::SetSecret(const TArray<uint8>& NewSecret)
{
    this->Secret = NewSecret;

#if ODIN_USE_CRYPTO
    int32 ret = odin_crypto_set_password(this->GetHandle(), this->Secret.GetData(), this->Secret.Num());
    if (ret != 0) {
        ODIN_LOG(Error, "Aborting SetSecret due to invalid odin_crypto_set_password call: %d", ret);
        return;
    }
#endif
}

EOdinCryptoPeerStatus UOdinCrypto::GetPeerStatus(int64 PeerId)
{
#if ODIN_USE_CRYPTO
    OdinCryptoPeerStatus ret = odin_crypto_get_peer_status(this->GetHandle(), PeerId);
    return (EOdinCryptoPeerStatus)ret;
#else
    return EOdinCryptoPeerStatus::ODIN_CRYPTO_PEER_STATUS_UNKNOWN;
#endif
}
