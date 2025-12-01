/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinVoice.h"
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

#include "OdinCore/include/odin.h"
#include "OdinFunctionLibrary.h"
#include "OdinNative/OdinUtils.h"

#define LOCTEXT_NAMESPACE "FOdinModule"

#ifndef UE_ODIN_DEBUG
#define UE_ODIN_DEBUG 0
#endif // !UE_ODIN_DEBUG

DEFINE_LOG_CATEGORY(Odin)

void FOdinModule::StartupModule()
{
#if PLATFORM_IOS
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::Playback Active:true];
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::Record Active:true];
    [[IOSAppDelegate GetDelegate] SetFeature:EAudioFeature::VoiceChat Active:true];
#endif

#ifndef PLATFORM_PS5
#define PLATFORM_PS5 0
#endif // !PLATFORM_PS5

#if PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_PS5
    FString         BaseDir = IPluginManager::Get().FindPlugin("Odin")->GetBaseDir();
    FString         LibraryPath;
    FString         LibraryName;
    TArray<FString> LibraryExtensionNames;
    FString         PlatformArchitecture;
#if PLATFORM_CPU_X86_FAMILY
    PlatformArchitecture = "x64";
#elif PLATFORM_CPU_ARM_FAMILY
    PlatformArchitecture = "arm64";
#endif

#if PLATFORM_WINDOWS
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/OdinCore"), PlatformArchitecture, TEXT("Win"));
    LibraryName = "odin.dll";
    LibraryExtensionNames.Add("odin_crypto.dll");
#elif PLATFORM_LINUX
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/OdinCore"), PlatformArchitecture, TEXT("Linux"));
    LibraryName = "libodin.so";
    LibraryExtensionNames.Add("libodin_crypto.so");
#elif PLATFORM_PS5
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/OdinCore"), PlatformArchitecture, TEXT("PS5"));
    LibraryName = "libodin.prx";
    LibraryExtensionNames.Add("libodin_crypto.prx");
#endif

#if WITH_EDITOR
    Debug(PlatformArchitecture, LibraryPath, LibraryName);
#endif

    FPlatformProcess::PushDllDirectory(*LibraryPath);
    OdinLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*(LibraryPath / LibraryName)) : nullptr;

    if (OdinLibraryHandle == nullptr) {
        ODIN_LOG(Error, "Failed to load library (%s)", *(LibraryPath / LibraryName));
    } else {
        ODIN_LOG(Log, "Loaded Library (%s)", *(LibraryPath / LibraryName));

#if ODIN_USE_EXTENSIONS
        for (FString extension : LibraryExtensionNames)
            LoadExtension(LibraryPath, extension); // silent fail
#endif
    }
    FPlatformProcess::PopDllDirectory(*LibraryPath);
#endif

    odin_initialize(ODIN_VERSION);
}

void FOdinModule::Debug(FString platform, FString path, FString name)
{
    auto DBG = FGenericPlatformMisc::GetEnvironmentVariable(TEXT("UE_ODIN_DEBUG")).Equals(FString(TEXT("true")));
    if (DBG) {
        // native debug
        FGenericPlatformMisc::SetEnvironmentVar(TEXT("ODIN_DEBUG"), TEXT("true"));
        FGenericPlatformMisc::SetEnvironmentVar(TEXT("ODIN_DEBUG_SERVER"), TEXT("127.0.0.1"));

        ODIN_LOG(Warning, "UE_ODIN_DEBUG is set %s %s %s v%s", *platform, *path, *name, *FString(ODIN_VERSION));
    }
}

void FOdinModule::ShutdownModule()
{
    odin_shutdown();

#if PLATFORM_WINDOWS || PLATFORM_LINUX
#if ODIN_USE_EXTENSIONS
    if (OdinLibraryExtensionHandles.Num() > 0) {
        for (auto kvp : OdinLibraryExtensionHandles)
            FPlatformProcess::FreeDllHandle(kvp.Value);

        OdinLibraryExtensionHandles.Empty();
    }
#endif

    FPlatformProcess::FreeDllHandle(OdinLibraryHandle);
    OdinLibraryHandle = nullptr;
#endif
}

void FOdinModule::LoadExtension(FString& libraryPath, FString libraryExtensionName)
{
    auto extensionHandle = !libraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*(libraryPath / libraryExtensionName)) : nullptr;

    if (extensionHandle != nullptr)
        OdinLibraryExtensionHandles.Add(libraryExtensionName, extensionHandle);
}

/// <summary>
/// Log stack with SDK on ffi call error
/// </summary>
void FOdinModule::LogErrorCode(const char* prefix, int32 errorCode)
{
    FString ErrorMessage = UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(errorCode + OdinUtility::EODIN_ERROR_OFFSET), UE_ODIN_DEBUG);
    ODIN_LOG(Error, "%hs, Error: %s", prefix, *ErrorMessage);
    FDebug::DumpStackTraceToLog(TEXT("Odin SDK Stack (FFI)"), ELogVerbosity::Error);
}
/// <summary>
/// Log stack with SDK error
/// </summary>
void FOdinModule::LogErrorCode(const char* prefix, uint8 errorCode)
{
    FString ErrorMessage = UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(errorCode), UE_ODIN_DEBUG);
    ODIN_LOG(Error, "%hs, Error: %s", prefix, *ErrorMessage);
    FDebug::DumpStackTraceToLog(TEXT("Odin SDK Stack"), ELogVerbosity::Warning);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FOdinModule, Odin)