/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"

#include "OdinTokenGenerator.generated.h"

UENUM(BlueprintType)
enum EOdinTokenAudience {
    Default,
    SingleServer,
};

UCLASS(ClassGroup = Utility, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinTokenGenerator : public UObject
{
    GENERATED_UCLASS_BODY()

  public:
    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "Construct Token Generator",
                      ToolTip     = "Creates a new generator for signed authentication tokens",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Authentication")
    static UOdinTokenGenerator *ConstructTokenGenerator(UObject       *WorldContextObject,
                                                        const FString &AccessKey);

    void SetAccessKey(const FString &AccesssKey);

    UFUNCTION(BlueprintCallable, Category = "Odin|Authentication")
    FString GenerateRoomToken(const FString &RoomId, const FString &UserId,
                              EOdinTokenAudience TokenAudience);

  private:
    struct OdinTokenGenerator *TokenGenerator = nullptr;
};
