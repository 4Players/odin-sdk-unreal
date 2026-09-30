/* Copyright (c) 2022-2026 4Players GmbH. All rights reserved. */

/**
 * Compile-time checks to stay in sync with odin.h native counterparts.
 *
 * For Unreal mirror CI checks on each pairing
 * (native enumerator/field <-> Unreal mirror) is declared once
 *
 * @remarks Whenever a struct or enum in odin.h changes, update its mirror
 * in OdinNativeBlueprint.h AND the corresponding list below.
 */

#include "OdinCore/include/odin.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinNative/OdinUtils.h"

#include <type_traits>

// Enum ordinal parity mirror
// OdinNoiseSuppressionLevel <-> EOdinNoiseSuppression : X(NativeEnumerator, UEEnumerator)
#define ODIN_NOISE_SUPPRESSION_PAIRS(X)                                                                                                                        \
    X(ODIN_NOISE_SUPPRESSION_LEVEL_NONE, ODIN_NOISE_SUPPRESSION_NONE)                                                                                          \
    X(ODIN_NOISE_SUPPRESSION_LEVEL_LOW, ODIN_NOISE_SUPPRESSION_LOW)                                                                                            \
    X(ODIN_NOISE_SUPPRESSION_LEVEL_MODERATE, ODIN_NOISE_SUPPRESSION_MODERATE)                                                                                  \
    X(ODIN_NOISE_SUPPRESSION_LEVEL_HIGH, ODIN_NOISE_SUPPRESSION_HIGH)                                                                                          \
    X(ODIN_NOISE_SUPPRESSION_LEVEL_VERY_HIGH, ODIN_NOISE_SUPPRESSION_VERY_HIGH)

#define ODIN_ASSERT_NOISE_SUPPRESSION_PAIR(NativeName, UEName) static_assert(static_cast<int32>(EOdinNoiseSuppression::UEName) == NativeName);
ODIN_NOISE_SUPPRESSION_PAIRS(ODIN_ASSERT_NOISE_SUPPRESSION_PAIR)
#undef ODIN_ASSERT_NOISE_SUPPRESSION_PAIR

// OdinGainControllerVersion <-> EOdinGainControllerVersion
#define ODIN_GAIN_CONTROLLER_PAIRS(X)                                                                                                                          \
    X(ODIN_GAIN_CONTROLLER_VERSION_DISABLED, ODIN_GAIN_CONTROLLER_DISABLED)                                                                                    \
    X(ODIN_GAIN_CONTROLLER_VERSION_V1, ODIN_GAIN_CONTROLLER_V1)                                                                                                \
    X(ODIN_GAIN_CONTROLLER_VERSION_V2, ODIN_GAIN_CONTROLLER_V2)

#define ODIN_ASSERT_GAIN_CONTROLLER_PAIR(NativeName, UEName) static_assert(static_cast<int32>(EOdinGainControllerVersion::UEName) == NativeName);
ODIN_GAIN_CONTROLLER_PAIRS(ODIN_ASSERT_GAIN_CONTROLLER_PAIR)
#undef ODIN_ASSERT_GAIN_CONTROLLER_PAIR

// OdinEffectType <-> EOdinEffectType (native and UE enumerators same name)
#define ODIN_EFFECT_TYPE_PAIRS(X)                                                                                                                              \
    X(ODIN_EFFECT_TYPE_VAD)                                                                                                                                    \
    X(ODIN_EFFECT_TYPE_APM)                                                                                                                                    \
    X(ODIN_EFFECT_TYPE_VI)                                                                                                                                     \
    X(ODIN_EFFECT_TYPE_CUSTOM)

#define ODIN_ASSERT_EFFECT_TYPE(Name) static_assert(static_cast<int32>(EOdinEffectType::Name) == Name);
ODIN_EFFECT_TYPE_PAIRS(ODIN_ASSERT_EFFECT_TYPE)
#undef ODIN_ASSERT_EFFECT_TYPE

// ODIN_ERROR_SUCCESS deliberately kept at literal 0
static_assert(static_cast<int32>(EOdinError::ODIN_ERROR_SUCCESS) == ODIN_ERROR_SUCCESS);

