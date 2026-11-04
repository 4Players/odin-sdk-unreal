/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinMultiplayerPeer.h"
#include "OdinRoom.h"
#include "OdinSocket.h"
#include "OdinVoice.h"

UOdinMultiplayerPeer* UOdinMultiplayerPeer::ConstructMultiplayerPeer(UObject* WorldContextObject)
{ return NewObject<UOdinMultiplayerPeer>(WorldContextObject); }

bool UOdinMultiplayerPeer::Initialize(UOdinRoom* Room)
{
    if (!IsValid(Room)) {
        ODIN_LOG(Error, "Aborting Initialize of multiplayer peer due to invalid room.");
        return false;
    }
    if (UOdinRoom* Current = AttachedRoom.Get()) {
        if (Current == Room) {
            return true;
        }
        ODIN_LOG(Error, "Aborting Initialize of multiplayer peer: already attached to another room, call Close first.");
        return false;
    }

    AttachedRoom = Room;
    Room->OnRoomPeerJoinedBP.AddDynamic(this, &UOdinMultiplayerPeer::HandlePeerJoined);
    Room->OnRoomPeerLeftBP.AddDynamic(this, &UOdinMultiplayerPeer::HandlePeerLeft);
    Room->OnSocketBP.AddDynamic(this, &UOdinMultiplayerPeer::HandleSocketMessage);

    for (const int64 PeerId : Room->GetPeerIds()) {
        LinkPeer(PeerId);
    }
    return true;
}

void UOdinMultiplayerPeer::Close()
{
    TArray<int64> PeerIds;
    for (const TPair<int64, FOdinMultiplayerPeerLink>& Pair : Links) {
        ResetLink(Pair.Value);
        PeerIds.Add(Pair.Key);
    }
    Links.Empty();
    DisconnectedPeerIds.Empty();
    UnbindRoom();

    for (const int64 PeerId : PeerIds) {
        BroadcastPeerDisconnected(PeerId);
    }
}

void UOdinMultiplayerPeer::BeginDestroy()
{
    // the socket wrappers may already be unreachable as well, so only detach:
    // the room closes outbound sockets of peers that leave, and all of them on close
    UnbindRoom();
    Super::BeginDestroy();
}

void UOdinMultiplayerPeer::UnbindRoom()
{
    if (UOdinRoom* Room = AttachedRoom.Get()) {
        Room->OnRoomPeerJoinedBP.RemoveDynamic(this, &UOdinMultiplayerPeer::HandlePeerJoined);
        Room->OnRoomPeerLeftBP.RemoveDynamic(this, &UOdinMultiplayerPeer::HandlePeerLeft);
        Room->OnSocketBP.RemoveDynamic(this, &UOdinMultiplayerPeer::HandleSocketMessage);
    }
    AttachedRoom.Reset();
}

bool UOdinMultiplayerPeer::SendPacket(const TArray<uint8>& Data, int32 Channel, EOdinSocketKind Mode, int64 TargetPeerId)
{
    if (!AttachedRoom.IsValid()) {
        ODIN_LOG(Warning, "Aborting SendPacket: multiplayer peer is not attached to a room.");
        return false;
    }

    TArray<uint8> Message;
    EncodePacket(Channel, Data, Message);

    if (TargetPeerId > 0) {
        const FOdinMultiplayerPeerLink* Link = Links.Find(TargetPeerId);
        if (Link == nullptr) {
            ODIN_LOG(Warning, "Aborting SendPacket: no connection to peer %lld.", TargetPeerId);
            return false;
        }
        return SendToLink(*Link, Mode, Message);
    }

    bool bAllSent = true;
    for (const TPair<int64, FOdinMultiplayerPeerLink>& Pair : Links) {
        if (TargetPeerId < 0 && Pair.Key == -TargetPeerId) {
            continue;
        }
        if (!SendToLink(Pair.Value, Mode, Message)) {
            bAllSent = false;
        }
    }
    return bAllSent;
}

void UOdinMultiplayerPeer::DisconnectPeer(int64 PeerId)
{
    FOdinMultiplayerPeerLink Link;
    if (!Links.RemoveAndCopyValue(PeerId, Link)) {
        return;
    }
    ResetLink(Link);
    DisconnectedPeerIds.Add(PeerId);
    BroadcastPeerDisconnected(PeerId);
}

bool UOdinMultiplayerPeer::IsActive() const
{
    const UOdinRoom* Room = AttachedRoom.Get();
    return Room != nullptr && Room->IsConnected();
}

int64 UOdinMultiplayerPeer::GetUniqueId() const
{
    UOdinRoom* Room = AttachedRoom.Get();
    return Room != nullptr ? Room->GetOwnPeerId() : 0;
}

TArray<int64> UOdinMultiplayerPeer::GetConnectedPeerIds() const
{
    TArray<int64> PeerIds;
    PeerIds.Reserve(Links.Num());
    for (const TPair<int64, FOdinMultiplayerPeerLink>& Pair : Links) {
        PeerIds.Add(Pair.Key);
    }
    return PeerIds;
}

bool UOdinMultiplayerPeer::IsPeerConnected(int64 PeerId) const
{ return Links.Contains(PeerId); }

UOdinRoom* UOdinMultiplayerPeer::GetRoom() const
{ return AttachedRoom.Get(); }

