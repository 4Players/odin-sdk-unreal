/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinRoom.h"
#include "Async/TaskGraphInterfaces.h"
#include "OdinAudio/OdinEncoder.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinNative/OdinNativeRpc.h"
#include "OdinNative/OdinUtils.h"
#include "OdinSubsystem.h"
#include "JsonObjectConverter.h"
#include "OdinFunctionLibrary.h"
#include "OdinVoice.h"

UOdinRoom::UOdinRoom(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

void UOdinRoom::BeginDestroy()
{
    ODIN_LOG(Verbose, "ODIN Destroy: %s", ANSI_TO_TCHAR(__FUNCTION__));
    CloseRoom();
    ReleaseHandle();
    Super::BeginDestroy();
}

void UOdinRoom::FinishDestroy()
{
    CleanupRoomInternal();
    UObject::FinishDestroy();
}

UOdinRoom *UOdinRoom::ConstructRoom(UObject *WorldContextObject)
{ return NewObject<UOdinRoom>(WorldContextObject); }

UOdinRoom *UOdinRoom::ConstructRoom(UObject *WorldContextObject, OdinRoom *handle, OdinCipher *crypto)
{
    UOdinRoom *result = NewObject<UOdinRoom>(WorldContextObject);

    result->SetHandle(handle);
    if (nullptr != crypto) {
        result->Crypto = UOdinCrypto::ConstructCrypto(WorldContextObject, crypto);
        result->Crypto->MarkAttachedToRoom();
    }

    if (UOdinSubsystem *const &OdinSubsystem = UOdinSubsystem::Get()) {
        OdinSubsystem->RegisterRoom(handle, result);
    }
    return result;
}

UOdinRoom *UOdinRoom::ConnectRoom(FString gateway, FString authentication, bool &bSuccess, UOdinCrypto *crypto)
{
    const OdinError Ret = ConnectRoomNative(gateway, authentication, crypto);
    bSuccess            = Ret == OdinError::ODIN_ERROR_SUCCESS;
    if (!bSuccess) {
        FOdinModule::LogErrorCode("Aborting ConnectRoom due to invalid odin_room_create call: %s", Ret);
    }
    return this;
}

OdinError UOdinRoom::ConnectRoomNative(const FString &Gateway, const FString &Authentication, UOdinCrypto *InCrypto)
{
    FScopeLock ConnectionRoomLock(&Room_CS);
    OdinRoom  *room = nullptr;
    ODIN_LOG(VeryVerbose, "Send ConnectRoom for Gateway %s, Auth: %s", *Gateway, *Authentication);

    OdinCipher *CipherHandle = nullptr;
    if (IsValid(InCrypto)) {
        if (InCrypto->IsAttachedToRoom()) {
            ODIN_LOG(Error, "Aborting ConnectRoom: the provided crypto is already owned by another room.");
            return OdinError::ODIN_ERROR_ARGUMENT_INVALID_CIPHER;
        }
        CipherHandle = InCrypto->GetHandle();
        if (CipherHandle == nullptr) {
#if ODIN_USE_CRYPTO
            ODIN_LOG(Error, "Aborting ConnectRoom: the provided crypto has no valid cipher handle.");
            return OdinError::ODIN_ERROR_ARGUMENT_INVALID_CIPHER;
#else
            ODIN_LOG(Warning, "Crypto extension not compiled in, connecting without encryption.");
#endif
        }
    }

    if (OdinRoom *ExistingHandle = GetHandle()) {
        ODIN_LOG(Warning, "ConnectRoom called on a room with an existing handle, closing and freeing the previous room.");
        CloseOdinRoomByHandle(ExistingHandle);
        ReleaseHandle();
    }

    auto ret = odin_room_create(TCHAR_TO_UTF8(*Gateway), TCHAR_TO_UTF8(*Authentication), &this->Roomcb, CipherHandle, &room);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        this->SetHandle(room);
        ++ConnectionGeneration;
        this->Crypto = CipherHandle != nullptr ? InCrypto : nullptr;
        if (CipherHandle != nullptr) {
            InCrypto->MarkAttachedToRoom();
        }

        if (auto esub = UOdinSubsystem::Get()) {
            esub->RegisterRoom(room, this);
        }
    }
    return ret;
}