// OdinError <-> EOdinError : X(Name) - same name, offset differ by OdinUtility::EODIN_ERROR_OFFSET
#define ODIN_ERROR_LIST(X)                                                                                                                                     \
    X(ODIN_ERROR_NO_DATA)                                                                                                                                      \
    X(ODIN_ERROR_INITIALIZATION_FAILED)                                                                                                                        \
    X(ODIN_ERROR_UNSUPPORTED_VERSION)                                                                                                                          \
    X(ODIN_ERROR_UNEXPECTED_STATE)                                                                                                                             \
    X(ODIN_ERROR_CLOSED)                                                                                                                                       \
    X(ODIN_ERROR_ALREADY_IN_USE)                                                                                                                               \
    X(ODIN_ERROR_ARGUMENT_NULL)                                                                                                                                \
    X(ODIN_ERROR_ARGUMENT_TOO_SMALL)                                                                                                                           \
    X(ODIN_ERROR_ARGUMENT_OUT_OF_BOUNDS)                                                                                                                       \
    X(ODIN_ERROR_ARGUMENT_INVALID_STRING)                                                                                                                      \
    X(ODIN_ERROR_ARGUMENT_INVALID_HANDLE)                                                                                                                      \
    X(ODIN_ERROR_ARGUMENT_INVALID_ID)                                                                                                                          \
    X(ODIN_ERROR_ARGUMENT_INVALID_JSON)                                                                                                                        \
    X(ODIN_ERROR_ARGUMENT_INVALID_CIPHER)                                                                                                                      \
    X(ODIN_ERROR_ARGUMENT_TOO_LARGE)                                                                                                                           \
    X(ODIN_ERROR_INVALID_VERSION)                                                                                                                              \
    X(ODIN_ERROR_INVALID_ACCESS_KEY)                                                                                                                           \
    X(ODIN_ERROR_INVALID_URI)                                                                                                                                  \
    X(ODIN_ERROR_INVALID_TOKEN)                                                                                                                                \
    X(ODIN_ERROR_INVALID_EFFECT)                                                                                                                               \
    X(ODIN_ERROR_INVALID_MSG_PACK)                                                                                                                             \
    X(ODIN_ERROR_INVALID_JSON)                                                                                                                                 \
    X(ODIN_ERROR_TOKEN_ROOM_REJECTED)                                                                                                                          \
    X(ODIN_ERROR_TOKEN_MISSING_CUSTOMER)                                                                                                                       \
    X(ODIN_ERROR_AUDIO_PROCESSING_FAILED)                                                                                                                      \
    X(ODIN_ERROR_AUDIO_CODEC_CREATION_FAILED)                                                                                                                  \
    X(ODIN_ERROR_AUDIO_ENCODING_FAILED)                                                                                                                        \
    X(ODIN_ERROR_AUDIO_DECODING_FAILED)                                                                                                                        \
    X(ODIN_ERROR_AUDIO_POSITION_LIMIT_REACHED)                                                                                                                 \
    X(ODIN_ERROR_AUDIO_VOICE_ISOLATION_FAILED)

#define ODIN_ASSERT_ERROR(Name) static_assert(static_cast<int32>(EOdinError::Name) == Name + OdinUtility::EODIN_ERROR_OFFSET);
ODIN_ERROR_LIST(ODIN_ASSERT_ERROR)
#undef ODIN_ASSERT_ERROR

// Struct field parity mirrors
// OdinSensitivityConfig <-> FOdinSensitivityConfig : X(NativeField, UEField, Type)
#define ODIN_SENSITIVITY_CONFIG_FIELDS(X)                                                                                                                      \
    X(enabled, Enabled, bool)                                                                                                                                  \
    X(attack_threshold, AttackThreshold, float)                                                                                                                \
    X(release_threshold, ReleaseThreshold, float)

#define ODIN_ASSERT_SENSITIVITY_FIELD(NativeField, UEField, Type)                                                                                              \
    static_assert(std::is_same_v<decltype(OdinSensitivityConfig::NativeField), Type>);                                                                         \
    static_assert(std::is_same_v<decltype(FOdinSensitivityConfig::UEField), Type>);
