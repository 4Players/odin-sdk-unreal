/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

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

void UOdinRoom::SetPassword(const FString Password) const
{
    if (IsValid(this->Crypto)) {

        TArray<uint8> Buffer;
        UOdinFunctionLibrary::OdinStringToBytes(Password, Buffer);
        this->Crypto->SetSecret(Buffer);
    }
}

void UOdinRoom::BeginDestroy()
{
    ODIN_LOG(Verbose, "ODIN Destroy: %s", ANSI_TO_TCHAR(__FUNCTION__));
    CloseRoom();
    Super::BeginDestroy();
}

void UOdinRoom::FinishDestroy()
{
    CleanupRoomInternal();
    UObject::FinishDestroy();
}

UOdinRoom *UOdinRoom::ConstructRoom(UObject *WorldContextObject)
{
    return NewObject<UOdinRoom>(WorldContextObject);
}

UOdinRoom *UOdinRoom::ConstructRoom(UObject *WorldContextObject, OdinRoom *handle, OdinCipher *crypto)
{
    UOdinRoom *result = NewObject<UOdinRoom>(WorldContextObject);

    result->SetHandle(handle);
    if (nullptr != crypto)
        result->Crypto = UOdinCrypto::ConstructCrypto(WorldContextObject, crypto);

    if (UOdinSubsystem *const &OdinSubsystem = UOdinSubsystem::Get()) {
        OdinSubsystem->RegisterRoom(handle, result);
    }
    return result;
}

UOdinRoom *UOdinRoom::ConnectRoom(FString gateway, FString authentication, bool &bSuccess, UOdinCrypto *crypto)
{
    FScopeLock ConnectionRoomLock(&Room_CS);
    OdinRoom  *room;
    auto       ret =
        odin_room_create(TCHAR_TO_UTF8(*gateway), TCHAR_TO_UTF8(*authentication), &this->Roomcb, (IsValid(crypto) ? crypto->GetHandle() : nullptr), &room);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        this->SetHandle(room);
        this->Crypto = crypto;

        if (auto esub = UOdinSubsystem::Get()) {
            esub->RegisterRoom(room, this);
        }
        bSuccess = true;
    } else {
        FOdinModule::LogErrorCode("Aborting ConstructRoom due to invalid odin_room_create call: %s", ret);
        bSuccess = false;
    }

    return this;
}

bool UOdinRoom::CloseRoom()
{
    return CloseOdinRoomByHandle(GetHandle());
}

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
    return FreeRoomByHandle(GetHandle());
}

bool UOdinRoom::FreeRoomByHandle(OdinRoom *handle)
{
    if (handle == nullptr) {
        ODIN_LOG(Verbose, "Aborted FreeRoom due to invalid Odin Room handle.");
        return false;
    }
    DeregisterRoom(handle);
    odin_room_free(handle);
    return true;
}

int64 UOdinRoom::GetOwnPeerId()
{
    return this->State.own_peer_id;
}

FString UOdinRoom::GetReconnectToken()
{
    return FString(this->ReconnectToken.token);
}

FName UOdinRoom::GetRoomName()
{
    return FName(this->State.room_id);
}

FOdinConnectionStats UOdinRoom::GetConnectionStats()
{
    OdinConnectionStats stats;
    auto                ret = odin_room_get_connection_stats(GetHandle(), &stats);
    if (ret != OdinError::ODIN_ERROR_SUCCESS)
        FOdinModule::LogErrorCode("Aborting GetConnectionStats due to invalid odin_room_get_connection_stats call: %s", ret);

    return FOdinConnectionStats(stats);
}

void UOdinRoom::SetRoomEvents(const OdinRoomEvents &roomcb)
{
    this->Roomcb = roomcb;
}

OdinRoomEvents *UOdinRoom::GetRoomEvents()
{
    return &this->Roomcb;
}

void UOdinRoom::RemoveRoomEvents()
{
    this->Roomcb = OdinRoomEvents{.on_datagram = OnDatagramFunc, .on_rpc = OnRpcFunc, .user_data = this};
}

OdinCipher *UOdinRoom::GetRoomCipher()
{
    return IsValid(Crypto) ? Crypto->GetHandle() : nullptr;
}

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
{
    return this->SendRpc(request.AsJson());
}

bool UOdinRoom::SetChannelMasks(const FOdinSetChannelMasks &request)
{
    return this->SendRpc(request.AsJson());
}

bool UOdinRoom::SetChannelMasks(TMap<int64, uint64> masks, bool reset)
{
    FOdinSetChannelMasks SetChannelMaskRequest(masks, reset);
    return SetChannelMasks(SetChannelMaskRequest);
}

