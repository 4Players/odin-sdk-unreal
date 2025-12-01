/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinCore/include/odin.h"
#include "OdinNative/OdinNativeHandle.h"
#include "OdinCryptoExtension.generated.h"

#ifndef ODIN_USE_EXTENSIONS
#define ODIN_USE_EXTENSIONS 0
#endif

#if ODIN_USE_EXTENSIONS
#define ODIN_USE_CRYPTO 1
#else
#define ODIN_USE_CRYPTO 0
#endif

UENUM(BlueprintType)
enum class EOdinCryptoPeerStatus : uint8 {
    ODIN_CRYPTO_PEER_STATUS_PASSWORD_MISSMATCH = static_cast<unsigned char>(-1) UMETA(DisplayName = "INVALID_PASSWORD"),
    ODIN_CRYPTO_PEER_STATUS_UNKNOWN            = 0 UMETA(DisplayName = "UNKNOWN"),
    ODIN_CRYPTO_PEER_STATUS_UNENCRYPTED        = 1 UMETA(DisplayName = "UNENCRYPTED"),
    ODIN_CRYPTO_PEER_STATUS_ENCRYPTED          = 2 UMETA(DisplayName = "ENCRYPTED"),
};

/**
 * Optional, pluggable encryption module for room communications. A cipher can be attached to an
 * ODIN room handle on creation to enable customizable, end-to-end encryption (E2EE). When enabled,
 * it intercepts data right before transmission and immediately after reception, allowing custom
 * processing of datagrams, messages and custom peer user data. The structure provides a suite of
 * callback functions for initialization, cleanup, event handling and encryption/decryption tasks,
 * along with parameters to adjust for any additional capacity overhead.
 */
UCLASS(Blueprintable, ClassGroup = Odin)
class ODIN_API UOdinCrypto : public UObject
{
    GENERATED_BODY()
    friend class UOdinRoom;

  public:
    UOdinCrypto(const class FObjectInitializer& PCIP);

    static UOdinCrypto* ConstructCrypto(UObject* WorldContextObject, OdinCipher* handle);
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Construct Crypto Extension", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Room")
    static UOdinCrypto* ConstructCrypto(UObject* WorldContextObject, const TArray<uint8>& Secret);

    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Peer Crypto Status", ToolTip = "Get info about the encryption state of peer by id"),
              Category = "Odin|Room|Crypto")
    EOdinCryptoPeerStatus GetPeerStatus(int64 PeerId);

    UFUNCTION(BlueprintInternalUseOnly, BlueprintPure, Category = "Odin|Room")
    TArray<uint8> GetSecret() const;

    UFUNCTION(BlueprintInternalUseOnly, BlueprintCallable, Category = "Odin|Room")
    void SetSecret(const TArray<uint8>& NewSecret);

    UPROPERTY(BlueprintGetter = GetSecret, BlueprintSetter = SetSecret, Category = "Odin|Room")
    TArray<uint8> Secret;

    inline OdinCipher* GetHandle() const
    {
        return IsValid(Handle) && Handle->IsValidLowLevel() ? reinterpret_cast<OdinCipher*>(Handle->GetHandle()) : nullptr;
    }

    inline void SetHandle(OdinCipher* NewHandle)
    {
        if (NewHandle == nullptr && IsValid(this->Handle)) {
            this->Handle->SetHandle(nullptr);
            return;
        }

        this->Handle = NewObject<UOdinHandle>();
        this->Handle->SetHandle(NewHandle);
    }

  protected:
    virtual void BeginDestroy() override;

  private:
    UPROPERTY()
    UOdinHandle* Handle;
};