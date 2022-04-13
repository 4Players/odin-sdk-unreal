#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "Templates/SharedPointer.h"

#include "OdinCaptureMedia.h"
#include "OdinJsonObject.h"

#include "OdinPlaybackMedia.h"

#include <memory>

#include "OdinRoom.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJoinRoomResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAddMediaResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdatePositionResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdatePeerUserDataResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdateRoomUserDataResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSendMessageResponsePin, bool, success);

UENUM(BlueprintType)
enum EOdinRoomConnectionState {
    Disconnected,
    Connecting,
    Connected,
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomJoinError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE_FourParams(FOdinRoomJoinSuccess, FString, roomId, const TArray<uint8> &,
                                    roomUserData, FString, customer, int64, ownPeerId);
UCLASS(ClassGroup = Odin)
class UOdinRoomJoin : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Room",
                                         WorldContext      = "WorldContextObject",
                                         AutoCreateRefTerm = "userData,url,onSuccess"))
    static UOdinRoomJoin *JoinRoom(UObject *WorldContextObject, UOdinRoom *room, const FString url,
                                   const FString token, const TArray<uint8> &userData,
                                   FOdinRoomJoinError          onError,
                                   const FOdinRoomJoinSuccess &onSuccess);

    void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FJoinRoomResponsePin OnResponse;

    UPROPERTY()
    UOdinRoom *Room;

    FString              Url;
    FString              Token;
    TArray<uint8>        UserData;
    FOdinRoomJoinError   OnError;
    FOdinRoomJoinSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomAddMediaError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomAddMediaSuccess, int32, mediaId);
UCLASS(ClassGroup = Odin)
class UOdinRoomAddMedia : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess"))
    static UOdinRoomAddMedia *AddMedia(UObject *WorldContextObject, UOdinRoom *room,
                                       UOdinCaptureMedia *media, FOdinRoomAddMediaError onError,
                                       const FOdinRoomAddMediaSuccess &onSuccess);

    void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FAddMediaResponsePin OnResponse;

    UPROPERTY();
    UOdinRoom *Room;

    UPROPERTY();
    UOdinCaptureMedia *CaptureMedia;

    FOdinRoomAddMediaError   OnError;
    FOdinRoomAddMediaSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomUpdatePositionError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomUpdatePositionSuccess);
UCLASS(ClassGroup = Odin)
class UOdinRoomUpdatePosition : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess"))
    static UOdinRoomUpdatePosition *UpdatePosition(UObject *WorldContextObject, UOdinRoom *room,
                                                   FVector2D                             position,
                                                   FOdinRoomUpdatePositionError          onError,
                                                   const FOdinRoomUpdatePositionSuccess &onSuccess);

    void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FUpdatePositionResponsePin OnResponse;

    UPROPERTY();
    UOdinRoom *Room;

    FVector2D Position;

    FOdinRoomUpdatePositionError   OnError;
    FOdinRoomUpdatePositionSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomUpdatePeerUserDataError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomUpdatePeerUserDataSuccess);
UCLASS(ClassGroup = Odin)
class UOdinRoomUpdatePeerUserData : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess"))
    static UOdinRoomUpdatePeerUserData *
    UpdatePeerUserData(UObject *WorldContextObject, UOdinRoom *room, const TArray<uint8> &data,
                       FOdinRoomUpdatePeerUserDataError          onError,
                       const FOdinRoomUpdatePeerUserDataSuccess &onSuccess);

    void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FUpdatePeerUserDataResponsePin OnResponse;

    UPROPERTY();
    UOdinRoom *Room;

    TArray<uint8> Data;

    FOdinRoomUpdatePeerUserDataError   OnError;
    FOdinRoomUpdatePeerUserDataSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomUpdateRoomUserDataError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomUpdateRoomUserDataSuccess);
