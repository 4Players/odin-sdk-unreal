/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once
#include "OdinCore/include/odin.h"
#include "OdinNative/OdinUtils.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "OdinAudio/Effects/OdinCustomEffect.h"
#include "Templates/SharedPointer.h"

#include "OdinNativeBlueprint.generated.h"

class UOdinRoom;
class UOdinEncoder;
class UOdinDecoder;
class UOdinPipeline;
class UOdinTokenGenerator;
//** Wrapping workaround for enum blueprints that only supports uint8 values */
UENUM(BlueprintType)
enum class EOdinError : uint8 {
    /**
     * No data available
     */
    ODIN_ERROR_NO_DATA = 1 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "OK_NO_DATA", ToolTip = "No data available"),
    /**
     * Operation completed successfully
     */
    ODIN_ERROR_SUCCESS = 0 UMETA(DisplayName = "ERROR_SUCCESS", ToolTip = "Operation completed successfully"),
    /**
     * The runtime initialization failed
     */
    ODIN_ERROR_INITIALIZATION_FAILED =
        -1 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_INITIALIZATION_FAILED", ToolTip = "The runtime initialization failed"),
    /**
     * The specified API version is not supported
     */
    ODIN_ERROR_UNSUPPORTED_VERSION =
        -2 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_UNSUPPORTED_VERSION", ToolTip = "The specified API version is not supported"),
    /**
     * The object is in an unexpected state
     */
    ODIN_ERROR_UNEXPECTED_STATE =
        -3 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_UNEXPECTED_STATE", ToolTip = "The object is in an unexpected state"),
    /**
     * The object is closed
     */
    ODIN_ERROR_CLOSED = -4 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_CLOSED", ToolTip = "The object is closed"),
    /**
     * A mandatory argument is null
     */
    ODIN_ERROR_ARGUMENT_NULL = -11 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_ARGUMENT_NULL", ToolTip = "A mandatory argument is null"),
    /**
     * A provided argument is too small
     */
    ODIN_ERROR_ARGUMENT_TOO_SMALL =
        -12 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_ARGUMENT_TOO_SMALL", ToolTip = "A provided argument is too small"),
    /**
     * A provided argument is out of the expected bounds
     */
    ODIN_ERROR_ARGUMENT_OUT_OF_BOUNDS =
        -13
        + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_ARGUMENT_OUT_OF_BOUNDS", ToolTip = "A provided argument is out of the expected bounds"),
    /**
     * A provided string argument is not valid UTF-8
     */
    ODIN_ERROR_ARGUMENT_INVALID_STRING =
        -14 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_ARGUMENT_INVALID_STRING", ToolTip = "A provided string argument is not valid UTF-8"),
    /**
     * A provided handle argument is invalid
     */
    ODIN_ERROR_ARGUMENT_INVALID_HANDLE =
        -15 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_ARGUMENT_INVALID_HANDLE", ToolTip = "A provided handle argument is invalid"),
    /**
     * A provided identifier argument is invalid
     */
    ODIN_ERROR_ARGUMENT_INVALID_ID =
        -16 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_ARGUMENT_INVALID_ID", ToolTip = "A provided identifier argument is invalid"),
    /**
     * A provided JSON argument is invalid
     */
    ODIN_ERROR_ARGUMENT_INVALID_JSON =
        -17 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_ARGUMENT_INVALID_JSON", ToolTip = "A provided JSON argument is invalid"),
    /**
     * The provided version is invalid
     */
    ODIN_ERROR_INVALID_VERSION =
        -21 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_INVALID_VERSION", ToolTip = "The provided version is invalid"),
    /**
     * The provided access key is invalid
     */
    ODIN_ERROR_INVALID_ACCESS_KEY =
        -22 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_INVALID_ACCESS_KEY", ToolTip = "The provided access key is invalid"),
    /**
     * The provided gateway/server address is invalid
     */
    ODIN_ERROR_INVALID_URI =
        -23 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_INVALID_URI", ToolTip = "The provided gateway/server address is invalid"),
    /**
     * The provided token is invalid
     */
    ODIN_ERROR_INVALID_TOKEN = -24 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_INVALID_TOKEN", ToolTip = "The provided token is invalid"),
    /**
     * The provided effect is not compatible with the expected effect type
     */
    ODIN_ERROR_INVALID_EFFECT = -25
                                + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_INVALID_EFFECT",
                                                                        ToolTip     = "The provided effect is not compatible with the expected effect type"),
    /**
     * The provided MessagePack encoded bytes are invalid
     */
    ODIN_ERROR_INVALID_MSG_PACK =
        -26 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_INVALID_MSG_PACK", ToolTip = "The provided MessagePack encoded bytes are invalid"),
    /**
     * The provided JSON string is invalid
     */
    ODIN_ERROR_INVALID_JSON = -27 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_INVALID_JSON", ToolTip = "The provided JSON string is invalid"),
    /**
     * The provided token does not grant access to the requested room
     */
    ODIN_ERROR_TOKEN_ROOM_REJECTED = -31
                                     + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_TOKEN_ROOM_REJECTED",
                                                                             ToolTip     = "The provided token does not grant access to the requested room"),
    /**
     * The token is missing a customer identifier
     */
    ODIN_ERROR_TOKEN_MISSING_CUSTOMER =
        -32 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_TOKEN_MISSING_CUSTOMER", ToolTip = "The token is missing a customer identifier"),
    /**
     * The audio processing module reported an error
     */
    ODIN_ERROR_AUDIO_PROCESSING_FAILED =
        -41 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_AUDIO_PROCESSING_FAILED", ToolTip = "The audio processing module reported an error"),
    /**
     * The setup process of the Opus audio codec reported an error
     */
    ODIN_ERROR_AUDIO_CODEC_CREATION_FAILED = -42
                                             + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_AUDIO_CODEC_CREATION_FAILED",
                                                                                     ToolTip = "The setup process of the Opus audio codec reported an error"),
    /**
     * Encoding of an audio packet failed
     */
    ODIN_ERROR_AUDIO_ENCODING_FAILED =
        -43 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_AUDIO_ENCODING_FAILED", ToolTip = "Encoding of an audio packet failed"),
    /**
     * Decoding of an audio packet failed
     */
    ODIN_ERROR_AUDIO_DECODING_FAILED =
        -44 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_AUDIO_DECODING_FAILED", ToolTip = "Decoding of an audio packet failed"),
    /**
     * Position limit reached
     */
    ODIN_ERROR_AUDIO_POSITION_LIMIT_REACHED =
        -45 + OdinUtility::EODIN_ERROR_OFFSET UMETA(DisplayName = "ERROR_AUDIO_POSITION_LIMIT_REACHED", ToolTip = "Position limit reached"),
};

