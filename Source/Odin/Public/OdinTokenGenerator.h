/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"

#include "OdinTokenGenerator.generated.h"

UCLASS(ClassGroup = Utility, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinTokenGenerator : public UObject
{
    GENERATED_UCLASS_BODY()

  public:
    UFUNCTION(BlueprintPure,
              meta = (DisplayName = "Construct a Token Generator", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Token")
    static UOdinTokenGenerator *ConstructTokenGenerator(UObject       *WorldContextObject,
                                                        const FString &AccessKey);

    void SetAccessKey(const FString &AccesssKey);

    UFUNCTION(BlueprintCallable, Category = "Odin|Token")
    FString GenerateRoomToken(const FString &RoomId, const FString &UserId);

  private:
    struct OdinTokenGenerator *TokenGenerator = nullptr;
};