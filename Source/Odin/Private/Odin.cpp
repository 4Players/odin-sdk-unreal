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
}

void FOdinModule::ShutdownModule()
{
#if PLATFORM_WINDOWS || PLATFORM_LINUX
    FPlatformProcess::FreeDllHandle(OdinLibraryHandle);
    OdinLibraryHandle = nullptr;
#endif
}

void FOdinModule::LogErrorCode(FString Prefix, uint32_t ErrorCode)
{
    FString ErrorMessage = UOdinFunctionLibrary::FormatError(ErrorCode, false);
    UE_LOG(Odin, Error, TEXT("%s, Error: %s"), *Prefix, *ErrorMessage);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOdinModule, Odin)
