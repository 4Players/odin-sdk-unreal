/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinSubsystem.h"
#include "OdinAudio/OdinDecoder.h"
#include "OdinAudio/OdinEncoder.h"
#include "OdinRoom.h"
#include "OdinVoice.h"
#include "Engine/Engine.h"

UOdinSubsystem* UOdinSubsystem::Get()
{ return GEngine ? GEngine->GetEngineSubsystem<UOdinSubsystem>() : nullptr; }

bool UOdinSubsystem::GlobalIsRoomValid(const OdinRoom* Handle)
{
    if (const UOdinSubsystem* OdinSubsystem = Get()) {
        return OdinSubsystem->IsRoomRegistered(Handle);
    }
    return false;
}

void UOdinSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ODIN_LOG(Log, "Initialize Odin Registration Subsystem");
    PushDataThread           = MakeUnique<FOdinAudioPushDataThread>();
    DatagramProcessingThread = MakeUnique<FOdinDatagramProcessingThread>();
}

void UOdinSubsystem::Deinitialize()
{
    Super::Deinitialize();
    ODIN_LOG(Log, "Deinitialize Odin Registration Subsystem");
    if (PushDataThread.IsValid()) {
        PushDataThread->Exit();
        PushDataThread.Reset();
    }
    if (DatagramProcessingThread.IsValid()) {
        DatagramProcessingThread->Exit();
        DatagramProcessingThread.Reset();
    }
}

void UOdinSubsystem::LinkEncoder(const TWeakObjectPtr<UOdinEncoder> Encoder, const TWeakObjectPtr<UOdinRoom> TargetRoom)
{
    if (!PushDataThread.IsValid()) {
        ODIN_LOG(Error, "Push Data Thread is not valid.");
        return;
    }

    if (!Encoder.IsValid()) {
        ODIN_LOG(Error, "Tried linking with invalid Odin Encoder UObject pointer.");
        return;
    }

    if (!TargetRoom.IsValid()) {
        ODIN_LOG(Error, "Tried linking with invalid Odin Room UObject pointer.");
        return;
    }
    PushDataThread->LinkEncoder(Encoder->GetHandle(), TargetRoom->GetHandle());
}

void UOdinSubsystem::UnlinkEncoder(TWeakObjectPtr<UOdinEncoder> Encoder)
{
    if (!Encoder.IsValid()) {
        ODIN_LOG(Error, "Tried unlinking an invalid Odin Encoder UObject pointer.");
        return;
    }
    UnlinkEncoder(Encoder->GetHandle());
}

void UOdinSubsystem::UnlinkEncoder(OdinEncoder* Encoder)
{
    if (PushDataThread.IsValid()) {
        PushDataThread->UnlinkEncoder(Encoder);
    } else {
        ODIN_LOG(Error, "Push Data Thread is not valid.");
    }
}

void UOdinSubsystem::PushAudioToEncoder(OdinEncoder* Encoder, TArray<float>&& Audio)
{
    if (PushDataThread.IsValid()) {
        PushDataThread->PushAudioToEncoder(Encoder, MoveTemp(Audio));
    }
}

void UOdinSubsystem::RegisterEncoder(OdinEncoder* Handle, UOdinEncoder* Encoder)
{
    if (Handle == nullptr || !IsValid(Encoder)) {
        return;
    }
    FScopeLock RegisterLock(&EncoderObjectsCS);
    EncoderObjects.Add(Handle, FRegisteredEncoder{Encoder, ++EncoderRegistrationCounter});
}

void UOdinSubsystem::DeregisterEncoder(OdinEncoder* Handle)
{
    if (Handle == nullptr) {
        return;
    }
    FScopeLock DeregisterLock(&EncoderObjectsCS);
    EncoderObjects.Remove(Handle);
}

TWeakObjectPtr<UOdinEncoder> UOdinSubsystem::GetEncoderByHandle(OdinEncoder* Handle) const
{
    FScopeLock GetEncoderLock(&EncoderObjectsCS);
    if (const FRegisteredEncoder* EncoderObject = EncoderObjects.Find(Handle)) {
        return EncoderObject->Encoder;
    }
    return nullptr;
}

uint64 UOdinSubsystem::GetEncoderRegistrationId(OdinEncoder* Handle) const
{
    FScopeLock GetEncoderLock(&EncoderObjectsCS);
    if (const FRegisteredEncoder* EncoderObject = EncoderObjects.Find(Handle)) {
        return EncoderObject->RegistrationId;
    }
    return 0;
}

TWeakObjectPtr<UOdinEncoder> UOdinSubsystem::GetEncoderByRegistration(OdinEncoder* Handle, uint64 RegistrationId) const
{
    FScopeLock GetEncoderLock(&EncoderObjectsCS);
    if (const FRegisteredEncoder* EncoderObject = EncoderObjects.Find(Handle)) {
        if (EncoderObject->RegistrationId == RegistrationId) {
            return EncoderObject->Encoder;
        }
    }
    return nullptr;
}

