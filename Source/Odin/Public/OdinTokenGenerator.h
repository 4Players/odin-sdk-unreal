/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once
#include "CoreMinimal.h"
#include "OdinCore/include/odin.h"
#include "OdinNative/OdinNativeHandle.h"

#include "OdinTokenGenerator.generated.h"

class UOdinJsonObject;

/**
 * A struct for generating ODIN tokens, employed for generating signed room tokens predicated on
 * an access key. Be aware that access keys serve as your unique authentication keys, requisite for
 * generating room tokens to access the ODIN server network. To ensure your security, it's strongly
 * recommended that you _NEVER_ embed an access key within your client code, and instead generate
 * room tokens on a server.
 */
UCLASS(ClassGroup = Utility, BlueprintType, Blueprintable)
class ODIN_API UOdinTokenGenerator : public UObject
{
    GENERATED_BODY()

  public:
    UOdinTokenGenerator(const class FObjectInitializer& PCIP);

    /**
     * Constructs a new instance of the UOdinTokenGenerator class. This method is used
     * to initialize a token generator object with a specified access key for generating
     * authentication tokens.
     *
     * @param WorldContextObject A reference to the current world context.
     * @param AccessKey A string containing the unique access key required to generate authentication tokens. To maintain security, avoid embedding the access
     * key in client code.
     * @return A pointer to the newly constructed UOdinTokenGenerator instance.
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Token Generator", ToolTip = "Creates a new generator for signed authentication tokens",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Authentication")
    static UOdinTokenGenerator* ConstructTokenGenerator(UObject* WorldContextObject, const FString& AccessKey);

    void SetAccessKey(const FString& AccesssKey);

    /**
     * Generates a signed room token for authenticating access to a specific room within the ODIN server network.
     * This method also creates a JSON object containing the generated token and associated room ID, which can be
     * used for further integration or debugging purposes.
     *
     * @param RoomId The identifier of the room for which the token is being generated. This is typically a unique
     *               string representing the specific room within the service.
     * @param UserId The identifier of the user requesting the token. This is a unique value representing the user
     *               within the context of the application.
     * @param JsonAuth A reference to a UOdinJsonObject pointer that will be populated with a JSON structure containing
     *                 details about the generated token and its associated room ID.
     * @param Token A reference to a string that will hold the generated authentication token if successful.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin|Authentication")
    void GenerateRoomToken(const FString& RoomId, const FString& UserId, UOdinJsonObject*& JsonAuth, FString& Token);

    /**
     * Generates a signed room token with detailed customization options for connecting to the ODIN server network.
     * The method signs a JSON body with user, room, and additional metadata to construct the token.
     *
     * @param RoomId The identifier of the room for which the token is being generated.
     * @param UserId A unique identifier of the user for whom the token is being created.
     * @param CustomerId An optional identifier for the customer creating the token.
     * @param Audience An optional audience field.
     * @param Subject An optional subject field.
     * @param Address An optional address field.
     * @param Upstream An optional upstream field.
     * @param Leeway An integer representing the lifetime duration in seconds for the token after its creation.
     * @return A signed token string that grants access to the specified room with the provided configuration. Returns an empty string if token generation
     * fails.
     */
    FString GenerateRoomTokenEx(const FString& RoomId, const FString& UserId, const FString& CustomerId = "", const FString& Audience = "",
                                const FString& Subject = "", const FString& Address = "", const FString& Upstream = "", const int32 Leeway = 300) const;

    inline OdinTokenGenerator* GetHandle() const
    {
        return IsValid(handle_) ? reinterpret_cast<OdinTokenGenerator*>(handle_->GetHandle()) : nullptr;
    }

    inline void SetHandle(OdinTokenGenerator* handle)
    {
        this->handle_ = NewObject<UOdinHandle>();
        this->handle_->SetHandle(handle);
    }

  private:
    UPROPERTY()
    UOdinHandle* handle_;
};