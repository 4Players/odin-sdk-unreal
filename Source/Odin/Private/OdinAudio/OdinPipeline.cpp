/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinPipeline.h"

#include "AudioDevice.h"
#include "OdinVoice.h"
#include "OdinAudio/Effects/OdinActivityEffect.h"
#include "OdinAudio/Effects/OdinCloneEffect.h"
#include "OdinAudio/Effects/OdinMuteEffect.h"
#include "OdinAudio/Effects/OdinVolumeEffect.h"
#include "OdinAudio/Effects/OdinRmsEffect.h"

UOdinPipeline::UOdinPipeline(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
}

UOdinPipeline *UOdinPipeline::ConstructPipeline(UObject *WorldContextObject, const OdinPipeline *Handle)
{
    UOdinPipeline *result = NewObject<UOdinPipeline>(WorldContextObject);
    result->SetHandle(Handle);

    return result;
}

// effects
int32 UOdinPipeline::GetEffectCount() const
{ return odin_pipeline_get_effect_count(this->GetHandle()); }

int32 UOdinPipeline::GetEffectId(int32 Index) const
{
    uint32_t EffectId;
    auto     Result = odin_pipeline_get_effect_id(this->GetHandle(), Index, &EffectId);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return EffectId;
    } else {
        FOdinModule::LogErrorCode("Aborting GetEffectId due to invalid odin_pipeline_get_effect_id call: %s", Result);
    }

    return 0;
}

int32 UOdinPipeline::GetEffectIndex(int32 EffectId) const
{
    uint32_t   Index;
    const auto Ret = odin_pipeline_get_effect_index(this->GetHandle(), EffectId, &Index);
    if (Ret == OdinError::ODIN_ERROR_SUCCESS) {
        return Index;
    } else {
        FOdinModule::LogErrorCode("Aborting GetEffectIndex due to invalid odin_pipeline_get_effect_index call: %s", Ret);
    }

    return 0;
}

EOdinEffectType UOdinPipeline::GetEffectType(int32 EffectId) const
{
    OdinEffectType  EffectType;
    const OdinError Result = odin_pipeline_get_effect_type(this->GetHandle(), EffectId, &EffectType);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return static_cast<EOdinEffectType>(EffectType);
    } else {
        FOdinModule::LogErrorCode("Aborting GetEffectType due to invalid odin_pipeline_get_effect_type call: %s", Result);
    }

    return static_cast<EOdinEffectType>(0);
}

bool UOdinPipeline::MoveEffect(int32 EffectId, int32 Index)
{
    const OdinError Result = odin_pipeline_move_effect(this->GetHandle(), EffectId, Index);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return true;
    }

    FOdinModule::LogErrorCode("Aborting MoveEffect due to invalid odin_pipeline_move_effect call: %s", Result);
    return false;
}

bool UOdinPipeline::RemoveEffect(int32 EffectId)
{
    const OdinError Result = RemoveEffectWithCleanup(EffectId);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return true;
    }

    FOdinModule::LogErrorCode("Aborting RemoveEffect due to invalid odin_pipeline_remove_effect call: %s", Result);
    return false;
}

OdinError UOdinPipeline::RemoveEffectWithCleanup(int32 EffectId)
{
    const bool bWasApmEffect = GetEffectType(EffectId) == EOdinEffectType::ODIN_EFFECT_TYPE_APM;

    const OdinError Result = odin_pipeline_remove_effect(this->GetHandle(), EffectId);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        ApmPlaybackFormats.Remove(EffectId);
        if (bWasApmEffect && OnApmConfigChanged.IsBound()) {
            OnApmConfigChanged.Broadcast(this, EffectId, FOdinApmConfig{});
        }
    }
    return Result;
}

// custom effects
int32 UOdinPipeline::InsertCustomEffect(int32 Index, UOdinCustomEffect *Effect)
{
    uint32     ID     = 0;
    const auto Result = InsertCustomEffectNative(Index, Effect, ID);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return ID;
    }
    FOdinModule::LogErrorCode("Aborting InsertCustomEffect due to invalid "
                              "odin_pipeline_insert_custom_effect call: %s",
                              Result);
    return 0;
}

OdinError UOdinPipeline::InsertCustomEffectNative(int32 Index, UOdinCustomEffect *Effect, uint32 &OutEffectId)
{
    if (!IsValid(Effect)) {
        UE_LOG(Odin, Error, TEXT("Aborting InsertCustomEffect due to invalid UOdinCustomEffect pin."));
        return OdinError::ODIN_ERROR_ARGUMENT_NULL;
    }

    const auto Result = odin_pipeline_insert_custom_effect(this->GetHandle(), Index, Effect->FFICallback, &Effect->UserData, &OutEffectId);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        Effect->SetParent(this->Self);
        Effect->Index    = Index;
        Effect->EffectId = OutEffectId;
    }
    return Result;
}

