/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

#include <atomic>

#include "CoreMinimal.h"
#include "OdinCryptoExtension.h"
#include "OdinNative/OdinNativeHandle.h"
#include "OdinNative/OdinNativeRpc.h"
#include "OdinSocket.h"

#include "OdinRoom.generated.h"

class UOdinEncoder;
class UOdinSocket;
struct FOdinConnectionStats;

/**
 * An opaque type representing an ODIN room handle, which is managed by the underlying connection.
 * This abstraction provides a high-level interface for joining rooms, managing persistent state
 * and sending/receiving data, making it easier to integrate room-based interactions into your
 * application.
 */
UCLASS(ClassGroup = Odin, Blueprintable, BlueprintType)
class ODIN_API UOdinRoom : public UObject
{
    GENERATED_BODY()

  public:
    UOdinRoom(const class FObjectInitializer& PCIP);
    /**
     * Internal OnDatagram hook to redirect incoming callback for datagrams
     * @remarks This should only be changed if the underlying callback has to call a custom implementation of handling callbacks
     */
    void (*OnDatagramFunc)(OdinRoom* room, const struct OdinDatagramProperties* properties, const uint8_t* bytes, uint32_t bytes_length, void* user_data) =
        [](OdinRoom* room, const struct OdinDatagramProperties* properties, const uint8_t* bytes, uint32_t bytes_length, void* user_data) {
            TArray<uint8> Datagram = TArray<uint8>(bytes, bytes_length);
            ODIN_LOG(VeryVerbose, "Handle Odin Datagram with Channel Mask: %llu", properties->channel_mask);
            HandleOdinEventDatagram(room, properties->peer_id, properties->channel_mask, properties->ssrc_id, Datagram);
        };

