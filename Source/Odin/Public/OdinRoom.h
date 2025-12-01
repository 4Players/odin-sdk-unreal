/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

#include "CoreMinimal.h"
#include "OdinCryptoExtension.h"
#include "OdinNative/OdinNativeHandle.h"
#include "OdinNative/OdinNativeRpc.h"

#include "OdinRoom.generated.h"

class UOdinEncoder;
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
        HandleOdinEventRpc(room, FString(json));
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

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Connect Room", ToolTip = "Creates the room in a connection pool", Keywords = "Start Connection,Start Room"),
              Category = "Odin")
    UOdinRoom* ConnectRoom(FString gateway, FString authentication, bool& bSuccess, UOdinCrypto* crypto = nullptr);
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
     * @param json   json rpc string
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

    void            SetRoomEvents(const OdinRoomEvents& roomcb);
    OdinRoomEvents* GetRoomEvents();
    void            RemoveRoomEvents();
    OdinCipher*     GetRoomCipher();

    inline OdinRoom* GetHandle() const
    {
        return IsValid(Handle) && Handle->IsValidLowLevel() ? static_cast<OdinRoom*>(Handle->GetHandle()) : nullptr;
    }

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

  protected:
    virtual void BeginDestroy() override;
    virtual void FinishDestroy() override;

    OdinRoomEvents Roomcb = OdinRoomEvents{.on_datagram = OnDatagramFunc, .on_rpc = OnRpcFunc, .user_data = this};

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
    UOdinHandle*     Handle;
    FCriticalSection Room_CS;
    FCriticalSection Encoder_CS;
    static void      HandleOdinEventDatagram(OdinRoom* RoomHandle, uint32 PeerId, uint64 ChannelMask, uint32 SsrcId, TArray<uint8>& Datagram);
    static void      HandleOdinEventRpc(OdinRoom* RoomHandle, const FString& JsonString);
    static bool      StringifyRpcField(const TSharedPtr<FJsonObject>* EventObj, const FString& Field);
    static void      DeregisterRoom(OdinRoom* NativeRoomHandle);

    void CleanupRoomInternal();
};