// vad
int32 UOdinPipeline::InsertVadEffect(int32 Index)
{
    uint32_t   ID;
    const auto Result = odin_pipeline_insert_vad_effect(this->GetHandle(), Index, &ID);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return ID;
    } else {
        FOdinModule::LogErrorCode("Aborting InsertVadEffect due to invalid odin_pipeline_insert_vad_effect call: %s", Result);
    }
    return 0;
}

FOdinVadConfig UOdinPipeline::GetVadConfig(int32 EffectId) const
{
    OdinVadConfig config;
    auto          ret = odin_pipeline_get_vad_config(this->GetHandle(), EffectId, &config);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        return FOdinVadConfig{.VoiceActivity = FOdinSensitivityConfig{.Enabled          = config.voice_activity.enabled,
                                                                      .AttackThreshold  = config.voice_activity.attack_threshold,
                                                                      .ReleaseThreshold = config.voice_activity.release_threshold},
                              .VolumeGate    = FOdinSensitivityConfig{.Enabled          = config.volume_gate.enabled,
                                                                      .AttackThreshold  = config.volume_gate.attack_threshold,
                                                                      .ReleaseThreshold = config.volume_gate.release_threshold}};
    } else {
        FOdinModule::LogErrorCode("Aborting GetVadConfig due to invalid odin_pipeline_get_vad_config call: %s", ret);
    }
    return FOdinVadConfig{};
}

bool UOdinPipeline::SetVadConfig(int32 EffectId, FOdinVadConfig Config)
{
    OdinVadConfig cfg = Config;
    auto          ret = odin_pipeline_set_vad_config(this->GetHandle(), EffectId, &cfg);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        ODIN_LOG(Log, "Successfully changed VAD Configuration.");
        return true;
    }

    FOdinModule::LogErrorCode("Aborting SetVadConfig due to invalid odin_pipeline_set_vad_config call: %s", ret);
    return false;
}

// vi
int32 UOdinPipeline::InsertViEffect(int32 Index)
{
    uint32_t   ID;
    const auto Result = odin_pipeline_insert_vi_effect(this->GetHandle(), Index, &ID);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return ID;
    } else {
        FOdinModule::LogErrorCode("Aborting InsertViEffect due to invalid odin_pipeline_insert_vi_effect call: %s", Result);
    }
    return 0;
}

FOdinViConfig UOdinPipeline::GetViConfig(int32 EffectId) const
{
    OdinViConfig config;
    auto         ret = odin_pipeline_get_vi_config(this->GetHandle(), EffectId, &config);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        return FOdinViConfig{.Enabled = config.enabled, .AttenuationLimitDb = config.attenuation_limit_db};
    } else {
        FOdinModule::LogErrorCode("Aborting GetViConfig due to invalid odin_pipeline_get_vi_config call: %s", ret);
    }
    return FOdinViConfig{};
}

bool UOdinPipeline::SetViConfig(int32 EffectId, FOdinViConfig Config)
{
    OdinViConfig cfg = Config;
    auto         ret = odin_pipeline_set_vi_config(this->GetHandle(), EffectId, &cfg);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        ODIN_LOG(Log, "Successfully changed VI Configuration.");
        return true;
    }

    FOdinModule::LogErrorCode("Aborting SetViConfig due to invalid odin_pipeline_set_vi_config call: %s", ret);
    return false;
}

// apm
int32 UOdinPipeline::InsertApmEffect(int32 Index, int32 SampleRate, bool bStereo)
{
    if (SampleRate <= 0) {
        // default to current audio mixer sample rate, since the reverse stream is fed from the UE main submix
        SampleRate = 48000;
        if (FAudioDeviceManager *DeviceManager = FAudioDevice::GetAudioDeviceManager()) {
            if (FAudioDeviceHandle AudioDevice = DeviceManager->GetActiveAudioDevice(); AudioDevice.IsValid()) {
                SampleRate = static_cast<int32>(AudioDevice->GetSampleRate());
            }
        }
        ODIN_LOG(Log, "InsertApmEffect using audio mixer sample rate %d hz for the playback stream", SampleRate);
    }

    uint32_t id;
    auto     ret = odin_pipeline_insert_apm_effect(this->GetHandle(), Index, SampleRate, bStereo, &id);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        RegisterApmPlaybackFormat(id, SampleRate, bStereo);
        return id;
    } else {
        FOdinModule::LogErrorCode("Aborting InsertApmEffect due to invalid odin_pipeline_insert_apm_effect call: %s", ret);
    }
    return 0;
}

void UOdinPipeline::RegisterApmPlaybackFormat(int32 EffectId, int32 SampleRate, bool bStereo)
{ ApmPlaybackFormats.Add(EffectId, TPair<int32, bool>(SampleRate, bStereo)); }

bool UOdinPipeline::GetApmPlaybackFormat(int32 EffectId, int32 &OutSampleRate, bool &bOutStereo) const
{
    if (const TPair<int32, bool> *Format = ApmPlaybackFormats.Find(EffectId)) {
        OutSampleRate = Format->Key;
        bOutStereo    = Format->Value;
        return true;
    }
    return false;
}

