/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinSubsystem.h"

#include "Odin.h"
#include "OdinRoom.h"

UOdinSubsystem* UOdinSubsystem::Get()
{
    if (GEngine) {
        return GEngine->GetEngineSubsystem<UOdinSubsystem>();
    }
    return nullptr;
}

void UOdinSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(Odin, Log, TEXT("Initialize Odin Room Registration Subsystem"));
}

void UOdinSubsystem::Deinitialize()
{
    Super::Deinitialize();
    UE_LOG(Odin, Log, TEXT("Deinitialize Odin Room Registration Subsystem"));
}

void UOdinSubsystem::RegisterRoom(OdinRoomHandle RoomHandle, UOdinRoom* Room)
{
    FScopeLock RegisterRoomLock(&RegisteredRoomsLock);
    UE_LOG(Odin, Log, TEXT("Registering Odin Room with room handle %llu"), RoomHandle);
    RegisteredRooms.Add(RoomHandle, TWeakObjectPtr<UOdinRoom>(Room));
}

void UOdinSubsystem::DeregisterRoom(OdinRoomHandle RoomHandle)
{
    FScopeLock DeregisterRoomLock(&RegisteredRoomsLock);
    if (RegisteredRooms.Contains(RoomHandle)) {
        UE_LOG(Odin, Log, TEXT("Deregistering Odin Room with room handle %llu"), RoomHandle);
        RegisteredRooms.Remove(RoomHandle);
    } else {
        UE_LOG(Odin, Warning,
               TEXT("Failed deregistering Odin room with room handle %llu - room handle was never "
                    "registered."),
               RoomHandle);
    }
}

TWeakObjectPtr<UOdinRoom> UOdinSubsystem::GetRoomByHandle(OdinRoomHandle RoomHandle)
{
    FScopeLock                       GetRoomLock(&RegisteredRoomsLock);
    const TWeakObjectPtr<UOdinRoom>* OdinRoom = RegisteredRooms.Find(RoomHandle);
    if (OdinRoom) {
        UE_LOG(Odin, Verbose, TEXT("Retrieved Odin Room with room handle %llu"), RoomHandle);
        return *OdinRoom;
    }
    UE_LOG(Odin, Verbose, TEXT("Did not find Odin Room with room handle %llu"), RoomHandle);
    return nullptr;
}

bool UOdinSubsystem::IsRoomRegistered(OdinRoomHandle Handle) const
{
    return RegisteredRooms.Contains(Handle);
}