/**
 * Audio-related events emitted by encoders and decoders. These values act as bitflags and
 * can be combined to filter or report multiple events.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EOdinAudioEvents : uint8 {
    NONE = 0 UMETA(Hidden),
    /**
     * Toggle silence state changes on/off
     * Triggered when the silence state changes.
     */
    AUDIO_EVENTS_IS_SILENT_CHANGED = 1 UMETA(DisplayName = "IsSilent changed"),
    /**
     * Toggle channel positions on/off
     * Triggered when 3D channel positions are added, updated or removed.
     */
    AUDIO_EVENTS_POSITIONS_CHANGED = 2 UMETA(DisplayName = "Position changed"),
    /**
     * Enables all available audio events.
     */
    AUDIO_EVENTS_ALL = static_cast<uint8>(-1) UMETA(DisplayName = "ALL"),
};
inline bool operator==(int a, EOdinAudioEvents b)
{
    return a == static_cast<uint8>(b);
}
ENUM_CLASS_FLAGS(EOdinAudioEvents);

UENUM(BlueprintType)
enum EOdinTokenSubject : uint8 {
    Connect,
};

UENUM(BlueprintType)
enum EOdinTokenAudience : uint8 {
    Sfu,
    Gateway,
};

UENUM(BlueprintType)
enum class EOdinEffectType : uint8 {
    /**
     * Voice Activity Detection (VAD) effect for detecting active speech segments in an audio stream
     */
    ODIN_EFFECT_TYPE_VAD = 0 UMETA(DisplayName = "VAD"),
    /**
     * Audio Processing Module (APM) effect to apply audio enhancements like noise suppression
     */
    ODIN_EFFECT_TYPE_APM UMETA(DisplayName = "APM"),
    /**
     * Custom user-defined audio processing effect that can be integrated into the audio pipeline
     */
    ODIN_EFFECT_TYPE_CUSTOM UMETA(DisplayName = "CUSTOM"),
};

USTRUCT(BlueprintType)
struct ODIN_API FOdinConnectionStats {
    GENERATED_BODY()

    operator OdinConnectionStats()
    {
        return OdinConnectionStats{
            .udp_tx_datagrams = (uint64)UdpTxDatagrams,
            .udp_tx_bytes     = (uint64)UdpTxBytes,
            .udp_rx_datagrams = (uint64)UdpRxDatagrams,
            .udp_rx_bytes     = (uint64)UdpRxBytes,
            .rtt              = Rtt,
        };
    }

  public:
    inline FOdinConnectionStats *SetStats(OdinConnectionStats *stats)
    {
        UdpTxDatagrams = (int64)stats->udp_tx_datagrams;
        UdpTxBytes     = (int64)stats->udp_tx_bytes;
        UdpRxDatagrams = (int64)stats->udp_rx_datagrams;
        UdpRxBytes     = (int64)stats->udp_rx_bytes;
        Rtt            = stats->rtt;

        return this;
    }
    FOdinConnectionStats() = default;
    FOdinConnectionStats(OdinConnectionStats stats)
    {
        UdpTxDatagrams = (int64)stats.udp_tx_datagrams;
        UdpTxBytes     = (int64)stats.udp_tx_bytes;
        UdpRxDatagrams = (int64)stats.udp_rx_datagrams;
        UdpRxBytes     = (int64)stats.udp_rx_bytes;
        Rtt            = stats.rtt;
    }

    /**
     * The amount of outgoing UDP datagrams observed
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UDP tx datagrams"), Category = "Odin")
    int64 UdpTxDatagrams = 0;
    /**
     * The total amount of bytes which have been transferred inside outgoing UDP datagrams
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UDP tx bytes"), Category = "Odin")
    int64 UdpTxBytes = 0;
    /**
     * The amount of incoming UDP datagrams observed
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UDP rx datagrams"), Category = "Odin")
    int64 UdpRxDatagrams = 0;
    /**
     * The total amount of bytes which have been transferred inside incoming UDP datagrams
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UDP rx bytes"), Category = "Odin")
    int64 UdpRxBytes = 0;
    /**
     * Current best estimate of the connection latency (round-trip-time) in milliseconds
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Round-Trip-Time"), Category = "Odin")
    float Rtt = 0;
};

UENUM(BlueprintType)
enum EOdinNoiseSuppression : uint8 {
    /**
     * Noise suppression is disabled
     */
    ODIN_NOISE_SUPPRESSION_NONE = 0 UMETA(DisplayName = "NONE"),
    /**
     * Use low suppression (6 dB)
     */
    ODIN_NOISE_SUPPRESSION_LOW UMETA(DisplayName = "LOW (6 dB)"),
    /**
     * Use moderate suppression (12 dB)
     */
    ODIN_NOISE_SUPPRESSION_MODERATE UMETA(DisplayName = "MODERATE (12 dB)"),
    /**
     * Use high suppression (18 dB)
     */
    ODIN_NOISE_SUPPRESSION_HIGH UMETA(DisplayName = "HIGH (18 dB)"),
    /**
     * Use very high suppression (21 dB)
     */
    ODIN_NOISE_SUPPRESSION_VERY_HIGH UMETA(DisplayName = "VERY_HIGH (21 dB)"),
};

UENUM(BlueprintType)
enum EOdinGainControllerVersion : uint8 {
    /**
     * AGC is disabled; the signal is not modified
     */
    ODIN_GAIN_CONTROLLER_DISABLED = 0 UMETA(DisplayName = "Disabled AGC"),
    /**
     * Legacy AGC with adaptive digital gain control and a limiter
     */
    ODIN_GAIN_CONTROLLER_V1 UMETA(DisplayName = "Legacy AGC"),
    /**
     * Enhanced AGC with improved digital processing and an input volume controller
     */
    ODIN_GAIN_CONTROLLER_V2 UMETA(DisplayName = "Enhanced AGC"),
};

/**
 * Configuration of the ODIN audio processing module which provides a variety of smart enhancement algorithms.
 */