UCLASS(ClassGroup = Odin)
class UOdinRoomUpdateRoomUserData : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess"))
    static UOdinRoomUpdateRoomUserData *
    UpdateRoomUserData(UObject *WorldContextObject, UOdinRoom *room, const TArray<uint8> &data,
                       FOdinRoomUpdateRoomUserDataError          onError,
                       const FOdinRoomUpdateRoomUserDataSuccess &onSuccess);

    void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FUpdateRoomUserDataResponsePin OnResponse;

    UPROPERTY();
    UOdinRoom *Room;

    TArray<uint8> Data;

    FOdinRoomUpdateRoomUserDataError   OnError;
    FOdinRoomUpdateRoomUserDataSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomSendMessageError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomSendMessageSuccess);
UCLASS(ClassGroup = Odin)
class UOdinRoomSendMessage : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess"))
    static UOdinRoomSendMessage *SendMessage(UObject *WorldContextObject, UOdinRoom *room,
                                             const TArray<int64>               &targets,
                                             const TArray<uint8>               &data,
                                             FOdinRoomSendMessageError          onError,
                                             const FOdinRoomSendMessageSuccess &onSuccess);

    void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FSendMessageResponsePin OnResponse;

    UPROPERTY();
    UOdinRoom *Room;

    TArray<uint8> Data;
    TArray<int64> Targets;

    FOdinRoomSendMessageError   OnError;
    FOdinRoomSendMessageSuccess OnSuccess;
};

UCLASS(ClassGroup = Odin, meta = (BlueprintSpawnableComponent))
class UOdinRoom : public UObject
{
    GENERATED_UCLASS_BODY()
  public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinRoomJoined, int64, peerId,
                                                 const TArray<uint8> &, roomUserData);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinRoomJoined onRoomJoined;
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinRoomUserDataChanged, const TArray<uint8> &,
                                                userData);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinRoomUserDataChanged onRoomUserDataChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinMediaAdded, int64, peerId,
                                                   UOdinPlaybackMedia *, media, UOdinJsonObject *,
                                                   properties);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinMediaAdded onMediaAdded;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinMediaRemoved, int64, peerId, int32, mediaId);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinMediaRemoved onMediaRemoved;

    // TODO(alexander): Do we want to move this to the media object instead?
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinMediaActiveStateChanged, int64, peerId,
                                                   int32, mediaId, bool, active);
    FOdinMediaActiveStateChanged onMediaActiveStateChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinMessageReceived, int64, peerId,
                                                 const TArray<uint8> &, data);
    FOdinMessageReceived onMessageReceived;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinPeerJoined, int64, peerId,
                                                 const TArray<uint8> &, userData);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerJoined onPeerJoined;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinPeerLeft, int64, peerId);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerLeft onPeerLeft;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinPeerUserDataChanged, int64, peerId,
                                                 const TArray<uint8> &, userData);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerUserDataChanged onPeerUserDataChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinRoomConnectionStatChanged,
                                                EOdinRoomConnectionState, connectionState);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinRoomConnectionStatChanged onConnectionStateChanged;

  public:
    UFUNCTION(BlueprintCallable, BlueprintPure,
              meta     = (DisplayName = "Construct a Room", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Room")
    static UOdinRoom *ConstructRoom(UObject *WorldContextObject);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Updates Position Scale", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Room|Functions")
    void SetPositionScale(float Scale);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Destroy the internal room and disconnect",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Room|Functions")
    void Destroy();

    // UFUNCTION(BlueprintCallable,
    //          meta     = (DisplayName = "Remove Media from Room",
    //                  HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
    //          Category = "Odin|Room|Functions")
    // void RemoveMedia(int32 mediaId);

  protected:
    void BeginDestroy() override;

  private:
    OdinRoomHandle room_handle_;

    TArray<UOdinCaptureMedia *> capture_medias_;

    FCriticalSection joined_callbacks_cs_;
    TArray<TFunction<void(FString roomId, FString roomCustomer, TArray<uint8> roomUserData,
                          int64 ownPeerId)>>
        joined_callbacks_;

    void HandleOdinEvent(const struct OdinEvent *event);

    friend class UOdinRoomJoin;
    friend class UOdinRoomAddMedia;
    friend class UOdinRoomUpdatePosition;
    friend class UOdinRoomUpdatePeerUserData;
    friend class UOdinRoomUpdateRoomUserData;
    friend class UOdinRoomSendMessage;
    friend class UOdinRoomJoinTask;
};
