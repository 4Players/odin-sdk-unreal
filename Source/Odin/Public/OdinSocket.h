/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#pragma once

#include "OdinCore/include/odin.h"

#include "CoreMinimal.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinNative/OdinNativeHandle.h"

#include "OdinSocket.generated.h"

class UOdinRoom;

/**
 * An opaque type representing an ODIN socket
 *
 * @remarks Sockets are bound to the current room session: they do not survive a reconnect, and sockets to a specific peer are closed automatically when
 * that peer leaves the room.
 */
UCLASS(ClassGroup = Odin, Blueprintable, BlueprintType)
class ODIN_API UOdinSocket : public UObject
{
    GENERATED_BODY()

  public:
    UOdinSocket(const class FObjectInitializer& PCIP);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinSocketReceivedDelegate, UOdinRoom*, room, UOdinSocket*, socket, const TArray<uint8>&, data);

    UPROPERTY(BlueprintAssignable, Category = "Odin|Socket|Events")
    FOdinSocketReceivedDelegate OnSocketMessageReceivedBP;

    /**
     * Creates a new ODIN socket handle
     */
    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Construct Socket", ToolTip = "Creates a new socket", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                      Keywords = "Create,Create Socket"),
              Category = "Odin")
    static UOdinSocket* ConstructSocket(UObject* WorldContextObject);

    static UOdinSocket* ConstructSocket(UObject* WorldContextObject, OdinSocket* handle);

    /**
     * Closes the specified ODIN socket handle, therefore local socket can not send
     */
    UFUNCTION(BlueprintCallable, Category = "Odin", meta = (Keywords = "Disconnect,Reset Connection,Destroy Socket"))
    bool ResetSocket();

    /**
     * Resets the specified ODIN socket handle
     */
    static bool ResetOdinSocketByHandle(OdinSocket* socket);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Create Socket", ToolTip = "Creates the socket for a room on the server",
                          Keywords = "Start Connection,Start Socket,Socket,Room Socket"),
              Category = "Odin")
    UOdinSocket* Create(UOdinRoom* Room, EOdinSocketKind SocketKind, int64 TargetPeerId, int32 Label, int32 Priority);

    /**
     * Get last retrieved reconnect token.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Remote Peer Id", ToolTip = "Gets the target peer id of the socket"),
              Category = "Odin|Socket|Info")
    int64 GetRemotePeerId();
    /**
     * Retrieves the socket label
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Label", ToolTip = "Gets the socket creation label"), Category = "Odin|Socket|Info")
    int32 GetSocketLabel();
    /**
     * Retrieves the socket label
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Named Label", ToolTip = "Gets the socket creation label that was named"),
              Category = "Odin|Socket|Info")
    FName GetSocketNamedLabel();
    /**
     * Retrieves the connection stats.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Socket Info", ToolTip = "Gets additional socket info"),
              Category = "Odin|Socket|Info")
    FOdinSocketInfo GetSocketInfo();
    /**
     * Sends an arbitrary message to the server.
     * @param Message   byte array
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Send", ToolTip = "Send arbitrary message"), Category = "Odin|Socket")
    bool Send(TArray<uint8> Message);
    /**
     * Sends an utf8 string message to the server.
     * @param Message   String Message
     * @return true on ODIN_ERROR_SUCCESS or false
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Send String", ToolTip = "Send UTF8 string message"), Category = "Odin|Socket")
    bool SendString(const FString& Message);

    /**
     * Checks socket and socket info. (extra socket info call)
     * @return true if this appears to be a valid object and unsent bytes are 0
     */
    UFUNCTION(BlueprintPure, Category = "Odin|Socket")
    bool IsReady() const;

    inline OdinSocket* GetNativeHandle() const
    { return IsValid(Handle) && Handle->IsValidLowLevel() ? static_cast<OdinSocket*>(Handle->GetHandle()) : nullptr; }

    TWeakObjectPtr<UOdinHandle> GetHandle() const
    { return Handle; }

    /**
     * Drops the reference to the native socket without touching it (e.g. after the owning room was freed).
     */
    inline void InvalidateHandle()
    {
        if (IsValid(this->Handle)) {
            this->Handle->Invalidate();
        }
    }

    inline void SetHandle(OdinSocket* handle)
    {
        if (handle == nullptr && IsValid(this->Handle)) {
            this->Handle->SetHandle(nullptr);
            return;
        }

        this->Handle = NewObject<UOdinHandle>();
        this->Handle->SetHandle(handle);
    }

  protected:
    virtual void BeginDestroy() override;
    virtual void FinishDestroy() override;

    UPROPERTY(BlueprintReadOnly, Category = "Odin|Socket")
    FOdinSocketInfo Info;

  private:
    UPROPERTY()
    UOdinHandle*     Handle;
    FCriticalSection Socket_CS;
    static void      HandleOdinEventSocket(OdinSocket* SocketHandle, const TArray<uint8>& Message);
};