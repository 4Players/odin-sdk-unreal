/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRemoveMediaResponsePin, bool, success);
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
class ODIN_API UOdinRoomJoin : public UBlueprintAsyncActionBase
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
class ODIN_API UOdinRoomAddMedia : public UBlueprintAsyncActionBase
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

    UPROPERTY()
    UOdinRoom *Room;

    UPROPERTY()
    UOdinCaptureMedia *CaptureMedia;

    FOdinRoomAddMediaError   OnError;
    FOdinRoomAddMediaSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomRemoveMediaError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomRemoveMediaSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomRemoveMedia : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(
        BlueprintCallable,
        meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Room",
                WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess",
                DisplayName = "Remove the media from the room and destroy the internal media"))
    static UOdinRoomRemoveMedia *RemoveMedia(UObject *WorldContextObject, UOdinRoom *room,
                                             UOdinCaptureMedia                 *media,
                                             FOdinRoomRemoveMediaError          onError,
                                             const FOdinRoomRemoveMediaSuccess &onSuccess);

    void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FRemoveMediaResponsePin OnResponse;

    UPROPERTY()
    UOdinRoom *Room;

    UPROPERTY()
    UOdinCaptureMedia *CaptureMedia;

    FOdinRoomRemoveMediaError   OnError;
    FOdinRoomRemoveMediaSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomUpdatePositionError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomUpdatePositionSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomUpdatePosition : public UBlueprintAsyncActionBase
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

    UPROPERTY()
    UOdinRoom *Room;

    FVector2D Position;

    FOdinRoomUpdatePositionError   OnError;
    FOdinRoomUpdatePositionSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomUpdatePeerUserDataError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomUpdatePeerUserDataSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomUpdatePeerUserData : public UBlueprintAsyncActionBase
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

    UPROPERTY()
    UOdinRoom *Room;

    TArray<uint8> Data;

    FOdinRoomUpdatePeerUserDataError   OnError;
    FOdinRoomUpdatePeerUserDataSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomUpdateRoomUserDataError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomUpdateRoomUserDataSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomUpdateRoomUserData : public UBlueprintAsyncActionBase
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

    UPROPERTY()
    UOdinRoom *Room;

    TArray<uint8> Data;

    FOdinRoomUpdateRoomUserDataError   OnError;
    FOdinRoomUpdateRoomUserDataSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomSendMessageError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomSendMessageSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomSendMessage : public UBlueprintAsyncActionBase
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

    UPROPERTY()
    UOdinRoom *Room;

    TArray<uint8> Data;
    TArray<int64> Targets;

    FOdinRoomSendMessageError   OnError;
    FOdinRoomSendMessageSuccess OnSuccess;
};

UENUM(BlueprintType)
enum EOdinNoiseSuppressionLevel {
    None UMETA(DisplayName = "Disabled"),
    /**
     * Use low suppression (6 dB)
     */
    Low UMETA(DisplayName = "Low"),
    /**
     * Use moderate suppression (12 dB)
     */
    Moderate UMETA(DisplayName = "Moderate"),
    /**
     * Use high suppression (18 dB)
     */
    High UMETA(DisplayName = "High"),
    /**
     * Use very high suppression (21 dB)
     */
    VeryHigh UMETA(DisplayName = "VeryHigh"),
};

USTRUCT(BlueprintType)
struct ODIN_API FOdinApmSettings {
    GENERATED_USTRUCT_BODY()

    /**
     * Enables or disables voice activity detection
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VAD,
              meta = (DisplayName = "Enable Voice Activity Detection"))
    bool bVoiceActivityDetection = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VAD,
              meta = (DisplayName = "Attack Probability", EditCondition = "bVoiceActivityDetection",
                      ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float fVadAttackProbability = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VAD,
              meta = (DisplayName   = "Release Probability",
                      EditCondition = "bVoiceActivityDetection", ClampMin = "0.0", ClampMax = "1.0",
                      UIMin = "0.0", UIMax = "1.0"))
    float fVadReleaseProbability = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Gate",
              meta = (DisplayName = "Enable Volume Gate", DefaultValue = "false"))
    bool bEnableVolumeGate = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Gate",
              meta = (DisplayName = "Attack Loudness (dBFS)", EditCondition = "bEnableVolumeGate",
                      ClampMin = "-90.0", ClampMax = "0.0", UIMin = "-90.0", UIMax = "0.0"))
    float fVolumeGateAttackLoudness = -90.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Gate",
              meta = (DisplayName = "Release Loudness (dBFS)", EditCondition = "bEnableVolumeGate",
                      ClampMin = "-90.0", ClampMax = "0.0", UIMin = "-90.0", UIMax = "0.0"))
    float fVolumeGateReleaseLoudness = -90.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "High Pass Filter"))
    bool bHighPassFilter = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Pre Amplifier"))
    bool bPreAmplifier = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Noise Suppresion"))
    TEnumAsByte<EOdinNoiseSuppressionLevel> noise_suppression_level =
        EOdinNoiseSuppressionLevel::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Transient Suppressor"))
    bool bTransientSuppresor = false;
};

UCLASS(ClassGroup     = Odin, BlueprintType,
       hidecategories = (Activation, Transform, Object, ActorComponent, Physics, Rendering,
                         Mobility, LOD),
       meta           = (BlueprintSpawnableComponent))
class ODIN_API UOdinRoom : public /* USceneComponent */ UObject
{
    GENERATED_BODY()