USTRUCT(BlueprintType)
struct ODIN_API FOdinApmConfig {
    GENERATED_BODY()

    operator OdinApmConfig()
    {
        return OdinApmConfig{.echo_canceller          = echo_canceller,
                             .high_pass_filter        = high_pass_filter,
                             .transient_suppressor    = transient_suppressor,
                             .noise_suppression_level = (OdinNoiseSuppressionLevel)noise_suppression.GetIntValue(),
                             .gain_controller_version = (OdinGainControllerVersion)gain_controller.GetIntValue()};
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Echo Canceller"), Category = "Odin|APM")
    bool echo_canceller = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "High Pass Filtering"), Category = "Odin|APM")
    bool high_pass_filter = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Transient Suppression"), Category = "Odin|APM")
    bool transient_suppressor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Noise Suppression"), Category = "Odin|APM")
    TEnumAsByte<EOdinNoiseSuppression> noise_suppression = EOdinNoiseSuppression::ODIN_NOISE_SUPPRESSION_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Gain Controller"), Category = "Odin|APM")
    TEnumAsByte<EOdinGainControllerVersion> gain_controller = EOdinGainControllerVersion::ODIN_GAIN_CONTROLLER_DISABLED;
};

/**
 * Sensitivity parameters for ODIN voice activity detection module configuration.
 */
USTRUCT(BlueprintType)
struct ODIN_API FOdinSensitivityConfig {
    GENERATED_BODY()

    operator OdinSensitivityConfig()
    {
        return OdinSensitivityConfig{
            .enabled           = Enabled,
            .attack_threshold  = AttackThreshold,
            .release_threshold = ReleaseThreshold,
        };
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Sensitivity configuration"), Category = "Odin|APM")
    bool Enabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Attack Threshold", ClampMin = "-90.0", ClampMax = "0.0", UIMin = "-90.0", UIMax = "0.0"),
              Category = "Odin|APM")
    float AttackThreshold = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (DisplayName = "Release Threshold", ClampMin = "-90.0", ClampMax = "0.0", UIMin = "-90.0", UIMax = "0.0"), Category = "Odin|APM")
    float ReleaseThreshold = 0;
};

/**
 * Configuration of the ODIN voice activity detection module, which offers advanced
 * algorithms to accurately determine when to start or stop transmitting audio data.
 */
USTRUCT(BlueprintType)
struct ODIN_API FOdinVadConfig {
    GENERATED_BODY()

    operator OdinVadConfig()
    {
        return OdinVadConfig{
            .voice_activity = VoiceActivity,
            .volume_gate    = VolumeGate,
        };
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Voice Activity Detection"), Category = "Odin|APM")
    FOdinSensitivityConfig VoiceActivity = FOdinSensitivityConfig{.Enabled = false, .AttackThreshold = 0.9f, .ReleaseThreshold = 0.8f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Volume Gate"), Category = "Odin|APM")
    FOdinSensitivityConfig VolumeGate = FOdinSensitivityConfig{.Enabled = false, .AttackThreshold = -30.0f, .ReleaseThreshold = -40.0f};
};

/**
 * Represents a 3D position used for peer proximity calculations.
 * The coordinates are expressed as floating - point32 values(x, y, z).
 */
USTRUCT(BlueprintType)
struct ODIN_API FOdinPosition {
    GENERATED_BODY()

    FOdinPosition();
    FOdinPosition(const FVector &In);

    operator OdinPosition() const
    {
        return OdinPosition{x, y, z};
    }

    operator FVector() const
    {
        return FVector(x, y, z);
    }

    FString ToString() const
    {
        return FString::Printf(TEXT("X: %f, Y: %f, Z: %f"), x, y, z);
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "X"), Category = "Odin|Position")
    float x = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Y"), Category = "Odin|Position")
    float y = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Z"), Category = "Odin|Position")
    float z = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeInitializeResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeShutdownResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeErrorGetLastErrorResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeErrorResetLastErrorResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeDecoderCreateResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeDecoderFreeResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeDecoderIsSilentResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeDecoderPushResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeDecoderPopResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeDecoderGetPipelineResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeEncoderCreateResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeEncoderCreateExResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeEncoderFreeResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeEncoderPushResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeEncoderIsSilentResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeEncoderPopResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeEncoderGetPipelineResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineInsertVadEffectResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineInsertApmEffectResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineInsertCustomEffectResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineRemoveEffectResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineMoveEffectResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineGetEffectIdResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineGetEffectIndexResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineGetEffectTypeResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineGetEffectCountResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineSetVadConfigResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineGetVadConfigResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineSetApmConfigResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineSetApmStreamDelayResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativePipelineUpdateApmPlaybackResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomCreateResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomCreateExResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomCloseResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomFreeResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomGetIdShutdownResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomGetNameResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomGetConnectionIdResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomGetConnectionStatsResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomResendUserDataResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomSendRpcResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomSendLoopbackRpcResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeRoomSendDatagramResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeTokenGeneratorCreateResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeTokenGeneratorFreeResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeTokenGeneratorGetAccessKeyResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeTokenGeneratorGetKeyIdResponsePin, bool, success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinNativeTokenGeneratorSignResponsePin, bool, success);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeInitializeError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeInitializeSuccess, FString, version);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeInitialize : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin Initialize", ToolTip = "odin_initialize",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeInitialize *Initialize(UObject *WorldContextObject, const FString version, const FOdinNativeInitializeError &onError,
                                             const FOdinNativeInitializeSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeInitializeResponsePin OnResponse;

    FString                      Version;
    FOdinNativeInitializeError   OnError;
    FOdinNativeInitializeSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE(FOdinNativeShutdownError);
