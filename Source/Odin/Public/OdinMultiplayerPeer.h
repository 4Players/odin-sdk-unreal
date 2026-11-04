/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinNative/OdinNativeRpc.h"

#include "OdinMultiplayerPeer.generated.h"

class UOdinRoom;
class UOdinSocket;

/**
 * The outbound socket pair a UOdinMultiplayerPeer keeps open to one remote peer.
 */
USTRUCT()
struct ODIN_API FOdinMultiplayerPeerLink {
    GENERATED_BODY()

    UPROPERTY()
    UOdinSocket* Reliable = nullptr;
    UPROPERTY()
    UOdinSocket* Unreliable = nullptr;
};

/**
 * Packet transport between all peers of an ODIN room, built on room sockets. Exchange its own packets over
 * an existing voice connection instead of a separate transport.
 *
 * This is a mesh peer: a reliable and an unreliable socket is opened to every other peer in the room.
 * Peers are linked as the room announces them (including peers already present when Initialize is called) and unlinked when they leave; since the room
 * reports every peer as left on connection loss and announces them again after a reconnect, links survive reconnects without extra handling.
 * Receivers only consider sockets labelled MeshSocketMagic and drop messages shorter than HeaderSize or with an unknown version; sender and
 * transfer mode are taken from the socket the message arrived on
 *
 * @remarks Game thread only. Unreliable packets must fit into a single datagram (about 1 KB after headers); larger ones fail to send. The room is not
 * owned: this object never closes it, and Close only detaches from it.
 */
UCLASS(ClassGroup = Odin, BlueprintType)
class ODIN_API UOdinMultiplayerPeer : public UObject
{
    GENERATED_BODY()