  public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinRoomJoined, int64, peerId,
                                                   const TArray<uint8> &, roomUserData, UOdinRoom *,
                                                   room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinRoomJoined onRoomJoined;
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinRoomUserDataChanged, const TArray<uint8> &,
                                                 userData, UOdinRoom *, room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinRoomUserDataChanged onRoomUserDataChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOdinMediaAdded, int64, peerId,
                                                  UOdinPlaybackMedia *, media, UOdinJsonObject *,
                                                  properties, UOdinRoom *, room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinMediaAdded onMediaAdded;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinMediaRemoved, int64, peerId,
                                                   UOdinPlaybackMedia *, media, UOdinRoom *, room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinMediaRemoved onMediaRemoved;

    // TODO(alexander): Do we want to move this to the media object instead?
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOdinMediaActiveStateChanged, int64, peerId,
                                                  UOdinMediaBase *const, media, bool, active,
                                                  UOdinRoom *, room);
    FOdinMediaActiveStateChanged onMediaActiveStateChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinMessageReceived, int64, peerId,
                                                   const TArray<uint8> &, data, UOdinRoom *, room);
    FOdinMessageReceived onMessageReceived;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinPeerJoined, int64, peerId,
                                                   const TArray<uint8> &, userData, UOdinRoom *,
                                                   room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerJoined onPeerJoined;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinPeerLeft, int64, peerId, UOdinRoom *, room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerLeft onPeerLeft;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinPeerUserDataChanged, int64, peerId,
                                                   const TArray<uint8> &, userData, UOdinRoom *,
                                                   room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinPeerUserDataChanged onPeerUserDataChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinRoomConnectionStatChanged,
                                                 EOdinRoomConnectionState, connectionState,
                                                 UOdinRoom *, room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinRoomConnectionStatChanged onConnectionStateChanged;

  public:
    UOdinRoom(const FObjectInitializer &ObjectInitializer);

    UFUNCTION(BlueprintCallable, BlueprintPure,
              meta     = (DisplayName = "Construct a Room", HidePin = "WorldContextObject",
                      DefaultToSelf     = "WorldContextObject",
                      AutoCreateRefTerm = "InitialAPMSettings"),
              Category = "Odin|Room")
    static UOdinRoom *ConstructRoom(UObject                *WorldContextObject,
                                    const FOdinApmSettings &InitialAPMSettings);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Updates Position Scale", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Room|Functions")
    void SetPositionScale(float Scale);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Updates APM Settings for Capture Medias",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Room|Functions")
    void UpdateAPMConfig(const FOdinApmSettings &apm_config);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Destroy the internal room and disconnect",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Room|Functions")
    void Destroy();

    void BindCaptureMedia(UOdinCaptureMedia *media);
    void UnbindCaptureMedia(UOdinCaptureMedia *media);

    OdinRoomHandle RoomHandle() const
    {
        return this->room_handle_;
    }

    // UFUNCTION(BlueprintCallable,
    //          meta     = (DisplayName = "Remove Media from Room",
    //                  HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
    //          Category = "Odin|Room|Functions")
    // void RemoveMedia(int32 mediaId);

    UFUNCTION(BlueprintCallable, Category = Sound, meta = (DisplayName = "Current APM Settings"))
    FOdinApmSettings GetCurrentApmSettings() const
    {
        return this->current_apm_settings_;
    }

  protected:
    void BeginDestroy() override;

  private:
    OdinRoomHandle room_handle_;

    UPROPERTY(BlueprintGetter = GetCurrentApmSettings, Category = Sound,
              meta = (DisplayName = "Current APM Settings"))
    FOdinApmSettings current_apm_settings_;

    FCriticalSection capture_medias_cs_;
    UPROPERTY(transient)
    TArray<UOdinCaptureMedia *> capture_medias_;

    FCriticalSection medias_cs_;
    UPROPERTY(transient)
    TMap<uint64, UOdinMediaBase *> medias_;

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
