/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "odin_sdk.h"
#include "Subsystems/EngineSubsystem.h"
#include "OdinSubsystem.generated.h"

class UOdinRoom;

/**
 * Manages Odin rooms within the subsystem, handling registration, deregistration, and retrieval of
 * rooms by their handles. Allows us to retrieve the Unreal UOdinRoom UObjects connected to the Odin
 * Native OdinRoomHandles that are supplied by event callbacks from Odin. Also allows us to do
 * thread safe checks on whether the UObjects connected to a room handle are still valid or in the
 * process of being destroyed.
 */
UCLASS()
class ODIN_API UOdinSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()
  public:
    static UOdinSubsystem* Get();
    static bool            GlobalIsRoomValid(OdinRoomHandle);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * @param RoomHandle The Odin Native handle of the room being registered.
     * @param Room Pointer to the UOdinRoom object that is being registered.
     */
    void RegisterRoom(OdinRoomHandle RoomHandle, UOdinRoom* Room);
    /**
     * @param RoomHandle The unique handle of the room to deregister
     */
    void DeregisterRoom(OdinRoomHandle RoomHandle);
    /**
     * @brief Retrieve Odin room by its handle.
     *
     * @param RoomHandle The Odin Native handle of the room to retrieve.
     * @return The Odin room associated with the given handle, or nullptr if not found.
     */
    TWeakObjectPtr<UOdinRoom> GetRoomByHandle(OdinRoomHandle RoomHandle);
    /**
     * Check if a room with the given handle is registered in the OdinSubsystem.
     *
     * @param Handle The handle of the room to check for registration.
     * @return True if the room is registered, false otherwise.
     */
    bool IsRoomRegistered(OdinRoomHandle) const;

  protected:
    TMap<uint64, TWeakObjectPtr<UOdinRoom>> RegisteredRooms;
    FCriticalSection                        RegisteredRoomsLock;
};
