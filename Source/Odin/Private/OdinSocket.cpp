/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinSocket.h"
#include "Async/TaskGraphInterfaces.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinNative/OdinNativeRpc.h"
#include "OdinNative/OdinUtils.h"
#include "OdinSubsystem.h"
#include "OdinRoom.h"
#include "OdinVoice.h"

UOdinSocket::UOdinSocket(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

void UOdinSocket::BeginDestroy()
{
    ODIN_LOG(Verbose, "ODIN Destroy: %s", ANSI_TO_TCHAR(__FUNCTION__));
    // ResetSocket(); // no need to reset on game object destroy -> freed native by room
    Super::BeginDestroy();
}

void UOdinSocket::FinishDestroy()
{ UObject::FinishDestroy(); }

UOdinSocket *UOdinSocket::ConstructSocket(UObject *WorldContextObject)
{ return NewObject<UOdinSocket>(WorldContextObject); }

UOdinSocket *UOdinSocket::ConstructSocket(UObject *WorldContextObject, OdinSocket *handle)
{
    UOdinSocket *result = NewObject<UOdinSocket>(WorldContextObject);

    result->SetHandle(handle);
    return result;
}

UOdinSocket *UOdinSocket::Create(UOdinRoom *Room, EOdinSocketKind SocketKind, int64 TargetPeerId, int32 Label, int32 Priority)
{
    FScopeLock  ConnectionSocketLock(&Socket_CS);
    OdinSocket *socket;
    auto        ret = odin_socket_create(Room->GetHandle(), static_cast<OdinSocketKind>(SocketKind), TargetPeerId, Label, Priority, &socket);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        this->SetHandle(socket);
    } else {
        FOdinModule::LogErrorCode("Aborting CreateSocket due to invalid odin_socket_create call: %s", ret);
    }

    return this;
}

bool UOdinSocket::ResetSocket()
{ return ResetOdinSocketByHandle(GetNativeHandle()); }

bool UOdinSocket::ResetOdinSocketByHandle(OdinSocket *handle)
{
    if (handle == nullptr) {
        ODIN_LOG(Verbose, "Aborted ResetSocket due to invalid Odin Socket handle.");
        return false;
    }

    odin_socket_reset(handle);
    return true;
}
int64 UOdinSocket::GetRemotePeerId()
{ return this->Info.RemotePeerId; }

int32 UOdinSocket::GetSocketLabel()
{ return this->Info.Label; }

FName UOdinSocket::GetSocketNamedLabel()
{ return FName(*FString::FromInt(this->GetSocketLabel())); }

FOdinSocketInfo UOdinSocket::GetSocketInfo()
{
    OdinSocketInfo info = {};
    auto           ret  = odin_socket_info(GetNativeHandle(), &info);
    if (ret != OdinError::ODIN_ERROR_SUCCESS) {
        FOdinModule::LogErrorCode("Aborting GetSocketInfo due to invalid odin_socket_info call: %s", ret);
        return this->Info; // keep the last known info instead of overwriting it with zeroed data
    }

    this->Info = FOdinSocketInfo(info);
    return this->Info;
}

bool UOdinSocket::Send(TArray<uint8> message)
{
    auto ret = odin_socket_send(this->GetNativeHandle(), message.GetData(), message.Num());
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        ODIN_LOG(VeryVerbose, "Send: %d bytes", message.Num());
        return true;
    } else {
        FOdinModule::LogErrorCode("Aborting Send due to invalid odin_socket_send call: %s", ret);
    }
    return false;
}

bool UOdinSocket::SendString(const FString &Message)
{
    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(FTCHARToUTF8_Convert::ConvertedLength(*Message, Message.Len()));
    FTCHARToUTF8_Convert::Convert((UTF8CHAR *)Buffer.GetData(), Buffer.Num(), *Message, Message.Len());
    return Send(Buffer);
}

bool UOdinSocket::IsReady() const
{
    OdinSocketInfo info;
    auto           ret = odin_socket_info(GetNativeHandle(), &info);
    return ret == OdinError::ODIN_ERROR_SUCCESS && info.unsent_bytes == 0 && info.room != nullptr;
}
