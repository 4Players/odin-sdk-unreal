/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"

#include "OdinTokenGenerator.generated.h"

UENUM(BlueprintType)
enum class EOdinTokenAudience : uint8 {
    Default,
    SingleServer,
};

UCLASS(ClassGroup = Utility, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinTokenGenerator : public UObject
{
    GENERATED_UCLASS_BODY()

  public:
    /**
     * Constructs a new instance of Odin token generator.
     *
     *@warning It is ok for development and testing purposes to create access keys room tokens on
     * client side - but in production you should switch to server side generation of room tokens!
     *
     * @param WorldContextObject The world context object to use for construction.
     * @param AccessKey The access key to set for the token generator.
     * @return A new instance of UOdinTokenGenerator with the provided AccessKey set.
     */
    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "Construct Token Generator",
                      ToolTip     = "Creates a new generator for signed authentication tokens",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Authentication")
    static UOdinTokenGenerator *ConstructTokenGenerator(UObject       *WorldContextObject,
                                                        const FString &AccessKey);

    /**
     * Sets the access key for the Odin Token Generator.
     *
     * @warning It is ok for development and testing purposes to create access keys room tokens on
     * client side - but in production you should switch to server side generation of room tokens!
     *
     * @param AccessKey The access key to be used for generating authentication tokens.
     */
    void SetAccessKey(const FString &AccesssKey);

    /**
     * Generates a token for joining a room.
     *
     * @warning It is ok for development and testing purposes to create access keys room tokens on
     * client side - but in production you should switch to server side generation of room tokens!
     *
     * @param RoomId The ID of the room to generate the token for.
     * @param UserId The ID of the user for whom the token is generated.
     * @param TokenAudience The audience type for the token (Default, SingleServer).
     * @param TokenLifeTime The lifetime in seconds of the generated room token.
     * @return The generated token as a FString.
     */
    UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "2"), Category = "Odin|Authentication")
    FString GenerateRoomToken(const FString &RoomId, const FString &UserId,
                              EOdinTokenAudience TokenAudience = EOdinTokenAudience::Default,
                              int32              TokenLifeTime = 300);

  private:
    struct OdinTokenGenerator *TokenGenerator = nullptr;
};