bool UOdinRoom::CloseRoom()
{ return CloseOdinRoomByHandle(GetHandle()); }

bool UOdinRoom::CloseOdinRoomByHandle(OdinRoom *handle)
{
    if (handle == nullptr) {
        ODIN_LOG(Verbose, "Aborted CloseRoom due to invalid Odin Room handle.");
        return false;
    }

    odin_room_close(handle);
    return true;
}

bool UOdinRoom::FreeRoom()
{
    if (GetHandle() == nullptr) {
        ODIN_LOG(Verbose, "Aborted FreeRoom due to invalid Odin Room handle.");
        return false;
    }
    ReleaseHandle();
    return true;
}

bool UOdinRoom::FreeRoomByHandle(OdinRoom *handle)
{
    if (handle == nullptr) {
        ODIN_LOG(Verbose, "Aborted FreeRoom due to invalid Odin Room handle.");
        return false;
    }

    if (UOdinSubsystem *const OdinSubsystem = UOdinSubsystem::Get()) {
        if (TWeakObjectPtr<UOdinRoom> Owner = OdinSubsystem->GetRoomByHandle(handle); Owner.IsValid()) {
            Owner->ReleaseHandle();
            return true;
        }
    }

    DeregisterRoom(handle);
    odin_room_free(handle);
    return true;
}

void UOdinRoom::ReleaseHandle()
{
    InvalidateAllSockets();

    bool bFreedRoom = false;
    if (OdinRoom *RoomHandle = GetHandle()) {
        DeregisterRoom(RoomHandle);
        odin_room_free(RoomHandle);
        SetHandle(nullptr);
        bFreedRoom = true;
    }

    if (bFreedRoom && IsValid(Crypto) && Crypto->IsAttachedToRoom()) {
        Crypto->InvalidateHandle();
    }
    Crypto = nullptr;

    ++ConnectionGeneration;
}

int64 UOdinRoom::GetOwnPeerId()
{ return this->State.own_peer_id; }

FString UOdinRoom::GetReconnectToken()
{ return FString(this->ReconnectToken.token); }

FName UOdinRoom::GetRoomName()
{ return FName(this->State.room_id); }

FOdinConnectionStats UOdinRoom::GetConnectionStats()
{
    OdinConnectionStats stats = {};
    auto                ret   = odin_room_get_connection_stats(GetHandle(), &stats);
    if (ret != OdinError::ODIN_ERROR_SUCCESS)
        FOdinModule::LogErrorCode("Aborting GetConnectionStats due to invalid odin_room_get_connection_stats call: %s", ret);

    return FOdinConnectionStats(stats);
}

void UOdinRoom::SetRoomEvents(const OdinRoomEvents &roomcb)
{ this->Roomcb = roomcb; }

OdinRoomEvents *UOdinRoom::GetRoomEvents()
{ return &this->Roomcb; }

void UOdinRoom::RemoveRoomEvents()
{ this->Roomcb = OdinRoomEvents{.on_datagram = OnDatagramFunc, .on_rpc = OnRpcFunc, .on_socket = OnSocketFunc, .user_data = this}; }

OdinCipher *UOdinRoom::GetRoomCipher()
{ return IsValid(Crypto) ? Crypto->GetHandle() : nullptr; }

bool UOdinRoom::SendRpc(FString json)
{
    auto ret = odin_room_send_rpc(this->GetHandle(), TCHAR_TO_UTF8(*json));
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        ODIN_LOG(Verbose, "SendRpc: %s", *json)
        return true;
    } else {
        FOdinModule::LogErrorCode("Aborting SendRpc due to invalid odin_room_send_rpc call: %s", ret);
    }
    return false;
}

bool UOdinRoom::ChangeSelf(FOdinChangeSelf request)
{ return this->SendRpc(request.AsJson()); }

bool UOdinRoom::SetChannelMasks(const FOdinSetChannelMasks &request)
{ return this->SendRpc(request.AsJson()); }