    UDELEGATE(BlueprintAuthorityOnly)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinRoomRpcDelegate, UOdinRoom*, room, FString, json);

    /**
     * JSON-encoded RPC message from the server
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinRoomRpcDelegate OnRpcBP;
    /**
     * Internal OnRpc hook to redirect incoming callback for rpc messages
     * @remarks This should only be changed if the underlying callback has to call a custom implementation of handling callbacks
     */
    void (*OnRpcFunc)(struct OdinRoom* room, const char* json, void* user_data) = [](struct OdinRoom* room, const char* json, void* user_data) {
        HandleOdinEventRpc(room, FString(UTF8_TO_TCHAR(json)));
    };

    UDELEGATE(BlueprintAuthorityOnly)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinSocketDelegate, UOdinSocket*, socket, TArray<uint8>, message);

    /**
     * On Socket message from the server
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinSocketDelegate OnSocketBP;
    /**
     * Internal OnSocket hook to redirect incoming callback for socket messages
     * @remarks This should only be changed if the underlying callback has to call a custom implementation of handling callbacks
     */
    void (*OnSocketFunc)(OdinSocket* socket, const uint8_t* message, uint32_t message_length, void* user_data) = [](OdinSocket* socket, const uint8_t* message,
                                                                                                                    uint32_t message_length, void* user_data) {
        TArray<uint8> data = TArray<uint8>(message, message_length);
        ODIN_LOG(VeryVerbose, "Handle Odin Socket: %p", socket);

        HandleOdinEventSocket(socket, data);
    };

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinRoomStatusChangedDelegate, UOdinRoom*, room, FOdinRoomStatusChanged, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinRoomStatusChangedDelegate OnRoomStatusChangedBP;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinNewReconnectTokenDelegate, UOdinRoom*, room, FOdinNewReconnectToken, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinNewReconnectTokenDelegate OnRoomNewReconnectTokenBP;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinMessageReceivedDelegate, UOdinRoom*, room, FOdinMessageReceived, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinMessageReceivedDelegate OnRoomMessageReceivedBP;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinJoinedDelegate, UOdinRoom*, room, FOdinJoined, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinJoinedDelegate OnRoomJoinedBP;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinPeerJoinedDelegate, UOdinRoom*, room, FOdinPeerJoined, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerJoinedDelegate OnRoomPeerJoinedBP;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinPeerChangedDelegate, UOdinRoom*, room, FOdinPeerChanged, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerChangedDelegate OnRoomPeerChangedBP;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinPeerLeftDelegate, UOdinRoom*, room, FOdinPeerLeft, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerLeftDelegate OnRoomPeerLeftBP;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinErrorDelegate, UOdinRoom*, room, FOdinError, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinErrorDelegate OnRoomErrorBP;

    /**
     * Creates a new ODIN room handle and starts the asynchronous connection process.
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct Room", ToolTip = "Creates a new room", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                          Keywords = "Create,Create Room"),
              Category = "Odin")
    static UOdinRoom* ConstructRoom(UObject* WorldContextObject);

    static UOdinRoom* ConstructRoom(UObject* WorldContextObject, OdinRoom* handle, OdinCipher* crypto = nullptr);

    /**
     * Closes the specified ODIN room handle, thus making our own peer leave the room on the server and closing the connection if needed.
     */
    UFUNCTION(BlueprintCallable, Category = "Odin", meta = (Keywords = "Disconnect,Close Connection,Destroy Room"))
    bool CloseRoom();

    /**
     * Closes the specified ODIN room handle, thus making our own peer leave the room on the server
     * and closing the connection if needed.
     * @remarks To release resources, call `FreeRoomByHandle`.
     */
    static bool CloseOdinRoomByHandle(OdinRoom* room);

    /**
     * Destroys the specified ODIN room handle in addition to close.
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Free Room", ToolTip = "Frees a room and handle immediately.", DefaultToSelf = "Room",
                          Keywords = "Destroy Immediate,Destroy Room"),
              Category = "Odin")
    bool FreeRoom();
    /**
     * Destroys the specified ODIN room handle and releases all underlying resources.
     * @remarks Since the handle could be invalid for the SDK while connecting, manual call `odin_room_free` would work, even if the
     * room is still connecting.
     */
    static bool FreeRoomByHandle(OdinRoom* room);
    /**
     * Frees the native room (if any), drops the subsystem registration and invalidates the crypto
     * and all socket wrappers. Safe to call during destruction; every path that frees the native
     * room must go through this, so the handle cannot be freed twice.
     */
    void ReleaseHandle();
    /**
     * Invalidates all socket wrappers of this room without touching the native sockets. Used when
     * the native room is freed, since its sockets die with it.
     */
    void InvalidateAllSockets();
    /**
     * Monotonic counter that is bumped whenever the native room handle changes (connect, free).
     * Deferred event tasks compare it to detect that their captured native handles belong to a
     * previous connection, even if a new native room reuses the same pointer value.
     */
    uint64 GetConnectionGeneration() const
    { return ConnectionGeneration.load(); }

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Connect Room", ToolTip = "Creates the room in a connection pool",
                          Keywords = "Start Connection,Start Room,Join,JoinRoom,Join Room"),
              Category = "Odin")
    UOdinRoom* ConnectRoom(FString gateway, FString authentication, bool& bSuccess, UOdinCrypto* crypto = nullptr);
    /**
     * Connects this room via the native API using this object's registered event callbacks. Shares
     * the full lifecycle handling (reconnect cleanup, cipher ownership, subsystem registration)
     * with ConnectRoom, but returns the raw error code.
     */
    OdinError ConnectRoomNative(const FString& Gateway, const FString& Authentication, UOdinCrypto* InCrypto);
    /**
     * Get last retrieved peer id that represents "self".
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get own Peer ID", ToolTip = "Gets the own peer id from an already connected room"),
              Category = "Odin|Room|Info")
    int64 GetOwnPeerId();
    /**
     * Get last retrieved reconnect token.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Reconnect Token", ToolTip = "Gets the latest reconnect token of the connected room"),
              Category = "Odin|Room|Info")
    FString GetReconnectToken();
    /**
     * Retrieves the name from the current room.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Room Name", ToolTip = "Gets the name of the connected room"),
              Category = "Odin|Room|Info")
    FName GetRoomName();
    /**
     * Retrieves the connection stats.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Connection Stats", ToolTip = "Gets the connection stats of the connected room"),
              Category = "Odin|Room|Info")
    FOdinConnectionStats GetConnectionStats();

    /**
     * Sends a JSON-encoded RPC message to the server.
     * @param json   JSON rpc string
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Send Rpc", ToolTip = "Send raw rpc data"), Category = "Odin|Room|Rpc")
    bool SendRpc(FString json);
    /**
     * Call SendRpc with request format helper.
     * @param request   Change own peer request
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Send ChangeSelf Rpc", ToolTip = "Send change self rpc to set own data"), Category = "Odin|Room|Rpc")
    bool ChangeSelf(FOdinChangeSelf request);
    /**
     * Call SendRpc with request format helper.
     * @param request   Change channel mask
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Send SetChannelMasks Rpc", ToolTip = "Send audio channel masks rpc to set the channel layer mask of peers"),
              Category = "Odin|Room|Rpc")
    bool SetChannelMasks(const FOdinSetChannelMasks& request);
    /**
     * Call SendRpc to set channel masks.
     * @param masks Masks to set, with map from peer Id to channel mask
     * @param reset
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    bool SetChannelMasks(TMap<int64, uint64> masks, bool reset);

    /**
     * Sets the channel mask used when listening to peers that do not have a per-peer override set
     * via SetListenChannelMaskForPeer. Applies immediately to every currently known peer and is
     * automatically applied to peers that join afterward.
     * @remarks Convenience wrapper around SetChannelMasks; for one room instead of multiple rooms
     * @param Mask channel mask to listen to for all peers without an override
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Listen ChannelMask", ToolTip = "Set the default channel mask used when listening to peers"),
              Category = "Odin|Room|Rpc")
    bool SetListenChannelMask(FOdinChannelMask Mask);
    /**
     * Overrides the channel mask used when listening to a specific peer, taking precedence over
     * the default set via SetListenChannelMask. Useful for finer per-sender control, e.g. always
     * hearing party members on the non-spatialized group channel even when standing next to them.
     * @param PeerId peer to override the listen channel mask for
     * @param Mask channel mask to listen to for this peer
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Set Listen ChannelMask For Peer", ToolTip = "Override the channel mask used when listening to a specific peer"),
              Category = "Odin|Room|Rpc")
    bool SetListenChannelMaskForPeer(int64 PeerId, FOdinChannelMask Mask);
    /**
     * Removes a previously set per-peer listen channel mask override, falling back to the default
     * set via SetListenChannelMask for that peer.
     * @param PeerId peer to remove the override for
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Clear Listen ChannelMask For Peer", ToolTip = "Remove a per-peer listen channel mask override"),
              Category = "Odin|Room|Rpc")
    bool ClearListenChannelMaskForPeer(int64 PeerId);

    /**
     *
     * @param request Set Message
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Send Message Rpc", ToolTip = "Send a message to a set of peers"), Category = "Odin|Room|Rpc")
    bool SendMessage(const FOdinSendMessage& request);
    /**
     * Drain Encoder pipline for audio data and sends encoded voice packets to the server.
     * @param encoder   encoder to drain all datagrams
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Send Audio", ToolTip = "Send all audio datagrams until the encoder is empty"),
              Category = "Odin|Room|Audio Pipeline")
    bool SendAudio(UOdinEncoder* encoder);

    /**
     * Get room name
     * @return room as string representation
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Room")
    FString GetOdinRoomName() const;

    /**
     * Checks local status state
     * @return true if JoinedStatus
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Room")
    bool IsConnected() const;

    /**
     * Replaces the event callback struct used for future room creations.
     * @attention The native room clones the event struct on odin_room_create, so changes made here
     * have no effect on an already-created room; they only apply to the next connect.
     */
    void SetRoomEvents(const OdinRoomEvents& roomcb);
    /** @see SetRoomEvents for the remark on already-created rooms */
    OdinRoomEvents* GetRoomEvents();
    /** @see SetRoomEvents for the remark on already-created rooms */
    void        RemoveRoomEvents();
    OdinCipher* GetRoomCipher();

    inline OdinRoom* GetHandle() const
    { return IsValid(Handle) && Handle->IsValidLowLevel() ? static_cast<OdinRoom*>(Handle->GetHandle()) : nullptr; }

    inline void SetHandle(OdinRoom* handle)
    {
        if (handle == nullptr && IsValid(this->Handle)) {
            this->Handle->SetHandle(nullptr);
            return;
        }

        this->Handle = NewObject<UOdinHandle>();
        this->Handle->SetHandle(handle);
    }

    /**
     * Optional instance of pluggable encryption module for room communications. A cipher can be attached to the room on creation to enable customizable,
     * end-to-end encryption(E2EE). Needs ODIN_USE_CRYPTO with CryptoExtention and associated library.
     */
    UPROPERTY(BlueprintReadWrite, Category = "Odin|Room|Extensions")
    UOdinCrypto* Crypto;

    /**
     * Set a string password as arbitary shared secret.
     * @param  Password   shared secret
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Set Crypto Password",
                          ToolTip     = "Set string password as bytes if Crypto is set and valid (result may not align outside of UnrealEngine)"),
              Category = "Odin|Room|Extensions")
    void SetPassword(const FString Password) const;

    /**
     * Get Decoders by PeerId
     * @param PeerId  Registered Decoder PeerId
     * @remarks Used for registered/linked decoders in subsystem (GetDecodersFor)
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Odin|Room")
    TArray<UOdinDecoder*> GetDecodersByPeer(const int64 PeerId) const;

    /**
     * Create an uobject socket, sets the native handle and refreshes socket info
     * @param SocketKind  Kind of Socket i.e. reliable or unreliable
     * @param TargetPeerId  Remote PeerId
     * @param Label  Arbitrary Id
     * @param Priority  Packet transport priority
     * @remarks Used for sockets with ownership
     */
    TWeakObjectPtr<UOdinSocket> CreateLocalSocket(EOdinSocketKind SocketKind, int64 TargetPeerId, int32 Label, int32 Priority);
    /**
     * Create an uobject socket, sets the native handle and refreshes socket info
     * @param SocketHandle  Native Socket
     * @remarks Used for remote peer event sockets where this client does not have ownership
     */
    TWeakObjectPtr<UOdinSocket> CreateRemoteSocket(OdinSocket* SocketHandle);
    /**
     * Gets socket from this room
     * @param SocketHandle  Native Socket
     */
    TWeakObjectPtr<UOdinSocket> GetSocketByHandle(const OdinSocket* SocketHandle) const;
    /**
     * Gets socket from this room or creates a remote socket with the provided handle
     * @param SocketHandle  Native Socket
     */
    TWeakObjectPtr<UOdinSocket> GetOrCreateRoomSocket(OdinSocket* SocketHandle);
    /**
     * Close native socket and return uobject socket
     * @param SocketHandle  Native Socket
     */
    UOdinSocket* RemoveSocket(const OdinSocket* SocketHandle);
    /**
     * Open socket to a peer in the current room.
     * @param TargetPeerId  Remote PeerId
     * @param SocketKind    Kind of socket
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Open Socket", ToolTip = "Open socket to a peer in the current room."), Category = "Odin|Room|Socket")
    UOdinSocket* OpenSocket(int64 TargetPeerId, EOdinSocketKind SocketKind);
    /**
     * Close all sockets in the current room.
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove All Sockets", ToolTip = "Remove and close all sockets in the current room."),
              Category = "Odin|Room|Socket")
    void RemoveAllSockets();

  protected:
    virtual void BeginDestroy() override;
    virtual void FinishDestroy() override;

    OdinRoomEvents Roomcb = OdinRoomEvents{.on_datagram = OnDatagramFunc, .on_rpc = OnRpcFunc, .on_socket = OnSocketFunc, .user_data = this};

    UPROPERTY(BlueprintReadOnly, Category = "Odin|Room")
    FOdinRoomStatusChanged Status;
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Room")
    FOdinNewReconnectToken ReconnectToken;
    UPROPERTY(BlueprintReadOnly, Category = "Odin|Room")
    FOdinJoined State;

    template <typename EventType>
    static bool DeserializeAndBroadcast(const TSharedPtr<FJsonObject> EventObject, TWeakObjectPtr<UOdinRoom> Room,
                                        TFunction<void(TWeakObjectPtr<UOdinRoom>, EventType)> Delegate);

  private:
    UPROPERTY()
    UOdinHandle*             Handle;
    std::atomic<uint64>      ConnectionGeneration{0};
    FCriticalSection         Room_CS;
    FCriticalSection         Encoder_CS;
    static void              HandleOdinEventDatagram(OdinRoom* RoomHandle, uint32 PeerId, uint64 ChannelMask, uint32 SsrcId, TArray<uint8>& Datagram);
    static void              HandleOdinEventRpc(OdinRoom* RoomHandle, const FString& JsonString);
    static void              HandleOdinEventSocket(OdinSocket* SocketHandle, const TArray<uint8>& Message);
    static bool              StringifyRpcField(const TSharedPtr<FJsonObject>* EventObj, const FString& Field);
    static bool              NormalizeRpcField(const TSharedPtr<FJsonObject>* EventObj, const FString& Field);
    static void              DeregisterRoom(OdinRoom* NativeRoomHandle);
    mutable FCriticalSection Socket_CS;
    TMap<OdinSocket*, TWeakObjectPtr<UOdinSocket>> Sockets;

    /**
     * Rebuilds and (re-)sends the listen channel mask for every known peer, using per-peer
     * overrides where set and the default listen channel mask otherwise.
     */
    bool ApplyListenChannelMasks();

    FCriticalSection              ListenChannelMasksCS;
    TSet<int64>                   KnownPeerIds;
    TMap<int64, FOdinChannelMask> ListenChannelMaskOverrides;
    FOdinChannelMask              DefaultListenChannelMask     = FOdinChannelMask::CreateFull();
    bool                          bListenChannelMaskCustomized = false;

    void CleanupRoomInternal();
};