DECLARE_DYNAMIC_DELEGATE(FOdinNativeShutdownSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeShutdown : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin Shutdown", ToolTip = "odin_shutdown",
                                         WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeShutdown *Shutdown(UObject *WorldContextObject, const FOdinNativeShutdownError &onError, const FOdinNativeShutdownSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeShutdownResponsePin OnResponse;

    FOdinNativeShutdownError   OnError;
    FOdinNativeShutdownSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeErrorGetLastErrorError, FString, reason);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeErrorGetLastErrorSuccess, FString, lastError);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeErrorGetLastError : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin Shutdown", ToolTip = "odin_shutdown",
                                         WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeErrorGetLastError *ErrorGetLastError(UObject *WorldContextObject, const FOdinNativeErrorGetLastErrorError &onError,
                                                           const FOdinNativeErrorGetLastErrorSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeErrorGetLastErrorResponsePin OnResponse;

    FString LastError;

    FOdinNativeErrorGetLastErrorError   OnError;
    FOdinNativeErrorGetLastErrorSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeErrorResetLastErrorError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinNativeErrorResetLastErrorSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeErrorResetLastError : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin Shutdown", ToolTip = "odin_shutdown",
                                         WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeErrorResetLastError *ErrorResetLastError(UObject *WorldContextObject, const FOdinNativeErrorResetLastErrorError &onError,
                                                               const FOdinNativeErrorResetLastErrorSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeErrorResetLastErrorResponsePin OnResponse;

    FOdinNativeErrorResetLastErrorError   OnError;
    FOdinNativeErrorResetLastErrorSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeDecoderIsSilentError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeDecoderIsSilentSuccess, bool, silent);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeDecoderIsSilent : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin DecoderIsSilent", ToolTip = "odin_DecoderIsSilent",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeDecoderIsSilent *DecoderIsSilent(UObject *WorldContextObject, UPARAM(ref) UOdinDecoder *&decoder, const bool &bIsSilent,
                                                       const FOdinNativeDecoderIsSilentError &onError, const FOdinNativeDecoderIsSilentSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeDecoderIsSilentResponsePin OnResponse;

    UPROPERTY()
    UOdinDecoder                     *Decoder;
    bool                              Silent;
    FOdinNativeDecoderIsSilentError   OnError;
    FOdinNativeDecoderIsSilentSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeDecoderPushError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeDecoderPushSuccess, uint32, datagram_length);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeDecoderPush : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin DecoderPush", ToolTip = "odin_DecoderPush",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeDecoderPush *DecoderPush(UObject *WorldContextObject, UPARAM(ref) UOdinDecoder *&decoder, const TArray<uint8> &datagram,
                                               const FOdinNativeDecoderPushError &onError, const FOdinNativeDecoderPushSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeDecoderPushResponsePin OnResponse;

    UPROPERTY()
    UOdinDecoder                 *Decoder;
    TArray<uint8>                 Datagram;
    FOdinNativeDecoderPushError   OnError;
    FOdinNativeDecoderPushSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeDecoderPopError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativeDecoderPopSuccess, const TArray<float> &, samples, bool, isSilent);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeDecoderPop : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin DecoderPop", ToolTip = "odin_DecoderPop",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeDecoderPop *DecoderPop(UObject *WorldContextObject, UPARAM(ref) UOdinDecoder *&decoder, const TArray<float> &buffer,
                                             const FOdinNativeDecoderPopError &onError, const FOdinNativeDecoderPopSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeDecoderPopResponsePin OnResponse;

    UPROPERTY()
    UOdinDecoder                *Decoder;
    TArray<float>                Samples;
    bool                         IsSilent;
    FOdinNativeDecoderPopError   OnError;
    FOdinNativeDecoderPopSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeDecoderGetPipelineError, UOdinDecoder *, decoder);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeDecoderGetPipelineSuccess, const UOdinPipeline *, pipeline);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeDecoderGetPipeline : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin DecoderGetPipeline", ToolTip = "odin_DecoderGetPipeline",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeDecoderGetPipeline *DecoderGetPipeline(UObject *WorldContextObject, UPARAM(ref) UOdinDecoder *&decoder,
                                                             const FOdinNativeDecoderGetPipelineError   &onError,
                                                             const FOdinNativeDecoderGetPipelineSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeDecoderGetPipelineResponsePin OnResponse;

    TWeakObjectPtr<UOdinDecoder>         Decoder;
    FOdinNativeDecoderGetPipelineError   OnError;
    FOdinNativeDecoderGetPipelineSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderCreateError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_FourParams(FOdinNativeEncoderCreateSuccess, uint32, peer_id, uint32, sample_rate, bool, stereo, UOdinEncoder *, encoder);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeEncoderCreate : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin EncoderCreate", ToolTip = "odin_EncoderCreate",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeEncoderCreate *EncoderCreate(UObject *WorldContextObject, UPARAM(ref) UOdinEncoder *&encoder, int64 peer_id, int32 sample_rate,
                                                   const bool stereo, const FOdinNativeEncoderCreateError &onError,
                                                   const FOdinNativeEncoderCreateSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeEncoderCreateResponsePin OnResponse;

    UPROPERTY()
    int64 PeerId;
    UPROPERTY()
    uint32 Samplerate;
    UPROPERTY()
    bool Stereo;

    TWeakObjectPtr<UOdinEncoder>    Encoder;
    FOdinNativeEncoderCreateError   OnError;
    FOdinNativeEncoderCreateSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderCreateExError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_SevenParams(FOdinNativeEncoderCreateExSuccess, int64, peer_id, uint32, sample_rate, bool, stereo, bool, application_voip, uint32,
                                     bitrate_kbps, int64, update_position_interval, UOdinEncoder *, encoder);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeEncoderCreateEx : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin EncoderCreateEx", ToolTip = "odin_EncoderCreateEx",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeEncoderCreateEx *EncoderCreateEx(UObject *WorldContextObject, UPARAM(ref) UOdinEncoder *&encoder, int64 peer_id, int32 sample_rate,
                                                       const bool stereo, const bool application_voip, int32 bitrate_kbps, int32 packet_loss_perc,
                                                       int64 update_position_interval_ms, const FOdinNativeEncoderCreateExError &onError,
                                                       const FOdinNativeEncoderCreateExSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeEncoderCreateExResponsePin OnResponse;

    UPROPERTY()
    uint32 PeerId;
    UPROPERTY()
    uint32 Samplerate;
    UPROPERTY()
    bool Stereo;
    UPROPERTY()
    bool ApplicationVoip;
    UPROPERTY()
    uint32 BitrateKbps;
    UPROPERTY()
    uint32 PacketLossPerc;
    UPROPERTY()
    int64 UpdatePositionInterval;

    TWeakObjectPtr<UOdinEncoder>      Encoder;
    FOdinNativeEncoderCreateExError   OnError;
    FOdinNativeEncoderCreateExSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE(FOdinNativeEncoderFreeError);
