/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinRegistrationSubsystem.h"
#include "Odin.h"
#include "OdinRoom.h"
#include "Engine/Engine.h"

UOdinRegistrationSubsystem* UOdinRegistrationSubsystem::Get()
{
    if (GEngine) {
        UOdinRegistrationSubsystem* OdinSubSystem =
            GEngine->GetEngineSubsystem<UOdinRegistrationSubsystem>();
        return OdinSubSystem;
    }
    UE_LOG(Odin, Error, TEXT("Error retrieving UOdinSubsystem: GEngine reference is invalid."));
    return nullptr;
}

bool UOdinRegistrationSubsystem::GlobalIsRoomValid(OdinRoomHandle Handle)
{
    UOdinRegistrationSubsystem* OdinSubsystem = Get();
    if (OdinSubsystem) {
        return OdinSubsystem->IsRoomRegistered(Handle);
    }
    return false;
}

void UOdinRegistrationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(Odin, Log, TEXT("Initialize Odin Room Registration Subsystem"));
}

void UOdinRegistrationSubsystem::Deinitialize()
{
    Super::Deinitialize();
    UE_LOG(Odin, Log, TEXT("Deinitialize Odin Room Registration Subsystem"));
}

void UOdinRegistrationSubsystem::RegisterRoom(OdinRoomHandle RoomHandle, UOdinRoom* Room)
{
    FScopeLock RegisterRoomLock(&RegisteredRoomsLock);
    UE_LOG(Odin, Log, TEXT("Registering Odin Room with room handle %llu"), RoomHandle);
    RegisteredRooms.Add(RoomHandle, TWeakObjectPtr<UOdinRoom>(Room));
}

void UOdinRegistrationSubsystem::DeregisterRoom(OdinRoomHandle RoomHandle)
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

TWeakObjectPtr<UOdinRoom> UOdinRegistrationSubsystem::GetRoomByHandle(OdinRoomHandle RoomHandle)
{
    FScopeLock                       GetRoomLock(&RegisteredRoomsLock);
    const TWeakObjectPtr<UOdinRoom>* OdinRoom = RegisteredRooms.Find(RoomHandle);
    if (OdinRoom) {
        UE_LOG(Odin, VeryVerbose, TEXT("Retrieved Odin Room with room handle %llu"), RoomHandle);
        return *OdinRoom;
    }
    UE_LOG(Odin, Verbose, TEXT("Did not find Odin Room with room handle %llu"), RoomHandle);
    return nullptr;
}

bool UOdinRegistrationSubsystem::IsRoomRegistered(OdinRoomHandle Handle) const
{
    return RegisteredRooms.Contains(Handle);
}