bool UOdinRoom::SetChannelMasks(TMap<int64, uint64> masks, bool reset)
{
    FOdinSetChannelMasks SetChannelMaskRequest(masks, reset);
    return SetChannelMasks(SetChannelMaskRequest);
}

bool UOdinRoom::SetListenChannelMask(FOdinChannelMask Mask)
{
    FScopeLock Lock(&ListenChannelMasksCS);
    DefaultListenChannelMask     = Mask;
    bListenChannelMaskCustomized = true;
    return ApplyListenChannelMasks();
}

bool UOdinRoom::SetListenChannelMaskForPeer(int64 PeerId, FOdinChannelMask Mask)
{
    FScopeLock Lock(&ListenChannelMasksCS);
    ListenChannelMaskOverrides.Add(PeerId, Mask);
    bListenChannelMaskCustomized = true;
    return ApplyListenChannelMasks();
}

bool UOdinRoom::ClearListenChannelMaskForPeer(int64 PeerId)
{
    FScopeLock Lock(&ListenChannelMasksCS);
    if (ListenChannelMaskOverrides.Remove(PeerId) == 0) {
        return true;
    }
    return ApplyListenChannelMasks();
}

bool UOdinRoom::ApplyListenChannelMasks()
{
    if (!bListenChannelMaskCustomized || KnownPeerIds.IsEmpty()) {
        return true;
    }

    TMap<int64, uint64> Masks;
    Masks.Reserve(KnownPeerIds.Num());
    for (const int64 PeerId : KnownPeerIds) {
        if (const FOdinChannelMask *Override = ListenChannelMaskOverrides.Find(PeerId)) {
            Masks.Add(PeerId, *Override);
        } else {
            Masks.Add(PeerId, DefaultListenChannelMask);
        }
    }

    return SetChannelMasks(Masks, /*reset=*/true);
}

bool UOdinRoom::SendMessage(const FOdinSendMessage &request)
{ return this->SendRpc(request.AsJson()); }

void UOdinRoom::HandleOdinEventDatagram(OdinRoom *RoomHandle, uint32 PeerId, uint64 ChannelMask, uint32 SsrcId, TArray<uint8> &Datagram)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinRoom::HandleOdinEventDatagram)
    ODIN_LOG(VeryVerbose, "Received HandleOdinEventDatagram for Room %p, PeerId %u, SsrcId %u, ChannelMask %llu", RoomHandle, PeerId, SsrcId, ChannelMask);
    if (Datagram.IsEmpty()) {
        return;
    }

    if (const auto OdinSubsystem = UOdinSubsystem::Get()) {
        OdinSubsystem->HandleDatagram(RoomHandle, PeerId, ChannelMask, SsrcId, MoveTemp(Datagram));
    }
}

bool UOdinRoom::SendAudio(UOdinEncoder *encoder)
{
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(UOdinRoom::SendAudio);
        FScopeLock lock(&Encoder_CS);

        if (!IsValid(encoder))
            return false;

        TArray<uint8> bytes;
        auto          bytesSize = 1300;
        bytes.AddZeroed(bytesSize);

        for (;;) {
            bytes.Reset(bytesSize);
            uint32_t  length = bytes.GetSlack();
            OdinError ret;
            {
                TRACE_CPUPROFILER_EVENT_SCOPE(odin_encoder_pop);
                ret = odin_encoder_pop(encoder->GetHandle(), bytes.GetData(), &length);
            }
            switch (ret) {
                case ODIN_ERROR_SUCCESS: {
                    TRACE_CPUPROFILER_EVENT_SCOPE(odin_room_send_datagram);
                    auto dret = odin_room_send_datagram(this->GetHandle(), bytes.GetData(), length);
                    if (dret != OdinError::ODIN_ERROR_SUCCESS) {
                        FOdinModule::LogErrorCode("Aborting SendAudio due to invalid "
                                                  "odin_room_send_datagram call: %s",
                                                  dret);
                        return false;
                    }
                } break;
                case ODIN_ERROR_NO_DATA:
                    return true;
                default:
                    FOdinModule::LogErrorCode("Aborting SendAudio due to invalid odin_encoder_pop call: %s", ret);
                    return false;
            }
        }
    }
}