  public:
    /** Magic number identifying mesh sockets, sent as their socket label: the ASCII bytes 'O','D','M','P' read as a little-endian int32. */
    static constexpr int32 MeshSocketMagic = 0x504D444F;
    /** Wire format version, the first byte of every message. */
    static constexpr uint8 ProtocolVersion = 1;
    /** Version byte plus the int32 channel. */
    static constexpr int32 HeaderSize = 5;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOdinMultiplayerPacketReceivedDelegate, int64, PeerId, int32, Channel, EOdinSocketKind, Mode,
                                                  const TArray<uint8>&, Data);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinMultiplayerPeerDelegate, int64, PeerId);

    /**
     * A packet from a remote mesh peer arrived. Mode is the kind of socket it was sent on.
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Multiplayer|Events")
    FOdinMultiplayerPacketReceivedDelegate OnPacketReceived;
    /**
     * Sockets to a remote peer were opened; packets can be sent to it from now on.
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Multiplayer|Events")
    FOdinMultiplayerPeerDelegate OnPeerConnected;
    /**
     * A remote peer was unlinked, because it left the room, the room lost its connection, DisconnectPeer or Close was called. Every connected peer is
     * reported as disconnected exactly once.
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Multiplayer|Events")
    FOdinMultiplayerPeerDelegate OnPeerDisconnected;

    DECLARE_MULTICAST_DELEGATE_FourParams(FOdinMultiplayerPacketReceivedNativeDelegate, int64, int32, EOdinSocketKind, const TArray<uint8>&);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOdinMultiplayerPeerNativeDelegate, int64);

    /** Native counterpart of OnPacketReceived, fired right after it. */
    FOdinMultiplayerPacketReceivedNativeDelegate OnPacketReceivedNative;
    /** Native counterpart of OnPeerConnected, fired right after it. */
    FOdinMultiplayerPeerNativeDelegate OnPeerConnectedNative;
    /** Native counterpart of OnPeerDisconnected, fired right after it. */
    FOdinMultiplayerPeerNativeDelegate OnPeerDisconnectedNative;

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Multiplayer Peer", ToolTip = "Creates a new multiplayer mesh peer", HidePin = "WorldContextObject",
                          DefaultToSelf = "WorldContextObject", Keywords = "Create,Mesh,Network"),
              Category = "Odin")
    static UOdinMultiplayerPeer* ConstructMultiplayerPeer(UObject* WorldContextObject);

    /**
     * Attaches to a room and links every remote peer the room knows, now and whenever one joins later. Can be called before or after joining.
     * @param Room  room to exchange packets in
     * @return false if Room is invalid or this peer is already attached to another room
     */
    UFUNCTION(BlueprintCallable, meta = (ToolTip = "Attaches to a room and opens sockets to all its peers"), Category = "Odin|Multiplayer")
    bool Initialize(UOdinRoom* Room);

    /**
     * Closes all mesh sockets, reports every connected peer as disconnected and detaches from the room. The room itself stays joined.
     */
    UFUNCTION(BlueprintCallable, meta = (ToolTip = "Closes all mesh sockets and detaches from the room"), Category = "Odin|Multiplayer")
    void Close();

    /**
     * Sends a packet to remote mesh peers.
     * @param Data          payload
     * @param Channel       application-defined channel, delivered to the receiver as is
     * @param Mode          reliable (ordered) or unreliable
     * @param TargetPeerId  0 sends to all connected peers, a positive id to that peer only and a negative id to all connected peers except -TargetPeerId
     * @return true if the packet was queued for every addressed peer
     */
    UFUNCTION(BlueprintCallable, meta = (ToolTip = "Sends a packet to one (id > 0), all (0) or all but one (-id) connected peers"),
              Category = "Odin|Multiplayer")
    bool SendPacket(const TArray<uint8>& Data, int32 Channel = 0, EOdinSocketKind Mode = EOdinSocketKind::ODIN_SOCKET_KIND_RELIABLE, int64 TargetPeerId = 0);

    /**
     * Closes the sockets to a peer and ignores its packets until it leaves the room. It is linked again once it (or a peer reusing its id) rejoins.
     */
    UFUNCTION(BlueprintCallable, meta = (ToolTip = "Closes the sockets to a peer and ignores its packets until it rejoins"), Category = "Odin|Multiplayer")
    void DisconnectPeer(int64 PeerId);

    /**
     * @return true if attached to a room that is joined
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Multiplayer")
    bool IsActive() const;

    /**
     * @return own peer id in the attached room, the id remote peers see as sender, or 0
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Multiplayer")
    int64 GetUniqueId() const;

    UFUNCTION(BlueprintPure, Category = "Odin|Multiplayer")
    TArray<int64> GetConnectedPeerIds() const;

    UFUNCTION(BlueprintPure, Category = "Odin|Multiplayer")
    bool IsPeerConnected(int64 PeerId) const;

    UFUNCTION(BlueprintPure, Category = "Odin|Multiplayer")
    UOdinRoom* GetRoom() const;

    /**
     * Frames a payload in the mesh wire format.
     */
    static void EncodePacket(int32 Channel, TConstArrayView<uint8> Payload, TArray<uint8>& OutMessage);
    /**
     * Parses a message in the mesh wire format.
     * @return false if the message is too short or has an unknown version
     */
    static bool DecodePacket(TConstArrayView<uint8> Message, int32& OutChannel, TArray<uint8>& OutPayload);

  protected:
    virtual void BeginDestroy() override;

  private:
    UFUNCTION()
    void HandlePeerJoined(UOdinRoom* InRoom, FOdinPeerJoined Data);
    UFUNCTION()
    void HandlePeerLeft(UOdinRoom* InRoom, FOdinPeerLeft Data);
    UFUNCTION()
    void HandleSocketMessage(UOdinSocket* Socket, TArray<uint8> Message);

    void LinkPeer(int64 PeerId);
    void UnbindRoom();
    void BroadcastPeerDisconnected(int64 PeerId);

    static void ResetLink(const FOdinMultiplayerPeerLink& Link);
    static bool SendToLink(const FOdinMultiplayerPeerLink& Link, EOdinSocketKind Mode, const TArray<uint8>& Message);

    TWeakObjectPtr<UOdinRoom> AttachedRoom;

    UPROPERTY()
    TMap<int64, FOdinMultiplayerPeerLink> Links;

    /** Peers unlinked with DisconnectPeer whose packets are ignored until they leave. */
    TSet<int64> DisconnectedPeerIds;
};
