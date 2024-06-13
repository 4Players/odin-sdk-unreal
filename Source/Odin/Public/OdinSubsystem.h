/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "odin_sdk.h"
#include "Subsystems/EngineSubsystem.h"
#include "OdinSubsystem.generated.h"

class UOdinRoom;

UCLASS()
class ODIN_API UOdinSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()
  public:
    static UOdinSubsystem* Get();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void                      RegisterRoom(OdinRoomHandle RoomHandle, UOdinRoom* Room);
    void                      DeregisterRoom(OdinRoomHandle RoomHandle);
    TWeakObjectPtr<UOdinRoom> GetRoomByHandle(OdinRoomHandle RoomHandle);
    bool                      IsRoomRegistered(OdinRoomHandle) const;

  protected:
    TMap<OdinRoomHandle, TWeakObjectPtr<UOdinRoom>> RegisteredRooms;
    FCriticalSection                                RegisteredRoomsLock;
};