DECLARE_DYNAMIC_DELEGATE(FOdinNativeEncoderFreeSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeEncoderFree : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin EncoderFree", ToolTip = "odin_EncoderFree",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeEncoderFree *EncoderFree(UObject *WorldContextObject, UPARAM(ref) UOdinEncoder *&encoder, const FOdinNativeEncoderFreeError &onError,
                                               const FOdinNativeEncoderFreeSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeEncoderFreeResponsePin OnResponse;

    UPROPERTY()
    UOdinEncoder                 *Encoder;
    FOdinNativeEncoderFreeError   OnError;
    FOdinNativeEncoderFreeSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderIsSilentError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderIsSilentSuccess, bool, silent);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeEncoderIsSilent : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin EncoderIsSilent", ToolTip = "odin_EncoderIsSilent",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeEncoderIsSilent *EncoderIsSilent(UObject *WorldContextObject, UPARAM(ref) UOdinEncoder *&decoder, const bool &bIsSilent,
                                                       const FOdinNativeEncoderIsSilentError &onError, const FOdinNativeEncoderIsSilentSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeEncoderIsSilentResponsePin OnResponse;

    UPROPERTY()
    UOdinEncoder                     *Encoder;
    bool                              Silent;
    FOdinNativeEncoderIsSilentError   OnError;
    FOdinNativeEncoderIsSilentSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderPushError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderPushSuccess, uint32, samples_count);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeEncoderPush : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin EncoderPush", ToolTip = "odin_EncoderPush",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeEncoderPush *EncoderPush(UObject *WorldContextObject, UPARAM(ref) UOdinEncoder *&encoder, const TArray<float> &samples,
                                               const FOdinNativeEncoderPushError &onError, const FOdinNativeEncoderPushSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeEncoderPushResponsePin OnResponse;

    UPROPERTY()
    UOdinEncoder                 *Encoder;
    TArray<float>                 Samples;
    FOdinNativeEncoderPushError   OnError;
    FOdinNativeEncoderPushSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderPopError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderPopSuccess, const TArray<uint8> &, datagram);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeEncoderPop : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin EncoderPop", ToolTip = "odin_EncoderPop",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeEncoderPop *EncoderPop(UObject *WorldContextObject, UPARAM(ref) UOdinEncoder *&encoder, const TArray<uint8> &buffer,
                                             const FOdinNativeEncoderPopError &onError, const FOdinNativeEncoderPopSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeEncoderPopResponsePin OnResponse;

    TArray<uint8> Datagram;
    UPROPERTY()
    UOdinEncoder                *Encoder;
    FOdinNativeEncoderPopError   OnError;
    FOdinNativeEncoderPopSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderGetPipelineError, UOdinEncoder *, encoder);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeEncoderGetPipelineSuccess, UOdinPipeline *, pipeline);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeEncoderGetPipeline : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin EncoderGetPipeline", ToolTip = "odin_EncoderGetPipeline",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeEncoderGetPipeline *EncoderGetPipeline(UObject *WorldContextObject, UPARAM(ref) UOdinEncoder *&encoder,
                                                             const FOdinNativeEncoderGetPipelineError   &onError,
                                                             const FOdinNativeEncoderGetPipelineSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeEncoderGetPipelineResponsePin OnResponse;

    UPROPERTY()
    UOdinEncoder                        *Encoder;
    const OdinPipeline                  *Pipeline;
    FOdinNativeEncoderGetPipelineError   OnError;
    FOdinNativeEncoderGetPipelineSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineInsertVadEffectError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativePipelineInsertVadEffectSuccess, uint32, index, uint32, effectId);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineInsertVadEffect : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineInsertVadEffect",
                                         ToolTip = "odin_PipelineInsertVadEffect", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineInsertVadEffect *PipelineInsertVadEffect(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, int32 index,
                                                                       const FOdinNativePipelineInsertVadEffectError   &onError,
                                                                       const FOdinNativePipelineInsertVadEffectSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineInsertVadEffectResponsePin OnResponse;

    UPROPERTY()
    uint32 Index;

    uint32 EffectId;
    UPROPERTY()
    UOdinPipeline *Pipeline;

    FOdinNativePipelineInsertVadEffectError   OnError;
    FOdinNativePipelineInsertVadEffectSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineInsertApmEffectError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativePipelineInsertApmEffectSuccess, uint32, index, uint32, effectId);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineInsertApmEffect : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineInsertApmEffect",
                                         ToolTip = "odin_PipelineInsertApmEffect", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineInsertApmEffect *PipelineInsertApmEffect(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, int32 index,
                                                                       int32 playback_sample_rate, const bool playback_stereo,
                                                                       const FOdinNativePipelineInsertApmEffectError   &onError,
                                                                       const FOdinNativePipelineInsertApmEffectSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineInsertApmEffectResponsePin OnResponse;

    UPROPERTY()
    uint32 Index;
    UPROPERTY()
    uint32 PlaybackSamplerate;
    UPROPERTY()
    bool PlaybackStereo;

    uint32 EffectId;
    UPROPERTY()
    UOdinPipeline *Pipeline;

    FOdinNativePipelineInsertApmEffectError   OnError;
    FOdinNativePipelineInsertApmEffectSuccess OnSuccess;
};

