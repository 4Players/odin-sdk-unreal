/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "Odin.h"
#include "CoreMinimal.h"
#include "Features/IModularFeatures.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_IOS || PLATFORM_LINUX
#include "HAL/PlatformProcess.h"
#endif

#if PLATFORM_IOS
#include "IOSAppDelegate.h"
#endif

#include "OdinFunctionLibrary.h"
#include "Sound/AudioSettings.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "FOdinModule"
#ifndef PLATFORM_PS5
#define PLATFORM_PS5 0
#endif
DEFINE_LOG_CATEGORY(Odin)

void FOdinModule::StartupModule()
{
#if PLATFORM_IOS
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::Playback Active:true];
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::Record Active:true];
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::VoiceChat Active:true];
#endif

#if PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_PS5
    FString BaseDir = IPluginManager::Get().FindPlugin("Odin")->GetBaseDir();
    FString LibraryPath;
    FString libraryName;

    FString PlatformArchitecture;
#if PLATFORM_CPU_X86_FAMILY
    PlatformArchitecture = "x64";
#elif PLATFORM_CPU_ARM_FAMILY
    PlatformArchitecture = "arm64";
#endif

#if PLATFORM_WINDOWS
    LibraryPath =
        FPaths::Combine(*BaseDir, TEXT("Source/OdinCore"), PlatformArchitecture, TEXT("Win"));
    libraryName = "odin.dll";
#elif PLATFORM_LINUX
    LibraryPath =
        FPaths::Combine(*BaseDir, TEXT("Source/OdinCore"), PlatformArchitecture, TEXT("Linux"));
    libraryName = "libodin.so";
#elif PLATFORM_PS5
    // packaging will copy prx to "prx/libodin.prx"
    libraryName = "libodin.prx";
#endif

#if PLATFORM_PS5
    // FSonyPlatformProcess will try "libraryName" and "/app0/prx/libraryName"
    OdinLibraryHandle = FPlatformProcess::GetDllHandle(*libraryName);
    if (OdinLibraryHandle == nullptr) {
        UE_LOG(Odin, Error, TEXT("Failed to load library (%s)"), *libraryName);
    } else {
        UE_LOG(Odin, Log, TEXT("Loaded Library (%s)"), *libraryName);
    }
#else
    FPlatformProcess::PushDllDirectory(*LibraryPath);
    OdinLibraryHandle = !LibraryPath.IsEmpty()
                            ? FPlatformProcess::GetDllHandle(*(LibraryPath / libraryName))
                            : nullptr;
    FPlatformProcess::PopDllDirectory(*LibraryPath);

    if (OdinLibraryHandle == nullptr) {
        UE_LOG(Odin, Error, TEXT("Failed to load library (%s)"), *(LibraryPath / libraryName));
    } else {
        UE_LOG(Odin, Log, TEXT("Loaded Library (%s)"), *(LibraryPath / libraryName));
    }
#endif
#endif

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
    FString OdinConfigPath = FConfigCacheIni::NormalizeConfigIniPath(
        FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("Odin/Config/Odin.ini")));
#else
    FString OdinConfigPath =
        FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("Odin/Config/Odin.ini"));
#endif
    if (FPaths::FileExists(OdinConfigPath)) {
        GConfig->LoadFile(OdinConfigPath);
    }

    TWeakObjectPtr<UAudioSettings> AudioSettings = GetMutableDefault<UAudioSettings>();
    if (GConfig && AudioSettings.IsValid()) {
        // only set this, if not changed from default by user
        float EngineMinPitchScale = -1.0f;
        GConfig->GetFloat(TEXT("/Script/Engine.AudioSettings"), TEXT("GlobalMinPitchScale"),
                          EngineMinPitchScale, GEngineIni);
        float MinPitch = 0.1f;
        GConfig->GetFloat(TEXT("Odin"), TEXT("GlobalMinPitch"), MinPitch, OdinConfigPath);
        if (EngineMinPitchScale < 0.0f && AudioSettings->GlobalMinPitchScale > MinPitch)
            AudioSettings->GlobalMinPitchScale = MinPitch;

        // only set this, if not changed from default by user
        float EngineMaxPitchScale = -1.0f;
        GConfig->GetFloat(TEXT("/Script/Engine.AudioSettings"), TEXT("GlobalMaxPitchScale"),
                          EngineMaxPitchScale, GEngineIni);
        float MaxPitch = 4.0f;
        GConfig->GetFloat(TEXT("Odin"), TEXT("GlobalMaxPitch"), MaxPitch, OdinConfigPath);
        if (EngineMaxPitchScale < 0.0f && AudioSettings->GlobalMaxPitchScale < MaxPitch)
            AudioSettings->GlobalMaxPitchScale = MaxPitch;
    }
}

void FOdinModule::ShutdownModule()
{
#if PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_PS5
    FPlatformProcess::FreeDllHandle(OdinLibraryHandle);
    OdinLibraryHandle = nullptr;
#endif
}

void FOdinModule::LogErrorCode(FString Prefix, uint32_t ErrorCode)
{
    FString ErrorMessage = UOdinFunctionLibrary::FormatError(ErrorCode, false);
    UE_LOG(Odin, Error, TEXT("%s, Error: %s"), *Prefix, *ErrorMessage);
}

void FOdinModule::LogReturnCode(FString Prefix, uint32_t ReturnCode)
{
    FString Message = UOdinFunctionLibrary::FormatError(ReturnCode, false);
    UE_LOG(Odin, Display, TEXT("%s Reason: %s"), *Prefix, *Message);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOdinModule, Odin)