TArray<TWeakObjectPtr<UOdinRoom>> UOdinSubsystem::GetRoomsForEncoder(OdinEncoder* Encoder) const
{
    TArray<TWeakObjectPtr<UOdinRoom>> Rooms;
    if (PushDataThread.IsValid()) {
        for (OdinRoom* RoomHandle : PushDataThread->GetRoomsFor(Encoder)) {
            TWeakObjectPtr<UOdinRoom> Room = GetRoomByHandle(RoomHandle);
            if (Room.IsValid()) {
                Rooms.Add(Room);
            }
        }
    }
    return Rooms;
}

void UOdinSubsystem::RegisterRoom(OdinRoom* Handle, UOdinRoom* Room)
{
    FScopeLock RegisterRoomLock(&RoomsCS);
    ODIN_LOG(Log, "Registering Odin Room with handle %p", Handle);
    RegisteredRooms.Add(Handle, TWeakObjectPtr<UOdinRoom>(Room));
}

void UOdinSubsystem::DeregisterRoom(OdinRoom* Handle)
{
    FScopeLock DeregisterRoomLock(&RoomsCS);
    if (RegisteredRooms.Contains(Handle)) {
        ODIN_LOG(Log, "Deregistering Odin Room with handle %p", Handle);
        RegisteredRooms.Remove(Handle);
    } else {
        ODIN_LOG(Log, "Could not deregister Odin room with handle %p: Room handle was not registered or already deregistered.", Handle);
    }
}

void UOdinSubsystem::SwapRoomHandle(OdinRoom* OldHandle, OdinRoom* NewHandle)
{
    FScopeLock DeregisterRoomLock(&RoomsCS);
    if (RegisteredRooms.Contains(OldHandle)) {
        ODIN_LOG(Verbose, "Swap Odin Room handle %p with handle %p", OldHandle, NewHandle);
        TWeakObjectPtr<UOdinRoom> Room;
        RegisteredRooms.RemoveAndCopyValue(OldHandle, Room);
        RegisterRoom(NewHandle, Room.Get());
    } else {
        ODIN_LOG(Warning, "Failed swap Odin room with handle %p - room handle was never registered.", OldHandle);
    }
}

TWeakObjectPtr<UOdinRoom> UOdinSubsystem::GetRoomByHandle(OdinRoom* Handle) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinSubsystem::GetRoomByHandle);
    FScopeLock                       GetRoomLock(&RoomsCS);
    const TWeakObjectPtr<UOdinRoom>* OdinRoom = RegisteredRooms.Find(Handle);
    if (OdinRoom) {
        ODIN_LOG(Verbose, "Retrieved Odin Room with handle %p", Handle);
        return *OdinRoom;
    }
    ODIN_LOG(Verbose, "Did not find Odin Room with handle %p", Handle);
    return nullptr;
}

TArray<TWeakObjectPtr<UOdinRoom>> UOdinSubsystem::GetRoomsByName(const FString& RoomId) const
{
    TArray<TWeakObjectPtr<UOdinRoom>> RegisteredRoomObjects;
    {
        FScopeLock GetRoomLock(&RoomsCS);
        RegisteredRooms.GenerateValueArray(RegisteredRoomObjects);
    }

    TArray<TWeakObjectPtr<UOdinRoom>> FoundRooms;
    for (TWeakObjectPtr CurrentRoomPtr : RegisteredRoomObjects) {

        if (CurrentRoomPtr.IsValid()) {
            const FString& CurrentRoomName = CurrentRoomPtr->GetOdinRoomName();
            if (CurrentRoomName == RoomId) {
                FoundRooms.Add(CurrentRoomPtr);
            }
        }
    }
    return FoundRooms;
}

bool UOdinSubsystem::IsRoomRegistered(const OdinRoom* Handle) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinSubsystem::IsRoomRegistered);
    FScopeLock GetRoomLock(&RoomsCS);
    return RegisteredRooms.Contains(Handle);
}

void UOdinSubsystem::LinkDecoderToPeer(const UOdinDecoder* Decoder, OdinRoom* TargetRoom, const uint32 PeerId)
{
    if (DatagramProcessingThread.IsValid() && IsValid(Decoder)) {
        DatagramProcessingThread->LinkDecoderToPeer(Decoder->GetNativeHandle(), TargetRoom, PeerId, Decoder->ChannelMask);
    }
}

void UOdinSubsystem::RegisterDecoderObject(const TWeakObjectPtr<UOdinDecoder> Decoder)
{
    if (Decoder.IsValid()) {
        FScopeLock DecoderObjectLock(&DecoderObjectsCS);
        DecoderObjects.Add(Decoder->GetNativeHandle(), Decoder.Get());
    }
}