// TODO UOdinCustomEffectCallback of OdinCustomEffectCallback
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineInsertCustomEffectError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativePipelineInsertCustomEffectSuccess, uint32, index, uint32, effectId);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineInsertCustomEffect : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineInsertCustomEffect",
                                         ToolTip = "odin_PipelineInsertCustomEffect", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineInsertCustomEffect *PipelineInsertCustomEffect(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline,
                                                                             UPARAM(ref) UOdinCustomEffect *&effect, int32 index,
                                                                             const FOdinNativePipelineInsertCustomEffectError   &onError,
                                                                             const FOdinNativePipelineInsertCustomEffectSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineInsertCustomEffectResponsePin OnResponse;

    UPROPERTY()
    uint32 Index;
    uint32 EffectId;
    UPROPERTY()
    UOdinPipeline *Pipeline;
    UPROPERTY()
    UOdinCustomEffect                           *Effect;
    FOdinNativePipelineInsertCustomEffectError   OnError;
    FOdinNativePipelineInsertCustomEffectSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineRemoveEffectError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineRemoveEffectSuccess, uint32, effectId);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineRemoveEffect : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineRemoveEffect",
                                         ToolTip = "odin_PipelineRemoveEffect", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineRemoveEffect *PipelineRemoveEffect(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, int32 effectId,
                                                                 const FOdinNativePipelineRemoveEffectError   &onError,
                                                                 const FOdinNativePipelineRemoveEffectSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineRemoveEffectResponsePin OnResponse;

    UPROPERTY()
    uint32 EffectId;
    UPROPERTY()
    UOdinPipeline *Pipeline;

    FOdinNativePipelineRemoveEffectError   OnError;
    FOdinNativePipelineRemoveEffectSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineMoveEffectError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativePipelineMoveEffectSuccess, uint32, effectId, uint32, newIndex);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineMoveEffect : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineMoveEffect", ToolTip = "odin_PipelineMoveEffect",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineMoveEffect *PipelineMoveEffect(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, int32 effect_id, int32 newIndex,
                                                             const FOdinNativePipelineMoveEffectError   &onError,
                                                             const FOdinNativePipelineMoveEffectSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineMoveEffectResponsePin OnResponse;

    UPROPERTY()
    uint32 EffectId;
    UPROPERTY()
    uint32 NewIndex;
    UPROPERTY()
    UOdinPipeline                       *Pipeline;
    FOdinNativePipelineMoveEffectError   OnError;
    FOdinNativePipelineMoveEffectSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineGetEffectIdError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativePipelineGetEffectIdSuccess, uint32, effectId, uint32, index);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineGetEffectId : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineGetEffectId",
                                         ToolTip = "odin_PipelineGetEffectId", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineGetEffectId *PipelineGetEffectId(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, int32 index,
                                                               const FOdinNativePipelineGetEffectIdError   &onError,
                                                               const FOdinNativePipelineGetEffectIdSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineGetEffectIdResponsePin OnResponse;

    uint32 Index;
    uint32 EffectId;
    UPROPERTY()
    UOdinPipeline                        *Pipeline;
    FOdinNativePipelineGetEffectIdError   OnError;
    FOdinNativePipelineGetEffectIdSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineGetEffectIndexError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativePipelineGetEffectIndexSuccess, uint32, effectId, uint32, index);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineGetEffectIndex : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineGetEffectIndex",
                                         ToolTip = "odin_PipelineGetEffectIndex", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineGetEffectIndex *PipelineGetEffectIndex(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, int32 effectId,
                                                                     const FOdinNativePipelineGetEffectIndexError   &onError,
                                                                     const FOdinNativePipelineGetEffectIndexSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineGetEffectIndexResponsePin OnResponse;

    UPROPERTY()
    uint32 EffectId;
    uint32 Index;
    UPROPERTY()
    UOdinPipeline                           *Pipeline;
    FOdinNativePipelineGetEffectIndexError   OnError;
    FOdinNativePipelineGetEffectIndexSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineGetEffectTypeError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativePipelineGetEffectTypeSuccess, uint32, effect_id, EOdinEffectType, effectType);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineGetEffectType : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineGetEffectType",
                                         ToolTip = "odin_PipelineGetEffectType", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineGetEffectType *PipelineGetEffectType(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, int32 effect_id,
                                                                   const FOdinNativePipelineGetEffectTypeError   &onError,
                                                                   const FOdinNativePipelineGetEffectTypeSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineGetEffectTypeResponsePin OnResponse;

    UPROPERTY()
    uint32 EffectId;
    UPROPERTY()
    EOdinEffectType EffectType;
    UPROPERTY()
    UOdinPipeline                          *Pipeline;
    FOdinNativePipelineGetEffectTypeError   OnError;
    FOdinNativePipelineGetEffectTypeSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineGetEffectCountError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineGetEffectCountSuccess, uint32, effectCount);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineGetEffectCount : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineGetEffectCount",
                                         ToolTip = "odin_PipelineGetEffectCount", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineGetEffectCount *PipelineGetEffectCount(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline,
                                                                     const FOdinNativePipelineGetEffectCountError   &onError,
                                                                     const FOdinNativePipelineGetEffectCountSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineGetEffectCountResponsePin OnResponse;

    uint32 EffectCount;
    UPROPERTY()
    UOdinPipeline                           *Pipeline;
    FOdinNativePipelineGetEffectCountError   OnError;
    FOdinNativePipelineGetEffectCountSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineSetVadConfigError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineSetVadConfigSuccess, uint32, effect_id);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineSetVadConfig : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineSetVadConfig",
                                         ToolTip = "odin_PipelineSetVadConfig", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineSetVadConfig *PipelineSetVadConfig(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, const int32 effect_id,
                                                                 UPARAM(ref) struct FOdinVadConfig &config, const FOdinNativePipelineSetVadConfigError &onError,
                                                                 const FOdinNativePipelineSetVadConfigSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineSetVadConfigResponsePin OnResponse;

    UPROPERTY()
    uint32 EffectId;

    FOdinVadConfig *Config;

    UPROPERTY()
    UOdinPipeline                         *Pipeline;
    FOdinNativePipelineSetVadConfigError   OnError;
    FOdinNativePipelineSetVadConfigSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineGetVadConfigError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineGetVadConfigSuccess, uint32, effect_id /*, const OdinVadConfig &, vadConfig*/);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineGetVadConfig : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineGetVadConfig",
                                         ToolTip = "odin_PipelineGetVadConfig", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineGetVadConfig *PipelineGetVadConfig(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, const int32 effect_id,
                                                                 const FOdinNativePipelineGetVadConfigError   &onError,
                                                                 const FOdinNativePipelineGetVadConfigSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineGetVadConfigResponsePin OnResponse;

    UPROPERTY()
    uint32 EffectId;

    OdinVadConfig *Config;
    UPROPERTY()
    UOdinPipeline                         *Pipeline;
    FOdinNativePipelineGetVadConfigError   OnError;
    FOdinNativePipelineGetVadConfigSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineSetApmConfigError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineSetApmConfigSuccess, uint32, effect_id);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineSetApmConfig : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineSetApmConfig",
                                         ToolTip = "odin_PipelineSetApmConfig", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineSetApmConfig *PipelineSetApmConfig(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline, const int32 effect_id,
                                                                 const FOdinNativePipelineSetApmConfigError   &onError,
                                                                 const FOdinNativePipelineSetApmConfigSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineSetApmConfigResponsePin OnResponse;

    UPROPERTY()
    uint32 EffectId;

    OdinApmConfig *Config;
    UPROPERTY()
    UOdinPipeline                         *Pipeline;
    FOdinNativePipelineSetApmConfigError   OnError;
    FOdinNativePipelineSetApmConfigSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativePipelineUpdateApmPlaybackError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOdinNativePipelineUpdateApmPlaybackSuccess, uint32, effect_id, uint32, samples_count);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativePipelineUpdateApmPlayback : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin PipelineUpdateApmPlayback",
                                         ToolTip = "odin_PipelineUpdateApmPlayback", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativePipelineUpdateApmPlayback *PipelineUpdateApmPlayback(UObject *WorldContextObject, UPARAM(ref) UOdinPipeline *&pipeline,
                                                                           const int32 effect_id, const TArray<float> &samples, const int32 samples_count,
                                                                           const int32 delay, const FOdinNativePipelineUpdateApmPlaybackError &onError,
                                                                           const FOdinNativePipelineUpdateApmPlaybackSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativePipelineUpdateApmPlaybackResponsePin OnResponse;
    UPROPERTY()
    uint32        EffectId;
    uint32        Delay;
    TArray<float> Samples;
    UPROPERTY()
    UOdinPipeline                              *Pipeline;
    FOdinNativePipelineUpdateApmPlaybackError   OnError;
    FOdinNativePipelineUpdateApmPlaybackSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomCreateError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomCreateSuccess, UOdinRoom *, room);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomCreate : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomCreate", ToolTip = "odin_RoomCreate",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomCreate *RoomCreate(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, const FString gateway, const FString authentication,
                                             const FOdinNativeRoomCreateError &onError, const FOdinNativeRoomCreateSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomCreateResponsePin OnResponse;

    TWeakObjectPtr<UOdinRoom>    Room;
    FString                      Gateway;
    FString                      Authentication;
    FOdinNativeRoomCreateError   OnError;
    FOdinNativeRoomCreateSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE(FOdinNativeRoomCloseError);