bool UOdinPipeline::SetApmConfig(int32 EffectId, FOdinApmConfig Config)
{
    const auto Result = SetApmConfigNative(EffectId, Config);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return true;
    }

    FOdinModule::LogErrorCode("Aborting SetApmConfig due to invalid odin_pipeline_set_apm_config call: %s", Result);
    return false;
}

OdinError UOdinPipeline::SetApmConfigNative(int32 EffectId, FOdinApmConfig Config)
{
    OdinApmConfig NativeConfig = Config;
    const auto    Result       = odin_pipeline_set_apm_config(this->GetHandle(), EffectId, &NativeConfig);
    if (Result == OdinError::ODIN_ERROR_SUCCESS && OnApmConfigChanged.IsBound()) {
        OnApmConfigChanged.Broadcast(this, EffectId, Config);
    }
    return Result;
}

FOdinApmConfig UOdinPipeline::GetApmConfig(int32 EffectId) const
{
    OdinApmConfig   Config;
    const OdinError Result = odin_pipeline_get_apm_config(GetHandle(), EffectId, &Config);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return FOdinApmConfig{.echo_canceller       = Config.echo_canceller,
                              .high_pass_filter     = Config.high_pass_filter,
                              .transient_suppressor = Config.transient_suppressor,
                              .noise_suppression    = static_cast<EOdinNoiseSuppression>(Config.noise_suppression_level),
                              .gain_controller      = static_cast<EOdinGainControllerVersion>(Config.gain_controller_version)};
    } else {
        FOdinModule::LogErrorCode("Aborting GetVadConfig due to invalid odin_pipeline_get_vad_config call: %s", Result);
    }
    return FOdinApmConfig{};
}

bool UOdinPipeline::UpdateApmPlayback(int32 EffectId, const float *Samples, int32 Count, int32 Delay)
{
    auto Result = odin_pipeline_update_apm_playback(this->GetHandle(), EffectId, Samples, Count, Delay);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return true;
    }

    FOdinModule::LogErrorCode("Aborting UpdateApmPlayback due to invalid odin_pipeline_update_apm_playback call: %s", Result);
    return false;
}

bool UOdinPipeline::UpdateApmPlayback(int32 EffectId, TArray<float> &Samples, int32 Delay)
{ return UpdateApmPlayback(EffectId, Samples.GetData(), Samples.Num(), Delay); }

// convenience effects
UOdinActivityEffect *UOdinPipeline::CreateActivityEffect(int32 Index)
{
    UOdinActivityEffect *ActivityEffect = UOdinActivityEffect::ConstructActivityEffect(this);
    if (InsertCustomEffect(Index, ActivityEffect))
        return ActivityEffect;

    return nullptr;
}

UOdinActivityEffect *UOdinPipeline::AppendActivityEffect()
{ return CreateActivityEffect(GetEffectCount()); }

UOdinCloneEffect *UOdinPipeline::CreateCloneEffect(int32 Index)
{
    UOdinCloneEffect *CloneEffect = UOdinCloneEffect::ConstructCloneEffect(this);
    if (InsertCustomEffect(Index, CloneEffect))
        return CloneEffect;

    return nullptr;
}

UOdinCloneEffect *UOdinPipeline::AppendCloneEffect()
{ return CreateCloneEffect(GetEffectCount()); }

UOdinMuteEffect *UOdinPipeline::CreateMuteEffect(int32 Index)
{
    UOdinMuteEffect *MuteEffect = UOdinMuteEffect::ConstructMuteEffect(this, EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_UNCHANGED);
    if (InsertCustomEffect(Index, MuteEffect))
        return MuteEffect;

    return nullptr;
}

UOdinMuteEffect *UOdinPipeline::AppendMuteEffect()
{ return CreateMuteEffect(GetEffectCount()); }

UOdinVolumeEffect *UOdinPipeline::CreateVolumeEffect(int32 Index)
{
    UOdinVolumeEffect *VolumeEffect = UOdinVolumeEffect::ConstructVolumeEffect(this);
    if (InsertCustomEffect(Index, VolumeEffect))
        return VolumeEffect;

    return nullptr;
}

UOdinVolumeEffect *UOdinPipeline::AppendVolumeEffect()
{ return CreateVolumeEffect(GetEffectCount()); }

UOdinRmsEffect *UOdinPipeline::CreateRmsEffect(int32 Index)
{
    UOdinRmsEffect *RmsEffect = UOdinRmsEffect::ConstructRmsEffect(this);
    if (InsertCustomEffect(Index, RmsEffect))
        return RmsEffect;

    return nullptr;
}

UOdinRmsEffect *UOdinPipeline::AppendRmsEffect()
{ return CreateRmsEffect(GetEffectCount()); }

void UOdinPipeline::SetHandle(const OdinPipeline *NewHandle)
{
    this->Handle = NewObject<UOdinHandle>();
    this->Handle->SetHandle(const_cast<OdinPipeline *>(NewHandle));
}