FString UOdinRoom::GetOdinRoomName() const
{
    FString Name = "";
    if (const auto RoomHandle = GetHandle()) {
        TArray<char> NameChars;
        NameChars.SetNumZeroed(1024 + 1);
        uint32     Length = NameChars.Num();
        const auto Result = odin_room_get_name(RoomHandle, NameChars.GetData(), &Length);
        if (Result == ODIN_ERROR_SUCCESS) {
            Name = FString(StringCast<UTF8CHAR>(NameChars.GetData(), Length));
        }
    }
    return Name;
}

bool UOdinRoom::IsConnected() const
{ return Status.status == FOdinRoomStatusChanged::JoinedStatus; }

void UOdinRoom::HandleOdinEventRpc(OdinRoom *RoomHandle, const FString &JsonString)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinRoom::HandleOdinEventRpc)
    if (JsonString.IsEmpty()) {
        return;
    }

    ODIN_LOG(VeryVerbose, "Received HandleOdinEventRpc for Room %p, Message: %s", RoomHandle, *JsonString);

    const UOdinSubsystem *OdinSubsystem = UOdinSubsystem::Get();
    if (!OdinSubsystem || !OdinSubsystem->IsRoomRegistered(RoomHandle)) {
        return;
    }

    TWeakObjectPtr<UOdinRoom> RoomObjectPtr = OdinSubsystem->GetRoomByHandle(RoomHandle);
    if (!RoomObjectPtr.IsValid() || RoomObjectPtr.IsStale(true, true)) {
        return;
    }

    {
        // dispatch raw rpc
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [RoomObjectPtr, JsonString]() {
                if (RoomObjectPtr.IsValid() && RoomObjectPtr->OnRpcBP.IsBound())
                    RoomObjectPtr->OnRpcBP.Broadcast(RoomObjectPtr.Get(), JsonString);
            },
            TStatId(), nullptr, ENamedThreads::GameThread);

        TSharedPtr<FJsonObject>              ParsedRpc;
        const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonString);
        if (!FJsonSerializer::Deserialize(JsonReader, ParsedRpc)) {
            ODIN_LOG(Error, "HandleOdinEventRpc failed to deserialize data: %s", *JsonString);
            return;
        }

        // RoomStatusChanged
        const TSharedPtr<FJsonObject> *EventObject;
        if (ParsedRpc->TryGetObjectField(FOdinRoomStatusChanged::Name, EventObject)) {
            if (EventObject) {
                const bool bSuccess = DeserializeAndBroadcast<FOdinRoomStatusChanged>(
                    *EventObject, RoomObjectPtr, [](TWeakObjectPtr<UOdinRoom> OdinRoom, FOdinRoomStatusChanged EventData) {
                        if (!OdinRoom.IsValid()) {
                            return;
                        }

                        OdinRoom->Status = EventData;

                        FOdinRoomStatusChangedDelegate Delegate = OdinRoom->OnRoomStatusChangedBP;
                        if (Delegate.IsBound()) {
                            Delegate.Broadcast(OdinRoom.Get(), EventData);
                        }

                        if (EventData.status == FOdinRoomStatusChanged::ClosedStatus) {
                            ODIN_LOG(Warning, "room connection closed: \"%s\"", *EventData.message);
                        }
                        ODIN_LOG(Verbose, "Successfully parsed event %s: state: %s, msg: \"%s\"", *FOdinRoomStatusChanged::Name, *EventData.status,
                                 *EventData.message);
                    });
                if (!bSuccess) {
                    ODIN_LOG(Error, "Parsing event %s failed!", *FOdinRoomStatusChanged::Name);
                }
            }
            return;
        }

        // NewReconnectToken
        if (ParsedRpc->TryGetObjectField(FOdinNewReconnectToken::Name, EventObject)) {
            if (EventObject
                && !DeserializeAndBroadcast<FOdinNewReconnectToken>(
                    *EventObject, RoomObjectPtr, [](TWeakObjectPtr<UOdinRoom> room, FOdinNewReconnectToken data) {
                        if (!room.IsValid()) {
                            return;
                        }

                        room->ReconnectToken = data;

                        FOdinNewReconnectTokenDelegate Delegate = room->OnRoomNewReconnectTokenBP;
                        if (Delegate.IsBound()) {
                            Delegate.Broadcast(room.Get(), data);
                        }
                        ODIN_LOG(Verbose, "Successfully parsed event %s: \"%s\"", *FOdinNewReconnectToken::Name, *data.token);
                    })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinNewReconnectToken::Name);
            }
            return;
        }

        // MessageReceived
        if (ParsedRpc->TryGetObjectField(FOdinMessageReceived::Name, EventObject)) {
            // Stringify (replace) "user_data" to maintain full custom user data
            StringifyRpcField(EventObject, "message");

            if (EventObject
                && !DeserializeAndBroadcast<FOdinMessageReceived>(*EventObject, RoomObjectPtr, [](TWeakObjectPtr<UOdinRoom> room, FOdinMessageReceived data) {
                       if (!room.IsValid()) {
                           return;
                       }

                       FOdinMessageReceivedDelegate Delegate = room->OnRoomMessageReceivedBP;
                       if (Delegate.IsBound()) {
                           Delegate.Broadcast(room.Get(), data);
                       }
                       ODIN_LOG(Verbose, "Successfully parsed event %s: %lld", *FOdinMessageReceived::Name, data.sender_peer_id);
                   })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinMessageReceived::Name);
            }
            return;
        }

        // Joined
        if (ParsedRpc->TryGetObjectField(FOdinJoined::Name, EventObject)) {
            if (EventObject && !DeserializeAndBroadcast<FOdinJoined>(*EventObject, RoomObjectPtr, [](TWeakObjectPtr<UOdinRoom> room, FOdinJoined data) {
                    if (!room.IsValid())
                        return;

                    room->State                  = data;
                    FOdinJoinedDelegate Delegate = room->OnRoomJoinedBP;
                    if (Delegate.IsBound()) {
                        Delegate.Broadcast(room.Get(), data);
                    }
                    ODIN_LOG(Verbose, "Successfully parsed event %s: \"%s\"", *FOdinJoined::Name, *data.room_id);
                })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinJoined::Name);
            }
            return;
        }

        // PeerJoined
        if (ParsedRpc->TryGetObjectField(FOdinPeerJoined::Name, EventObject)) {
            // Normalize "user_data" as raw bytes, regardless of raw byte array, plain/escaped string, or nested JSON object
            NormalizeRpcField(EventObject, "user_data");

            auto BroadcastDelegate = [](TWeakObjectPtr<UOdinRoom> room, FOdinPeerJoined data) {
                if (!room.IsValid()) {
                    return;
                }

                {
                    FScopeLock Lock(&room->ListenChannelMasksCS);
                    room->KnownPeerIds.Add(data.peer_id);
                }
                room->ApplyListenChannelMasks();

                FOdinPeerJoinedDelegate Delegate = room->OnRoomPeerJoinedBP;
                if (Delegate.IsBound()) {
                    Delegate.Broadcast(room.Get(), data);
                }
                ODIN_LOG(Verbose, "Successfully parsed event %s: %lld \"%s\"", *FOdinPeerJoined::Name, data.peer_id, *data.user_id);
            };

            const bool bParsingSuccess = !DeserializeAndBroadcast<FOdinPeerJoined>(*EventObject, RoomObjectPtr, BroadcastDelegate);
            if (EventObject && bParsingSuccess) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinPeerJoined::Name);
            }
            return;
        }

        // PeerChanged
        if (ParsedRpc->TryGetObjectField(FOdinPeerChanged::Name, EventObject)) {
            // Normalize "user_data" to raw bytes like PeerJoined
            NormalizeRpcField(EventObject, "user_data");
            StringifyRpcField(EventObject, "parameters");

            if (EventObject
                && !DeserializeAndBroadcast<FOdinPeerChanged>(*EventObject, RoomObjectPtr, [](TWeakObjectPtr<UOdinRoom> room, FOdinPeerChanged data) {
                       if (!room.IsValid()) {
                           return;
                       }

                       FOdinPeerChangedDelegate Delegate = room->OnRoomPeerChangedBP;
                       if (Delegate.IsBound()) {
                           Delegate.Broadcast(room.Get(), data);
                       }
                       ODIN_LOG(Verbose, "Successfully parsed event %s: %lld", *FOdinPeerChanged::Name, data.peer_id);
                   })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinPeerChanged::Name);
            }
            return;
        }

        // PeerLeft
        if (ParsedRpc->TryGetObjectField(FOdinPeerLeft::Name, EventObject)) {
            if (EventObject && !DeserializeAndBroadcast<FOdinPeerLeft>(*EventObject, RoomObjectPtr, [](TWeakObjectPtr<UOdinRoom> room, FOdinPeerLeft data) {
                    if (!room.IsValid()) {
                        return;
                    }
                    {
                        FScopeLock Lock(&room->ListenChannelMasksCS);
                        room->KnownPeerIds.Remove(data.peer_id);
                        room->ListenChannelMaskOverrides.Remove(data.peer_id);
                    }
                    FOdinPeerLeftDelegate Delegate = room->OnRoomPeerLeftBP;
                    if (Delegate.IsBound()) {
                        Delegate.Broadcast(room.Get(), data);
                    }
                    ODIN_LOG(Verbose, "Successfully parsed event %s: %lld", *FOdinPeerLeft::Name, data.peer_id);
                })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinPeerLeft::Name);
            }
            return;
        }

        // Error
        if (ParsedRpc->TryGetObjectField(FOdinError::Name, EventObject)) {
            if (EventObject && !DeserializeAndBroadcast<FOdinError>(*EventObject, RoomObjectPtr, [](TWeakObjectPtr<UOdinRoom> room, FOdinError data) {
                    ODIN_LOG(VeryVerbose, "Error event: %s", *data.message);
                    if (!room.IsValid()) {
                        return;
                    }
                    FOdinErrorDelegate Delegate = room->OnRoomErrorBP;
                    if (Delegate.IsBound()) {
                        Delegate.Broadcast(room.Get(), data);
                    }
                    ODIN_LOG(Verbose, "Successfully parsed event %s: %s", *FOdinError::Name, *data.message);
                })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinError::Name);
            }
            return;
        }
    }
}

