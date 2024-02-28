/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "Templates/SharedPointer.h"

#include "OdinCaptureMedia.h"
#include "OdinJsonObject.h"

#include "OdinPlaybackMedia.h"
#include "OdinSubmixListener.h"

#include <memory>

#include "OdinRoom.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJoinRoomResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAddMediaResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPauseMediaResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResumeMediaResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRemoveMediaResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdatePositionResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdatePeerUserDataResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSendMessageResponsePin, bool, success);

UENUM(BlueprintType)
enum EOdinRoomConnectionState {
    Disconnected,
    Disconnecting,
    Connecting,
    Connected,
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomJoinError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE_FiveParams(FOdinRoomJoinSuccess, FString, roomId, const TArray<uint8> &,
                                    roomUserData, FString, customer, int64, ownPeerId, FString,
                                    ownUserId);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomJoin : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Room",
                      DisplayName  = "Join Room",
                      ToolTip      = "Joins the room specified in a given authentication token",
                      WorldContext = "WorldContextObject",
                      AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinRoomJoin *JoinRoom(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room,
                                   const FString url, const FString token,
                                   const TArray<uint8> &initialPeerUserData,
                                   FVector initialPosition, const FOdinRoomJoinError &onError,
                                   const FOdinRoomJoinSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FJoinRoomResponsePin OnResponse;

    UPROPERTY()
    UOdinRoom *Room;

    FString              Url;
    FString              Token;
    TArray<uint8>        InitialPeerUserData;
    FVector              InitialPosition;
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
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Sound",
                      DisplayName  = "Add Media to Room",
                      ToolTip      = "Adds a capture media handle to the room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess,onError"))
    static UOdinRoomAddMedia *AddMedia(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room,
                                       UPARAM(ref) UOdinCaptureMedia *&media,
                                       const FOdinRoomAddMediaError   &onError,
                                       const FOdinRoomAddMediaSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FAddMediaResponsePin OnResponse;

    UPROPERTY()
    UOdinRoom *Room;

    UPROPERTY()
    UOdinCaptureMedia *CaptureMedia;

    FOdinRoomAddMediaError   OnError;
    FOdinRoomAddMediaSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomPauseMediaError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomPauseMediaSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomPauseMedia : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(
        BlueprintCallable,
        meta =
            (BlueprintInternalUseOnly = "true", Category = "Odin|Sound",
             DisplayName = "Pause Playback Media",
             ToolTip = "Pause the specified playback media handle, ceasing the reception of data",
             WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess,onError"))
    static UOdinRoomPauseMedia *PauseMedia(UObject                          *WorldContextObject,
                                           UPARAM(ref) UOdinPlaybackMedia  *&media,
                                           const FOdinRoomPauseMediaError   &onError,
                                           const FOdinRoomPauseMediaSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FPauseMediaResponsePin OnResponse;

    UPROPERTY()
    UOdinPlaybackMedia *PlaybackMedia;

    FOdinRoomPauseMediaError   OnError;
    FOdinRoomPauseMediaSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomResumeMediaError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomResumeMediaSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomResumeMedia : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(
        BlueprintCallable,
        meta =
            (BlueprintInternalUseOnly = "true", Category = "Odin|Sound",
             DisplayName = "Resume Playback Media",
             ToolTip =
                 "Resume the specified playback media handle, re-initiating the reception of data",
             WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess,onError"))
    static UOdinRoomResumeMedia *ResumeMedia(UObject                           *WorldContextObject,
                                             UPARAM(ref) UOdinPlaybackMedia   *&media,
                                             const FOdinRoomResumeMediaError   &onError,
                                             const FOdinRoomResumeMediaSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FResumeMediaResponsePin OnResponse;

    UPROPERTY()
    UOdinPlaybackMedia *PlaybackMedia;

    FOdinRoomResumeMediaError   OnError;
    FOdinRoomResumeMediaSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomRemoveMediaError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomRemoveMediaSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomRemoveMedia : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Sound",
                      DisplayName  = "Remove Media from Room",
                      ToolTip      = "Removes a capture media handle from the room and destroys it",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess,onError"))
    static UOdinRoomRemoveMedia *RemoveMedia(UObject                *WorldContextObject,
                                             UPARAM(ref) UOdinRoom *&room, UOdinCaptureMedia *media,
                                             const FOdinRoomRemoveMediaError   &onError,
                                             const FOdinRoomRemoveMediaSuccess &onSuccess);

    virtual void Activate() override;

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
                      DisplayName = "Update Peer Position",
                      ToolTip = "Updates the two-dimensional position of the own peer in the room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess,onError"))
    static UOdinRoomUpdatePosition *UpdatePosition(UObject                *WorldContextObject,
                                                   UPARAM(ref) UOdinRoom *&room, FVector position,
                                                   const FOdinRoomUpdatePositionError   &onError,
                                                   const FOdinRoomUpdatePositionSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FUpdatePositionResponsePin OnResponse;

    UPROPERTY()
    UOdinRoom *Room;

    FVector Position;

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
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Custom Data",
                      DisplayName  = "Update Peer User Data",
                      ToolTip      = "Updates the custom user data of the own peer in the room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess,onError"))
    static UOdinRoomUpdatePeerUserData *
    UpdatePeerUserData(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room,
                       const TArray<uint8> &data, const FOdinRoomUpdatePeerUserDataError &onError,
                       const FOdinRoomUpdatePeerUserDataSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable)
    FUpdatePeerUserDataResponsePin OnResponse;

    UPROPERTY()
    UOdinRoom *Room;

    TArray<uint8> Data;

    FOdinRoomUpdatePeerUserDataError   OnError;
    FOdinRoomUpdatePeerUserDataSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinRoomSendMessageError, int64, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinRoomSendMessageSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinRoomSendMessage : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Custom Data",
                      DisplayName  = "Send Message",
                      ToolTip      = "Sends arbitrary data to a list of target peers in the room",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "onSuccess,onError"))
    static UOdinRoomSendMessage *SendMessage(UObject                           *WorldContextObject,
                                             UPARAM(ref) UOdinRoom            *&room,
                                             const TArray<int64>               &targets,
                                             const TArray<uint8>               &data,
                                             const FOdinRoomSendMessageError   &onError,
                                             const FOdinRoomSendMessageSuccess &onSuccess);

    virtual void Activate() override;

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
    OdinNS_None UMETA(DisplayName = "Disabled"),
    /**
     * Use low suppression (6 dB)
     */
    OdinNS_Low UMETA(DisplayName = "Low"),
    /**
     * Use moderate suppression (12 dB)
     */
    OdinNS_Moderate UMETA(DisplayName = "Moderate"),
    /**
     * Use high suppression (18 dB)
     */
    OdinNS_High UMETA(DisplayName = "High"),
    /**
     * Use very high suppression (21 dB)
     */
    OdinNS_VeryHigh UMETA(DisplayName = "VeryHigh"),
};

USTRUCT(BlueprintType)
struct ODIN_API FOdinConnectionStats {
    GENERATED_BODY()

    /**
     * The amount of outgoing UDP datagrams observed
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats",
              meta = (DisplayName = "Outgoing UDP datagrams"))
    int64 udp_tx_datagrams = 0;
    /**
     * The amount of outgoing acknowledgement frames observed
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Outgoing ACK frames"))
    int64 udp_tx_acks = 0;
    /**
     * The total amount of bytes which have been transferred inside outgoing UDP datagrams
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Outgoing bytes"))
    int64 udp_tx_bytes = 0;
    /**
     * The amount of incoming UDP datagrams observed
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats",
              meta = (DisplayName = "Incoming UDP datagrams"))
    int64 udp_rx_datagrams = 0;
    /**
     * The amount of incoming acknowledgement frames observed
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Incoming ACK frames"))
    int64 udp_rx_acks = 0;
    /**
     * The total amount of bytes which have been transferred inside incoming UDP datagrams
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats", meta = (DisplayName = "Incoming bytes"))
    int64 udp_rx_bytes = 0;
    /**
     * Current congestion window of the connection
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats",
              meta = (DisplayName = "Congestion window of connection"))
    int64 cwnd = 0;
    /**
     * Congestion events on the connection
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats",
              meta = (DisplayName = "Congestion events of connection"))
    int64 congestion_events = 0;
    /**
     * Current best estimate of the connection latency (round-trip-time) in milliseconds
     */
    UPROPERTY(BlueprintReadOnly, Category = "Stats",
              meta = (DisplayName = "Estimated round-trip-time"))
    float rtt = 0;
};

USTRUCT(BlueprintType)
struct ODIN_API FOdinApmSettings {
    GENERATED_BODY()

    /**
     * Enables or disables voice activity detection
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VAD,
              meta = (DisplayName = "Enable Voice Activity Detection"))
    bool bVoiceActivityDetection = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VAD,
              meta = (DisplayName = "Attack Probability", ClampMin = "0.0", ClampMax = "1.0",
                      UIMin = "0.0", UIMax = "1.0"))
    float fVadAttackProbability = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VAD,
              meta = (DisplayName = "Release Probability", ClampMin = "0.0", ClampMax = "1.0",
                      UIMin = "0.0", UIMax = "1.0"))
    float fVadReleaseProbability = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Gate",
              meta = (DisplayName = "Enable Volume Gate"))
    bool bEnableVolumeGate = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Gate",
              meta = (DisplayName = "Attack Loudness (dBFS)", ClampMin = "-90.0", ClampMax = "0.0",
                      UIMin = "-90.0", UIMax = "0.0"))
    float fVolumeGateAttackLoudness = -90.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Gate",
              meta = (DisplayName = "Release Loudness (dBFS)", ClampMin = "-90.0", ClampMax = "0.0",
                      UIMin = "-90.0", UIMax = "0.0"))
    float fVolumeGateReleaseLoudness = -90.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Echo Canceller"),
              Category = "Filters")
    bool bEchoCanceller = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "High Pass Filter"),
              Category = "Filters")
    bool bHighPassFilter = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Pre Amplifier"),
              Category = "Filters")
    bool bPreAmplifier = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Noise Suppression"),
              Category = "Filters")
    TEnumAsByte<EOdinNoiseSuppressionLevel> noise_suppression_level =
        EOdinNoiseSuppressionLevel::OdinNS_Moderate;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Transient Suppression"),
              Category = "Filters")
    bool bTransientSuppresor = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Gain Controller"),
              Category = "Filters")
    bool bGainController = true;
};

UCLASS(ClassGroup     = Odin, BlueprintType,
       hidecategories = (Activation, Transform, Object, ActorComponent, Physics, Rendering,
                         Mobility, LOD))
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

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOdinMediaActiveStateChanged, int64, peerId,
                                                  UOdinMediaBase *const, media, bool, active,
                                                  UOdinRoom *, room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinMediaActiveStateChanged onMediaActiveStateChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinMessageReceived, int64, peerId,
                                                   const TArray<uint8> &, data, UOdinRoom *, room);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Room|Events")
    FOdinMessageReceived onMessageReceived;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOdinPeerJoined, int64, peerId, FString, userId,
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
    ~UOdinRoom();

    UFUNCTION(BlueprintCallable, BlueprintPure,
              meta     = (DisplayName = "Construct Local Room Handle",
                      ToolTip     = "Creates a new local room handle in an unconnected state",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                      AutoCreateRefTerm = "InitialAPMSettings"),
              Category = "Odin|Room")
    static UOdinRoom *ConstructRoom(UObject                *WorldContextObject,
                                    const FOdinApmSettings &InitialAPMSettings);

    UFUNCTION(
        BlueprintCallable,
        meta =
            (DisplayName = "Set Room Position Scale",
             ToolTip = "Sets the multiplicative scale for all coordinates used in position updates",
             HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
             Category = "Odin|Room"))
    void SetPositionScale(float Scale);

    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Get Room Connection Stats",
                      ToolTip     = "Get statistics for a room connection",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                      Category = "Odin|Debug"))
    FOdinConnectionStats ConnectionStats();

    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Set Room APM Config",
                      ToolTip     = "Updates audio processing settings for capture media handles",
                      HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                      Category = "Odin|Room"))
    void UpdateAPMConfig(FOdinApmSettings apm_config);

    UFUNCTION(
        BlueprintCallable,
        meta = (DisplayName = "Set Room APM Stream Delay",
                ToolTip =
                    "Updates the delay estimate in ms for reverse stream used in echo cancellation",
                HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                Category = "Odin|Room"))
    void UpdateAPMStreamDelay(int64 DelayInMs);

    UFUNCTION(
        BlueprintCallable,
        meta = (DisplayName = "Destroy Local Room Handle",
                ToolTip = "Closes the connection to the server and destroys the local room handle",
                HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject",
                Category = "Odin|Room"))
    void Destroy();

    void BindCaptureMedia(UOdinCaptureMedia *media);
    void UnbindCaptureMedia(UOdinCaptureMedia *media);

    OdinRoomHandle RoomHandle() const
    {
        return this->room_handle_;
    }

    UFUNCTION(BlueprintCallable,
              meta = (DisplayName = "Get Current Room APM Config", Category = "Odin|Room"))
    FOdinApmSettings GetCurrentApmSettings() const
    {
        return this->current_apm_settings_;
    }

  protected:
    void BeginDestroy() override;
    void FinishDestroy() override;

  private:
    OdinRoomHandle room_handle_;

    UPROPERTY(BlueprintGetter = GetCurrentApmSettings, Category = "Odin|Room",
              meta = (DisplayName = "Current Room APM Config"))
    FOdinApmSettings current_apm_settings_;

    FCriticalSection capture_medias_cs_;
    UPROPERTY(transient)
    TArray<UOdinCaptureMedia *> capture_medias_;

    FCriticalSection medias_cs_;
    UPROPERTY(transient)
    TMap<uint64, UOdinMediaBase *> medias_;

    FCriticalSection joined_callbacks_cs_;

    TArray<TFunction<void(FString roomId, FString roomCustomer, TArray<uint8> roomUserData,
                          int64 ownPeerId, FString ownUserId)>>
        joined_callbacks_;

    void HandleOdinEvent(const OdinEvent event);

    UPROPERTY(transient)
    UOdinSubmixListener *submix_listener_;

    friend class UOdinRoomJoin;
    friend class UOdinRoomAddMedia;
    friend class UOdinRoomUpdatePosition;
    friend class UOdinRoomUpdatePeerUserData;
    friend class UOdinRoomSendMessage;
    friend class UOdinRoomJoinTask;
};