void UOdinMultiplayerPeer::EncodePacket(int32 Channel, TConstArrayView<uint8> Payload, TArray<uint8>& OutMessage)
{
    const uint32 UnsignedChannel = static_cast<uint32>(Channel);
    OutMessage.Reset(HeaderSize + Payload.Num());
    OutMessage.Add(ProtocolVersion);
    OutMessage.Add(static_cast<uint8>(UnsignedChannel));
    OutMessage.Add(static_cast<uint8>(UnsignedChannel >> 8));
    OutMessage.Add(static_cast<uint8>(UnsignedChannel >> 16));
    OutMessage.Add(static_cast<uint8>(UnsignedChannel >> 24));
    OutMessage.Append(Payload.GetData(), Payload.Num());
}

bool UOdinMultiplayerPeer::DecodePacket(TConstArrayView<uint8> Message, int32& OutChannel, TArray<uint8>& OutPayload)
{
    if (Message.Num() < HeaderSize || Message[0] != ProtocolVersion) {
        return false;
    }
    OutChannel = static_cast<int32>(static_cast<uint32>(Message[1]) | static_cast<uint32>(Message[2]) << 8 | static_cast<uint32>(Message[3]) << 16
                                    | static_cast<uint32>(Message[4]) << 24);
    OutPayload = TArray<uint8>(Message.GetData() + HeaderSize, Message.Num() - HeaderSize);
    return true;
}

void UOdinMultiplayerPeer::HandlePeerJoined(UOdinRoom* InRoom, FOdinPeerJoined Data)
{ LinkPeer(Data.peer_id); }

void UOdinMultiplayerPeer::HandlePeerLeft(UOdinRoom* InRoom, FOdinPeerLeft Data)
{
    // the id may be reused by the next peer, so not inherit the ignore
    DisconnectedPeerIds.Remove(Data.peer_id);

    FOdinMultiplayerPeerLink Link;
    if (!Links.RemoveAndCopyValue(Data.peer_id, Link)) {
        return;
    }
    ResetLink(Link);
    BroadcastPeerDisconnected(Data.peer_id);
}

void UOdinMultiplayerPeer::HandleSocketMessage(UOdinSocket* Socket, TArray<uint8> Message)
{
    if (!IsValid(Socket) || Socket->GetSocketLabel() != MeshSocketMagic) {
        return;
    }

    const int64 SenderPeerId = Socket->GetRemotePeerId();
    if (DisconnectedPeerIds.Contains(SenderPeerId)) {
        return;
    }

    int32         Channel = 0;
    TArray<uint8> Payload;
    if (!DecodePacket(Message, Channel, Payload)) {
        ODIN_LOG(Verbose, "Dropping mesh message of %d bytes from peer %lld: too short or unknown version.", Message.Num(), SenderPeerId);
        return;
    }
    const EOdinSocketKind Mode = Socket->GetSocketKind();
    OnPacketReceived.Broadcast(SenderPeerId, Channel, Mode, Payload);
    OnPacketReceivedNative.Broadcast(SenderPeerId, Channel, Mode, Payload);
}

void UOdinMultiplayerPeer::LinkPeer(int64 PeerId)
{
    UOdinRoom* Room = AttachedRoom.Get();
    if (Room == nullptr || PeerId <= 0 || PeerId == Room->GetOwnPeerId() || Links.Contains(PeerId)) {
        return;
    }

    FOdinMultiplayerPeerLink Link;
    Link.Reliable   = Room->CreateLocalSocket(EOdinSocketKind::ODIN_SOCKET_KIND_RELIABLE, PeerId, MeshSocketMagic, 0).Get();
    Link.Unreliable = Room->CreateLocalSocket(EOdinSocketKind::ODIN_SOCKET_KIND_UNRELIABLE, PeerId, MeshSocketMagic, 0).Get();
    if (!IsValid(Link.Reliable) || Link.Reliable->GetNativeHandle() == nullptr || !IsValid(Link.Unreliable) || Link.Unreliable->GetNativeHandle() == nullptr) {
        ODIN_LOG(Error, "Failed to open mesh sockets to peer %lld.", PeerId);
        ResetLink(Link);
        return;
    }

    Links.Add(PeerId, Link);
    OnPeerConnected.Broadcast(PeerId);
    OnPeerConnectedNative.Broadcast(PeerId);
}

void UOdinMultiplayerPeer::BroadcastPeerDisconnected(int64 PeerId)
{
    OnPeerDisconnected.Broadcast(PeerId);
    OnPeerDisconnectedNative.Broadcast(PeerId);
}

void UOdinMultiplayerPeer::ResetLink(const FOdinMultiplayerPeerLink& Link)
{
    if (IsValid(Link.Reliable)) {
        Link.Reliable->ResetSocket();
    }
    if (IsValid(Link.Unreliable)) {
        Link.Unreliable->ResetSocket();
    }
}

bool UOdinMultiplayerPeer::SendToLink(const FOdinMultiplayerPeerLink& Link, EOdinSocketKind Mode, const TArray<uint8>& Message)
{
    UOdinSocket* Socket = Mode == EOdinSocketKind::ODIN_SOCKET_KIND_RELIABLE ? Link.Reliable : Link.Unreliable;
    if (!IsValid(Socket)) {
        return false;
    }

    OdinSocket* Handle = Socket->GetNativeHandle();
    if (Handle == nullptr) {
        ODIN_LOG(Verbose, "Skipping mesh send on a closed socket.");
        return false;
    }

    const OdinError Result = odin_socket_send(Handle, Message.GetData(), static_cast<uint32_t>(Message.Num()));
    if (Result != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Aborting mesh send due to invalid odin_socket_send call: %s", Result);
        return false;
    }
    return true;
}