void UOdinRoom::SetPassword(const FString Password) const
{
    if (IsValid(this->Crypto)) {

        TArray<uint8> Buffer;
        UOdinFunctionLibrary::OdinStringToBytes(Password, Buffer);
        this->Crypto->SetSecret(Buffer);
    }
}

TArray<UOdinDecoder *> UOdinRoom::GetDecodersByPeer(const int64 PeerId) const
{
    if (const UOdinSubsystem *OdinSubsystem = UOdinSubsystem::Get()) {
        return OdinSubsystem->GetDecodersFor(GetHandle(), PeerId);
    }
    return {};
}

TWeakObjectPtr<UOdinSocket> UOdinRoom::CreateLocalSocket(EOdinSocketKind SocketKind, int64 TargetPeerId, int32 Label, int32 Priority)
{
    FScopeLock   OpenSocketLock(&Socket_CS);
    UOdinSocket *socket = UOdinSocket::ConstructSocket(this);
    socket->Create(this, SocketKind, TargetPeerId, Label, Priority);
    ODIN_LOG(Log, "Create normal Socket %p in Room %p", socket->GetNativeHandle(), Handle);

    Sockets.Emplace(socket->GetNativeHandle(), MoveTemp(socket));
    return socket;
}

TWeakObjectPtr<UOdinSocket> UOdinRoom::CreateRemoteSocket(OdinSocket *SocketHandle)
{
    FScopeLock OpenSocketLock(&Socket_CS);
    ODIN_LOG(Log, "Create remote Socket %p in Room %p", SocketHandle, Handle);
    UOdinSocket *socket = UOdinSocket::ConstructSocket(this, SocketHandle);
    socket->GetSocketInfo();

    Sockets.Emplace(socket->GetNativeHandle(), MoveTemp(socket));
    return socket;
}