DECLARE_DYNAMIC_DELEGATE(FOdinNativeRoomCloseSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomClose : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomClose", ToolTip = "odin_RoomClose",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomClose *RoomClose(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, const FOdinNativeRoomCloseError &onError,
                                           const FOdinNativeRoomCloseSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomCloseResponsePin OnResponse;

    UPROPERTY()
    UOdinRoom                  *Room;
    FOdinNativeRoomCloseError   OnError;
    FOdinNativeRoomCloseSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE(FOdinNativeRoomFreeError);
DECLARE_DYNAMIC_DELEGATE(FOdinNativeRoomFreeSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomFree : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomFree", ToolTip = "odin_RoomFree",
                                         WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomFree *RoomFree(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, const FOdinNativeRoomFreeError &onError,
                                         const FOdinNativeRoomFreeSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomFreeResponsePin OnResponse;

    UPROPERTY()
    UOdinRoom                 *Room;
    FOdinNativeRoomFreeError   OnError;
    FOdinNativeRoomFreeSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomGetNameError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomGetNameSuccess, FString, roomName);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomGetName : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomGetName", ToolTip = "odin_RoomGetName",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomGetName *RoomGetName(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, const FOdinNativeRoomGetNameError &onError,
                                               const FOdinNativeRoomGetNameSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomGetNameResponsePin OnResponse;

    UPROPERTY()
    FString RoomName;
    UPROPERTY()
    UOdinRoom                    *Room;
    FOdinNativeRoomGetNameError   OnError;
    FOdinNativeRoomGetNameSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomGetConnectionIdError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomGetConnectionIdSuccess, uint64, connectionId);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomGetConnectionId : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomGetConnectionId",
                                         ToolTip = "odin_RoomGetConnectionId", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomGetConnectionId *RoomGetConnectionId(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room,
                                                               const FOdinNativeRoomGetConnectionIdError   &onError,
                                                               const FOdinNativeRoomGetConnectionIdSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomGetConnectionIdResponsePin OnResponse;
    UPROPERTY()
    uint64 ConnectionId;
    UPROPERTY()
    UOdinRoom                            *Room;
    FOdinNativeRoomGetConnectionIdError   OnError;
    FOdinNativeRoomGetConnectionIdSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomGetConnectionStatsError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomGetConnectionStatsSuccess, FOdinConnectionStats, stats);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomGetConnectionStats : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomGetConnectionStats",
                                         ToolTip = "odin_RoomGetConnectionStats", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomGetConnectionStats *RoomGetConnectionStats(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room,
                                                                     const FOdinNativeRoomGetConnectionStatsError   &onError,
                                                                     const FOdinNativeRoomGetConnectionStatsSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomGetConnectionStatsResponsePin OnResponse;

    FOdinConnectionStats Stats;
    UPROPERTY()
    UOdinRoom                               *Room;
    FOdinNativeRoomGetConnectionStatsError   OnError;
    FOdinNativeRoomGetConnectionStatsSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomResendUserDataError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE(FOdinNativeRoomResendUserDataSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomResendUserData : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomResendUserData", ToolTip = "odin_RoomResendUserData",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomResendUserData *RoomResendUserData(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room,
                                                             const FOdinNativeRoomResendUserDataError   &onError,
                                                             const FOdinNativeRoomResendUserDataSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomResendUserDataResponsePin OnResponse;
    UPROPERTY()
    UOdinRoom                           *Room;
    FOdinNativeRoomResendUserDataError   OnError;
    FOdinNativeRoomResendUserDataSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomSendRpcError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomSendRpcSuccess, FString, json);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomSendRpc : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomSendRpc", ToolTip = "odin_RoomSendRpc",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomSendRpc *RoomSendRpc(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, FString rpcBody,
                                               const FOdinNativeRoomSendRpcError &onError, const FOdinNativeRoomSendRpcSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomSendRpcResponsePin OnResponse;

    FString RpcBody;
    UPROPERTY()
    UOdinRoom                    *Room;
    FOdinNativeRoomSendRpcError   OnError;
    FOdinNativeRoomSendRpcSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomSendLoopbackRpcError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomSendLoopbackRpcSuccess, uint32, length);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomSendLoopbackRpc : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomSendLoopbackRpc",
                                         ToolTip = "odin_RoomSendLoopbackRpc", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomSendLoopbackRpc *RoomSendLoopbackRpc(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, FString rpcBody,
                                                               const FOdinNativeRoomSendLoopbackRpcError   &onError,
                                                               const FOdinNativeRoomSendLoopbackRpcSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomSendLoopbackRpcResponsePin OnResponse;

    FString RpcBody;
    UPROPERTY()
    UOdinRoom                            *Room;
    FOdinNativeRoomSendLoopbackRpcError   OnError;
    FOdinNativeRoomSendLoopbackRpcSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomSendDatagramError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeRoomSendDatagramSuccess, uint32, bytes);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeRoomSendDatagram : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin RoomSendDatagram", ToolTip = "odin_RoomSendDatagram",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeRoomSendDatagram *RoomSendDatagram(UObject *WorldContextObject, UPARAM(ref) UOdinRoom *&room, const TArray<uint8> &datagram,
                                                         const FOdinNativeRoomSendDatagramError &onError, const FOdinNativeRoomSendDatagramSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeRoomSendDatagramResponsePin OnResponse;

    TArray<uint8> DatagramBytes;
    UPROPERTY()
    UOdinRoom                         *Room;
    FOdinNativeRoomSendDatagramError   OnError;
    FOdinNativeRoomSendDatagramSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeTokenGeneratorCreateError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeTokenGeneratorCreateSuccess, UOdinTokenGenerator *, tokenGenerator);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeTokenGeneratorCreate : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin TokenGeneratorCreate",
                                         ToolTip = "odin_TokenGeneratorCreate", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeTokenGeneratorCreate *TokenGeneratorCreate(UObject *WorldContextObject, FString access_key,
                                                                 const FOdinNativeTokenGeneratorCreateError   &onError,
                                                                 const FOdinNativeTokenGeneratorCreateSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeTokenGeneratorCreateResponsePin OnResponse;
    UPROPERTY()
    FString AccessKey;

    UPROPERTY()
    UOdinTokenGenerator *TokenGenerator;

    FOdinNativeTokenGeneratorCreateError   OnError;
    FOdinNativeTokenGeneratorCreateSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE(FOdinNativeTokenGeneratorFreeError);
DECLARE_DYNAMIC_DELEGATE(FOdinNativeTokenGeneratorFreeSuccess);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeTokenGeneratorFree : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin TokenGeneratorFree", ToolTip = "odin_TokenGeneratorFree",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeTokenGeneratorFree *TokenGeneratorFree(UObject *WorldContextObject, UPARAM(ref) UOdinTokenGenerator *&tokenGenerator,
                                                             const FOdinNativeTokenGeneratorFreeError   &onError,
                                                             const FOdinNativeTokenGeneratorFreeSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeTokenGeneratorFreeResponsePin OnResponse;
    UPROPERTY()
    UOdinTokenGenerator                 *TokenGenerator;
    FOdinNativeTokenGeneratorFreeError   OnError;
    FOdinNativeTokenGeneratorFreeSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeTokenGeneratorGetAccessKeyError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeTokenGeneratorGetAccessKeySuccess, FString, accessKey);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeTokenGeneratorGetAccessKey : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin TokenGeneratorGetAccessKey",
                                         ToolTip = "odin_TokenGeneratorGetAccessKey", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeTokenGeneratorGetAccessKey *TokenGeneratorGetAccessKey(UObject *WorldContextObject, UPARAM(ref) UOdinTokenGenerator *&tokenGenerator,
                                                                             const FOdinNativeTokenGeneratorGetAccessKeyError   &onError,
                                                                             const FOdinNativeTokenGeneratorGetAccessKeySuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeTokenGeneratorGetAccessKeyResponsePin OnResponse;

    FString AccessKey;
    UPROPERTY()
    UOdinTokenGenerator                         *TokenGenerator;
    FOdinNativeTokenGeneratorGetAccessKeyError   OnError;
    FOdinNativeTokenGeneratorGetAccessKeySuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeTokenGeneratorGetKeyIdError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeTokenGeneratorGetKeyIdSuccess, FString, keyId);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeTokenGeneratorGetKeyId : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin TokenGeneratorGetKeyId",
                                         ToolTip = "odin_TokenGeneratorGetKeyId", WorldContext = "WorldContextObject",
                                         AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeTokenGeneratorGetKeyId *TokenGeneratorGetKeyId(UObject *WorldContextObject, UPARAM(ref) UOdinTokenGenerator *&tokenGenerator,
                                                                     const FOdinNativeTokenGeneratorGetKeyIdError   &onError,
                                                                     const FOdinNativeTokenGeneratorGetKeyIdSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeTokenGeneratorGetKeyIdResponsePin OnResponse;

    FString KeyId;
    UPROPERTY()
    UOdinTokenGenerator                     *TokenGenerator;
    FOdinNativeTokenGeneratorGetKeyIdError   OnError;
    FOdinNativeTokenGeneratorGetKeyIdSuccess OnSuccess;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeTokenGeneratorSignError, EOdinError, errorCode);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOdinNativeTokenGeneratorSignSuccess, FString, token);
UCLASS(ClassGroup = Odin)
class ODIN_API UOdinNativeTokenGeneratorSign : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", Category = "Odin|Native", DisplayName = "Odin TokenGeneratorSign", ToolTip = "odin_TokenGeneratorSign",
                      WorldContext = "WorldContextObject", AutoCreateRefTerm = "initialPeerUserData,url,onSuccess,onError"))
    static UOdinNativeTokenGeneratorSign *TokenGeneratorSign(UObject *WorldContextObject, UPARAM(ref) UOdinTokenGenerator *&tokenGenerator, const FString body,
                                                             const FOdinNativeTokenGeneratorSignError   &onError,
                                                             const FOdinNativeTokenGeneratorSignSuccess &onSuccess);

    virtual void Activate() override;

    UPROPERTY(BlueprintAssignable, Category = "Odin|Events")
    FOdinNativeTokenGeneratorSignResponsePin OnResponse;

    UPROPERTY()
    FString Body;
    FString Token;
    UPROPERTY()
    UOdinTokenGenerator                 *TokenGenerator;
    FOdinNativeTokenGeneratorSignError   OnError;
    FOdinNativeTokenGeneratorSignSuccess OnSuccess;
};