/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

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

#include "odin_sdk.h"

#define LOCTEXT_NAMESPACE "FOdinModule"

DEFINE_LOG_CATEGORY(Odin)

void FOdinModule::StartupModule()
{
#if PLATFORM_IOS
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::Playback Active:true];
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::Record Active:true];
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::VoiceChat Active:true];
#endif

#if PLATFORM_WINDOWS || PLATFORM_LINUX
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
#endif

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

    auto sample_rate   = 48000;
    auto channel_count = 2;

    odin_startup_ex(ODIN_VERSION,
                    OdinAudioStreamConfig{(uint32_t)sample_rate, (uint8_t)channel_count});
}

void FOdinModule::ShutdownModule()
{
    odin_shutdown();

#if PLATFORM_WINDOWS || PLATFORM_LINUX
    FPlatformProcess::FreeDllHandle(OdinLibraryHandle);
    OdinLibraryHandle = nullptr;
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FOdinModule, Odin)