UOdinSocket *UOdinRoom::OpenSocket(int64 TargetPeerId, EOdinSocketKind SocketKind)
{
    const int32                 label  = (int32)(intptr_t)this->GetHandle();
    TWeakObjectPtr<UOdinSocket> Socket = CreateLocalSocket(SocketKind, TargetPeerId, label, 0);
    return Socket.Get();
}
UOdinSocket *UOdinRoom::RemoveSocket(const OdinSocket *SocketHandle)
{
    FScopeLock CloseSocketLock(&Socket_CS);
    if (UOdinSocket *Socket = Sockets.FindAndRemoveChecked(SocketHandle).Get()) {
        return Socket;
    }
    return nullptr;
}
void UOdinRoom::RemoveAllSockets()
{
    FScopeLock ResetSocketLock(&Socket_CS);
    for (auto &kvp : Sockets) {
        if (auto socket = kvp.Value.Get())
            socket->ResetSocket();
    }
    Sockets.Empty();
}

void UOdinRoom::InvalidateAllSockets()
{
    FScopeLock ResetSocketLock(&Socket_CS);
    for (auto &kvp : Sockets) {
        if (auto socket = kvp.Value.Get())
            socket->InvalidateHandle();
    }
    Sockets.Empty();
}

TWeakObjectPtr<UOdinSocket> UOdinRoom::GetSocketByHandle(const OdinSocket *SocketHandle) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinRoom::GetSocketByHandle);
    FScopeLock GetSocketLock(&Socket_CS);

    if (const TWeakObjectPtr<UOdinSocket> *SocketObject = Sockets.Find(SocketHandle)) {
        ODIN_LOG(Verbose, "Retrieved Odin Socket with handle %p", Handle);
        return *SocketObject;
    }
    ODIN_LOG(Verbose, "Did not find Odin Socket with handle %p", Handle);
    return nullptr;
}