TWeakObjectPtr<UOdinDecoder> UOdinSubsystem::GetDecoderByHandle(OdinDecoder* Handle) const
{
    FScopeLock GetDecoderLock(&DecoderObjectsCS);
    if (const TWeakObjectPtr<UOdinDecoder>* DecoderObject = DecoderObjects.Find(Handle)) {
        ODIN_LOG(Verbose, "Retrieved Odin Decoder with handle %p", Handle);
        return *DecoderObject;
    }
    ODIN_LOG(Verbose, "Did not find Odin Decoder with handle %p", Handle);
    return nullptr;
}

void UOdinSubsystem::DeregisterDecoder(const UOdinDecoder* OdinDecoder)
{
    if (!IsValid(OdinDecoder)) {
        ODIN_LOG(Warning, "Tried deregistering invalid Decoder UObject.");
        return;
    }

    DeregisterDecoder(OdinDecoder->GetNativeHandle());
}

void UOdinSubsystem::DeregisterDecoder(const OdinDecoder* Handle)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinSubsystem::DeregisterDecoder);

    if (!Handle) {
        ODIN_LOG(Log, "Tried Deregistering invalid Decoder Handle, aborting.");
        return;
    }
    if (DatagramProcessingThread.IsValid()) {
        DatagramProcessingThread->UnlinkDecoder(Handle);
    }
    {
        FScopeLock DeregisterDecoderObjects(&DecoderObjectsCS);
        DecoderObjects.Remove(Handle);
        ODIN_LOG(Verbose, "Number of registered decoder objects: %d", DecoderObjects.Num());
    }
}

TArray<OdinDecoder*> UOdinSubsystem::GetDecoderHandlesFor(OdinRoom* TargetRoom, uint32 PeerId) const
{ return GetDecoderHandlesFor(TargetRoom, PeerId, ~static_cast<uint64>(0)); }

TArray<OdinDecoder*> UOdinSubsystem::GetDecoderHandlesFor(OdinRoom* TargetRoom, uint32 PeerId, uint64 ChannelMask) const
{
    if (DatagramProcessingThread.IsValid()) {
        return DatagramProcessingThread->GetDecoderHandlesFor(TargetRoom, PeerId, ChannelMask);
    }
    return TArray<OdinDecoder*>();
}

TArray<UOdinDecoder*> UOdinSubsystem::GetDecodersFor(OdinRoom* TargetRoom, uint32 PeerId) const
{ return GetDecodersFor(TargetRoom, PeerId, ~static_cast<uint64>(0)); }

TArray<UOdinDecoder*> UOdinSubsystem::GetDecodersFor(OdinRoom* TargetRoom, uint32 PeerId, uint64 ChannelMask) const
{
    const TArray<OdinDecoder*> OdinDecoderHandles = GetDecoderHandlesFor(TargetRoom, PeerId, ChannelMask);
    TArray<UOdinDecoder*>      OdinDecoders;
    {
        FScopeLock DecoderObjectsLock(&DecoderObjectsCS);
        for (OdinDecoder* DecoderHandle : OdinDecoderHandles) {
            const TWeakObjectPtr<UOdinDecoder>* DecoderObject = DecoderObjects.Find(DecoderHandle);
            if (DecoderObject && DecoderObject->IsValid()) {
                OdinDecoders.Add(DecoderObject->Get());
            }
        }
    }
    return OdinDecoders;
}

TArray<OdinDecoder*> UOdinSubsystem::GetAllDecoderHandlesByPeer(uint32 PeerId) const
{
    if (DatagramProcessingThread.IsValid()) {
        return DatagramProcessingThread->GetDecodersByPeer(PeerId);
    }
    return TArray<OdinDecoder*>();
}

TArray<UOdinDecoder*> UOdinSubsystem::GetAllDecodersByPeer(uint32 PeerId) const
{
    const TArray<OdinDecoder*> OdinDecoderHandles = GetAllDecoderHandlesByPeer(PeerId);
    TArray<UOdinDecoder*>      OdinDecoders;
    {
        FScopeLock DecoderObjectsLock(&DecoderObjectsCS);
        for (OdinDecoder* DecoderHandle : OdinDecoderHandles) {
            const TWeakObjectPtr<UOdinDecoder>* DecoderObject = DecoderObjects.Find(DecoderHandle);
            if (DecoderObject && DecoderObject->IsValid()) {
                OdinDecoders.Add(DecoderObject->Get());
            }
        }
    }
    return OdinDecoders;
}

void UOdinSubsystem::HandleDatagram(OdinRoom* RoomHandle, uint32 PeerId, uint64 ChannelMask, uint32 SsrcId, TArray<uint8>&& Datagram)
{
    if (DatagramProcessingThread.IsValid()) {
        DatagramProcessingThread->HandleDatagram(RoomHandle, PeerId, ChannelMask, SsrcId, MoveTemp(Datagram));
    }
}