bool UOdinRoom::SendMessage(const FOdinSendMessage &request)
{
    return this->SendRpc(request.AsJson());
}

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
        return false;
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
{
    return Status.status == FOdinRoomStatusChanged::JoinedStatus;
}

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
                    *EventObject, RoomObjectPtr,
                    [Delegate = RoomObjectPtr->OnRoomStatusChangedBP](TWeakObjectPtr<UOdinRoom> OdinRoom, FOdinRoomStatusChanged EventData) {
                        if (!OdinRoom.IsValid()) {
                            return;
                        }

                        OdinRoom->Status = EventData;

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
                    *EventObject, RoomObjectPtr,
                    [Delegate = RoomObjectPtr->OnRoomNewReconnectTokenBP](TWeakObjectPtr<UOdinRoom> room, FOdinNewReconnectToken data) {
                        if (!room.IsValid())
                            return;

                        room->ReconnectToken = data;

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
                && !DeserializeAndBroadcast<FOdinMessageReceived>(
                    *EventObject, RoomObjectPtr,
                    [Delegate = RoomObjectPtr->OnRoomMessageReceivedBP](TWeakObjectPtr<UOdinRoom> room, FOdinMessageReceived data) {
                        if (!room.IsValid())
                            return;

                        if (Delegate.IsBound()) {
                            Delegate.Broadcast(room.Get(), data);
                        }
                        ODIN_LOG(Verbose, "Successfully parsed event %s: %lld \"%s\"", *FOdinMessageReceived::Name, data.sender_peer_id, *data.message);
                    })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinMessageReceived::Name);
            }
            return;
        }

        // Joined
        if (ParsedRpc->TryGetObjectField(FOdinJoined::Name, EventObject)) {
            if (EventObject
                && !DeserializeAndBroadcast<FOdinJoined>(*EventObject, RoomObjectPtr,
                                                         [Delegate = RoomObjectPtr->OnRoomJoinedBP](TWeakObjectPtr<UOdinRoom> room, FOdinJoined data) {
                                                             if (!room.IsValid())
                                                                 return;

                                                             room->State = data;

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
            // Stringify (replace) "user_data" to maintain full custom user data
            StringifyRpcField(EventObject, "user_data");

            if (EventObject
                && !DeserializeAndBroadcast<FOdinPeerJoined>(
                    *EventObject, RoomObjectPtr, [Delegate = RoomObjectPtr->OnRoomPeerJoinedBP](TWeakObjectPtr<UOdinRoom> room, FOdinPeerJoined data) {
                        if (!room.IsValid())
                            return;

                        if (Delegate.IsBound()) {
                            Delegate.Broadcast(room.Get(), data);
                        }
                        ODIN_LOG(Verbose, "Successfully parsed event %s: %lld \"%s\"", *FOdinPeerJoined::Name, data.peer_id, *data.user_id);
                    })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinPeerJoined::Name);
            }
            return;
        }

        // PeerChanged
        if (ParsedRpc->TryGetObjectField(FOdinPeerChanged::Name, EventObject)) {
            // Stringify (replace) "user_data" and "parameters"
            StringifyRpcField(EventObject, "user_data");
            StringifyRpcField(EventObject, "parameters");

            if (EventObject
                && !DeserializeAndBroadcast<FOdinPeerChanged>(
                    *EventObject, RoomObjectPtr, [Delegate = RoomObjectPtr->OnRoomPeerChangedBP](TWeakObjectPtr<UOdinRoom> room, FOdinPeerChanged data) {
                        if (!room.IsValid())
                            return;

                        if (Delegate.IsBound()) {
                            Delegate.Broadcast(room.Get(), data);
                        }
                        ODIN_LOG(Verbose, "Successfully parsed event %s: %lld \"%s\"", *FOdinPeerChanged::Name, data.peer_id, *data.user_data);
                    })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinPeerChanged::Name);
            }
            return;
        }

        // PeerLeft
        if (ParsedRpc->TryGetObjectField(FOdinPeerLeft::Name, EventObject)) {
            if (EventObject
                && !DeserializeAndBroadcast<FOdinPeerLeft>(*EventObject, RoomObjectPtr,
                                                           [Delegate = RoomObjectPtr->OnRoomPeerLeftBP](TWeakObjectPtr<UOdinRoom> room, FOdinPeerLeft data) {
                                                               if (!room.IsValid())
                                                                   return;

                                                               if (Delegate.IsBound()) {
                                                                   Delegate.Broadcast(room.Get(), data);
                                                               }
                                                               ODIN_LOG(Verbose, "Successfully parsed event %s: %lld", *FOdinPeerLeft::Name, data.peer_id);
                                                           })) {
                ODIN_LOG(Error, "parsing event %s failed!", *FOdinPeerLeft::Name);
            }
            return;
        }
    }
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