TWeakObjectPtr<UOdinSocket> UOdinRoom::GetOrCreateRoomSocket(OdinSocket *SocketHandle)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinRoom::GetOrCreateRoomSocket);
    TWeakObjectPtr<UOdinSocket> Socket = GetSocketByHandle(SocketHandle);
    if (Socket.IsValid())
        return Socket;

    return CreateRemoteSocket(SocketHandle);
}

void UOdinRoom::HandleOdinEventSocket(OdinSocket *SocketHandle, const TArray<uint8> &Message)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinRoom::HandleOdinEventSocket)
    ODIN_LOG(VeryVerbose, "Received HandleOdinEventSocket for Socket %p, size: %d", SocketHandle, Message.Num());

    OdinSocketInfo info;
    auto           ret = odin_socket_info(SocketHandle, &info);
    if (ret != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Aborting HandleOdinEventSocket due to invalid odin_socket_info call: %s", ret);
        return;
    }
    FOdinSocketInfo SocketInfo = FOdinSocketInfo(info);
    if (!SocketInfo.Room.IsValid()) {
        return;
    }
    const uint64 Generation = SocketInfo.Room->GetConnectionGeneration();

    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [SocketInfo, SocketHandle, Generation, Message]() {
            // the room may have been freed or reconnected between queueing and execution
            if (!SocketInfo.Room.IsValid() || SocketInfo.Room->GetConnectionGeneration() != Generation) {
                return;
            }
            TWeakObjectPtr<UOdinSocket> socket = SocketInfo.Room->GetOrCreateRoomSocket(SocketHandle);

            // dispatch raw to room and to socket object
            if (SocketInfo.Room->OnSocketBP.IsBound())
                SocketInfo.Room->OnSocketBP.Broadcast(socket.Get(), Message);

            if (socket.IsValid() && socket->OnSocketMessageReceivedBP.IsBound())
                socket->OnSocketMessageReceivedBP.Broadcast(SocketInfo.Room.Get(), socket.Get(), Message);
        },
        TStatId(), nullptr, ENamedThreads::GameThread);
}