ODIN_SENSITIVITY_CONFIG_FIELDS(ODIN_ASSERT_SENSITIVITY_FIELD)
#undef ODIN_ASSERT_SENSITIVITY_FIELD

// OdinVadConfig <-> FOdinVadConfig : X(NativeField, UEField)
#define ODIN_VAD_CONFIG_FIELDS(X)                                                                                                                              \
    X(voice_activity, VoiceActivity)                                                                                                                           \
    X(volume_gate, VolumeGate)

#define ODIN_ASSERT_VAD_FIELD(NativeField, UEField)                                                                                                            \
    static_assert(std::is_same_v<decltype(OdinVadConfig::NativeField), OdinSensitivityConfig>);                                                                \
    static_assert(std::is_same_v<decltype(FOdinVadConfig::UEField), FOdinSensitivityConfig>);
ODIN_VAD_CONFIG_FIELDS(ODIN_ASSERT_VAD_FIELD)
#undef ODIN_ASSERT_VAD_FIELD

// OdinApmConfig <-> FOdinApmConfig : bool fields share their name and type
#define ODIN_APM_CONFIG_BOOL_FIELDS(X)                                                                                                                         \
    X(echo_canceller)                                                                                                                                          \
    X(high_pass_filter)                                                                                                                                        \
    X(transient_suppressor)

#define ODIN_ASSERT_APM_BOOL_FIELD(Field)                                                                                                                      \
    static_assert(std::is_same_v<decltype(OdinApmConfig::Field), bool>);                                                                                       \
    static_assert(std::is_same_v<decltype(FOdinApmConfig::Field), bool>);
ODIN_APM_CONFIG_BOOL_FIELDS(ODIN_ASSERT_APM_BOOL_FIELD)
#undef ODIN_ASSERT_APM_BOOL_FIELD

// X(NativeField, UEField, NativeType, UEType) : enum fields differ in both name and type between native and the TEnumAsByte mirror
#define ODIN_APM_CONFIG_ENUM_FIELDS(X)                                                                                                                         \
    X(noise_suppression_level, noise_suppression, OdinNoiseSuppressionLevel, TEnumAsByte<EOdinNoiseSuppression>)                                               \
    X(gain_controller_version, gain_controller, OdinGainControllerVersion, TEnumAsByte<EOdinGainControllerVersion>)

#define ODIN_ASSERT_APM_ENUM_FIELD(NativeField, UEField, NativeType, UEType)                                                                                   \
    static_assert(std::is_same_v<decltype(OdinApmConfig::NativeField), NativeType>);                                                                           \
    static_assert(std::is_same_v<decltype(FOdinApmConfig::UEField), UEType>);
ODIN_APM_CONFIG_ENUM_FIELDS(ODIN_ASSERT_APM_ENUM_FIELD)
#undef ODIN_ASSERT_APM_ENUM_FIELD

// OdinViConfig <-> FOdinViConfig : X(NativeField, UEField, Type)
#define ODIN_VI_CONFIG_FIELDS(X)                                                                                                                               \
    X(enabled, Enabled, bool)                                                                                                                                  \
    X(attenuation_limit_db, AttenuationLimitDb, float)

#define ODIN_ASSERT_VI_FIELD(NativeField, UEField, Type)                                                                                                       \
    static_assert(std::is_same_v<decltype(OdinViConfig::NativeField), Type>);                                                                                  \
    static_assert(std::is_same_v<decltype(FOdinViConfig::UEField), Type>);
ODIN_VI_CONFIG_FIELDS(ODIN_ASSERT_VI_FIELD)
#undef ODIN_ASSERT_VI_FIELD

static_assert(sizeof(OdinSensitivityConfig) <= 16, "OdinSensitivityConfig grew - check FOdinSensitivityConfig mirrors");
static_assert(sizeof(OdinVadConfig) <= 32, "OdinVadConfig grew - check FOdinVadConfig mirrors");
static_assert(sizeof(OdinApmConfig) <= 24, "OdinApmConfig grew - check FOdinApmConfig mirrors");
static_assert(sizeof(OdinViConfig) <= 16, "OdinViConfig grew - check FOdinViConfig mirrors");
