/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinPipeline.h"

#include "OdinVoice.h"
#include "OdinAudio/Effects/OdinActivityEffect.h"
#include "OdinAudio/Effects/OdinCloneEffect.h"
#include "OdinAudio/Effects/OdinMuteEffect.h"
#include "OdinAudio/Effects/OdinVolumeEffect.h"

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
{
    return odin_pipeline_get_effect_count(this->GetHandle());
}

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
    const OdinError Result = odin_pipeline_remove_effect(this->GetHandle(), EffectId);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        return true;
    }

    FOdinModule::LogErrorCode("Aborting RemoveEffect due to invalid odin_pipeline_remove_effect call: %s", Result);
    return false;
}

// custom effects
int32 UOdinPipeline::InsertCustomEffect(int32 Index, UOdinCustomEffect *Effect)
{
    if (!IsValid(Effect)) {
        UE_LOG(Odin, Error, TEXT("Aborting InsertCustomEffect due to invalid UOdinCustomEffect pin."));
        return 0;
    }

    uint32_t   ID;
    const auto Result = odin_pipeline_insert_custom_effect(this->GetHandle(), Index, Effect->FFICallback, &Effect->UserData, &ID);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        Effect->SetParent(this->Self);
        Effect->Index    = Index;
        Effect->EffectId = ID;
        return ID;
    } else {
        FOdinModule::LogErrorCode("Aborting InsertCustomEffect due to invalid "
                                  "odin_pipeline_insert_custom_effect call: %s",
                                  Result);
    }
    return 0;
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
    if (ret == OdinError::ODIN_ERROR_SUCCESS)
        return true;

    FOdinModule::LogErrorCode("Aborting SetVadConfig due to invalid odin_pipeline_set_vad_config call: %s", ret);
    return false;
}

// apm
int32 UOdinPipeline::InsertApmEffect(int32 Index, int32 SampleRate, bool bStereo)
{
    uint32_t id;
    auto     ret = odin_pipeline_insert_apm_effect(this->GetHandle(), Index, SampleRate, bStereo, &id);
    if (ret == OdinError::ODIN_ERROR_SUCCESS) {
        return id;
    } else {
        FOdinModule::LogErrorCode("Aborting InsertApmEffect due to invalid odin_pipeline_insert_apm_effect call: %s", ret);
    }
    return 0;
}

bool UOdinPipeline::SetApmConfig(int32 EffectId, FOdinApmConfig Config)
{
    OdinApmConfig NativeConfig = Config;
    auto          Result       = odin_pipeline_set_apm_config(this->GetHandle(), EffectId, &NativeConfig);
    if (Result == OdinError::ODIN_ERROR_SUCCESS) {
        if (OnApmConfigChanged.IsBound()) {
            OnApmConfigChanged.Broadcast(this, EffectId, Config);
        }
        return true;
    }

    FOdinModule::LogErrorCode("Aborting SetApmConfig due to invalid odin_pipeline_set_apm_config call: %s", Result);
    return false;
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
{
    return UpdateApmPlayback(EffectId, Samples.GetData(), Samples.Num(), Delay);
}

// convenience effects
UOdinActivityEffect *UOdinPipeline::CreateActivityEffect(int32 Index)
{
    UOdinActivityEffect *ActivityEffect = UOdinActivityEffect::ConstructActivityEffect(this);
    if (InsertCustomEffect(Index, ActivityEffect))
        return ActivityEffect;

    return nullptr;
}

UOdinActivityEffect *UOdinPipeline::AppendActivityEffect()
{
    return CreateActivityEffect(GetEffectCount());
}

UOdinCloneEffect *UOdinPipeline::CreateCloneEffect(int32 Index)
{
    UOdinCloneEffect *CloneEffect = UOdinCloneEffect::ConstructCloneEffect(this);
    if (InsertCustomEffect(Index, CloneEffect))
        return CloneEffect;

    return nullptr;
}

UOdinCloneEffect *UOdinPipeline::AppendCloneEffect()
{
    return CreateCloneEffect(GetEffectCount());
}

UOdinMuteEffect *UOdinPipeline::CreateMuteEffect(int32 Index)
{
    UOdinMuteEffect *MuteEffect = UOdinMuteEffect::ConstructMuteEffect(this, EOdinMuteEffectOptions::ODIN_EFFECT_TOGGLE_UNCHANGED);
    if (InsertCustomEffect(Index, MuteEffect))
        return MuteEffect;

    return nullptr;
}

UOdinMuteEffect *UOdinPipeline::AppendMuteEffect()
{
    return CreateMuteEffect(GetEffectCount());
}

UOdinVolumeEffect *UOdinPipeline::CreateVolumeEffect(int32 Index)
{
    UOdinVolumeEffect *VolumeEffect = UOdinVolumeEffect::ConstructVolumeEffect(this);
    if (InsertCustomEffect(Index, VolumeEffect))
        return VolumeEffect;

    return nullptr;
}

UOdinVolumeEffect *UOdinPipeline::AppendVolumeEffect()
{
    return CreateVolumeEffect(GetEffectCount());
}

void UOdinPipeline::SetHandle(const OdinPipeline *NewHandle)
{
    this->Handle = NewObject<UOdinHandle>();
    this->Handle->SetHandle(const_cast<OdinPipeline *>(NewHandle));
}