bool UOdinRoom::StringifyRpcField(const TSharedPtr<FJsonObject> *EventObj, const FString &Field)
{
    if (!EventObj) {
        return false;
    }

    FJsonObject *EventObjRef = EventObj->Get();
    if (!EventObjRef) {
        return false;
    }

    const TSharedPtr<FJsonObject> *ObjectField;
    if (EventObjRef->TryGetObjectField(*Field, ObjectField)) {
        FString                                             OutputString;
        TSharedRef<OdinUtility::FCondensedJsonStringWriter> TemporaryWriter = OdinUtility::FCondensedJsonStringWriterFactory::Create(&OutputString);
        if (FJsonSerializer::Serialize(ObjectField->ToSharedRef(), TemporaryWriter)) {
            UE_LOG(Odin, VeryVerbose, TEXT("Convert event object field \"%s\" to string: \"%s\""), *Field, *OutputString);

            TSharedRef<FJsonValueString> ValueRef = MakeShared<FJsonValueString>(*OutputString);
            EventObjRef->SetField(*Field, ValueRef);
            return true;
        } else {
            ODIN_LOG(Warning, "StringifyRpcField failed to serialize field %s from a JsonObject.", *Field);
        }
    }

    return false;
}

bool UOdinRoom::NormalizeRpcField(const TSharedPtr<FJsonObject> *EventObj, const FString &Field)
{
    if (!EventObj) {
        return false;
    }

    FJsonObject *EventObjRef = EventObj->Get();
    if (!EventObjRef) {
        return false;
    }

    const TSharedPtr<FJsonValue> FieldValue = EventObjRef->TryGetField(*Field);
    if (!FieldValue.IsValid()) {
        return false;
    }

    // already arbitrary data; JsonObjectToUStruct converts directly into a TArray<uint8>
    if (FieldValue->Type == EJson::Array) {
        return true;
    }

    FString TextValue;
    if (FieldValue->Type == EJson::String) {
        // plain text or a JSON-escaped string payload
        TextValue = FieldValue->AsString();
    } else {
        // re-serialize other types/objects back to JSON.
        TSharedRef<OdinUtility::FCondensedJsonStringWriter> TemporaryWriter = OdinUtility::FCondensedJsonStringWriterFactory::Create(&TextValue);
        if (!FJsonSerializer::Serialize(FieldValue, FString(), TemporaryWriter)) {
            ODIN_LOG(Warning, "NormalizeRpcField failed to serialize field %s from a JsonObject.", *Field);
            return false;
        }
    }

    UE_LOG(Odin, VeryVerbose, TEXT("Convert event object field \"%s\" to bytes from: \"%s\""), *Field, *TextValue);

    FTCHARToUTF8                   Utf8Text(*TextValue);
    TArray<TSharedPtr<FJsonValue>> ByteValues;
    ByteValues.Reserve(Utf8Text.Length());
    for (int32 Index = 0; Index < Utf8Text.Length(); ++Index) {
        ByteValues.Add(MakeShared<FJsonValueNumber>(static_cast<uint8>(Utf8Text.Get()[Index])));
    }

    EventObjRef->SetField(*Field, MakeShared<FJsonValueArray>(ByteValues));
    return true;
}

void UOdinRoom::DeregisterRoom(OdinRoom *NativeRoomHandle)
{
    if (UOdinSubsystem *const &OdinSubsystem = UOdinSubsystem::Get()) {
        OdinSubsystem->DeregisterRoom(NativeRoomHandle);
    }
}

void UOdinRoom::CleanupRoomInternal()
{
    DeregisterRoom(GetHandle());
    OnDatagramFunc = nullptr;
    OnRpcFunc      = nullptr;
    OnSocketFunc   = nullptr;
}

template <typename EventType>
bool UOdinRoom::DeserializeAndBroadcast(const TSharedPtr<FJsonObject> EventObject, TWeakObjectPtr<UOdinRoom> OdinRoom,
                                        TFunction<void(TWeakObjectPtr<UOdinRoom>, EventType)> Delegate)
{
    if (!EventObject) {
        return false;
    }

    EventType EventData;
    if (!FJsonObjectConverter::JsonObjectToUStruct(EventObject.ToSharedRef(), &EventData, 0, 0)) {
        return false;
    }

    FFunctionGraphTask::CreateAndDispatchWhenReady([OdinRoom, Delegate, EventData]() { Delegate(OdinRoom, EventData); }, TStatId(), nullptr,
                                                   ENamedThreads::GameThread);

    return true;
}
