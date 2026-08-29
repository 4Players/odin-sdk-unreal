/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinVoice.h"
#include "CoreMinimal.h"
#include "Features/IModularFeatures.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/EngineVersion.h"
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

#ifndef ODIN_RUN_ON_SERVER
#define ODIN_RUN_ON_SERVER 0
#endif // !ODIN_RUN_ON_SERVER

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

#ifndef PLATFORM_XSX
#define PLATFORM_XSX 0
#endif // !PLATFORM_XSX

    FString LibraryPath;
    FString LibraryName;
    FString PlatformArchitecture;
#if PLATFORM_CPU_X86_FAMILY
    PlatformArchitecture = "x64";
#elif PLATFORM_CPU_ARM_FAMILY
    PlatformArchitecture = "arm64";
#endif

#if PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_PS5 || PLATFORM_XSX
    FString         BaseDir = IPluginManager::Get().FindPlugin("Odin")->GetBaseDir();
    TArray<FString> LibraryExtensionNames;

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
#elif PLATFORM_XSX
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/OdinCore"), PlatformArchitecture, TEXT("XSX"));
    LibraryName = "odin.dll";
    LibraryExtensionNames.Add("odin_crypto.dll");
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

#if UE_SERVER && !ODIN_RUN_ON_SERVER
    return;
#else
    enum OdinError InitializationResult = odin_initialize(ODIN_VERSION);
    bIsInitialized                      = ODIN_ERROR_SUCCESS == InitializationResult;

    Debug(PlatformArchitecture, LibraryPath, LibraryName, InitializationResult);
#endif
}

void FOdinModule::Debug(FString platform, FString path, FString name, int initializationResult)
{
    const FString             dbgnl  = FGenericPlatformMisc::GetEnvironmentVariable(TEXT("UE_ODIN_NATIVE"));
    const FString             uev    = FEngineVersion::Current().ToString();
    const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("odin"));
    const FString             sdkv   = plugin.IsValid() ? plugin->GetDescriptor().VersionName : "2";

    ODIN_LOG(Display, "UE_ODIN_NATIVE ue%s sdk%s %s %s %s -l%s v%s r%d", *uev, *sdkv, *platform, *path, *name, *dbgnl, *FString(ODIN_VERSION),
             initializationResult);
    if (initializationResult != 0)
        return;

    DBGV     = FCString::Atoi(*dbgnl);
    auto ret = odin_debug_set_logging_hook(DBGV, LogHook);
    if (ret != OdinError::ODIN_ERROR_SUCCESS) {
        ODIN_LOG(Warning, "UE_ODIN_NATIVE %d", static_cast<int32_t>(ret));
    }
}

void FOdinModule::LogHook(const char* S)
{
    if (DBGV > 0) {
        UE_LOG(Odin, VeryVerbose, TEXT("ODIN_NATIVE %s"), UTF8_TO_TCHAR(S));
    } else {
        UE_LOG(Odin, Warning, TEXT("ODIN_NATIVE %s"), UTF8_TO_TCHAR(S));
    }
}

FString FOdinModule::Dump(uint32 Size)
{
    TArray<char> KeyChars;
    KeyChars.SetNumZeroed(Size);
    uint32_t len = sizeof(char) * Size;

    UE_LOG(Odin, VeryVerbose, TEXT("FOdinModule::Dump - Set len to %ud"), len);
    auto ret = odin_debug_dump_state(KeyChars.GetData(), &len);
    if (ret != OdinError::ODIN_ERROR_SUCCESS) {
        ODIN_LOG(Warning, "odin_debug_dump_state silent error %d", static_cast<int32_t>(ret));
    }
    UE_LOG(Odin, VeryVerbose, TEXT("FOdinModule::Dump - len after dumping is %ud"), len);

    return FString(StringCast<UTF8CHAR>(reinterpret_cast<UTF8CHAR*>(KeyChars.GetData()), len));
}

void FOdinModule::ShutdownModule()
{
    if (bIsInitialized) {
        odin_shutdown();
    }

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
void FOdinModule::LogErrorCode(const char* prefix, int32 errorCode, bool TraceAll)
{
    FString ErrorMessage = UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(errorCode + OdinUtility::EODIN_ERROR_OFFSET), TraceAll);
    ODIN_LOG(Error, "%hs, Error: %s", prefix, *ErrorMessage);
    FDebug::DumpStackTraceToLog(TEXT("Odin SDK Stack (FFI)"), ELogVerbosity::Error);
}
/// <summary>
/// Log stack with SDK error
/// </summary>
void FOdinModule::LogErrorCode(const char* prefix, uint8 errorCode, bool TraceAll)
{
    FString ErrorMessage = UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(errorCode), TraceAll);
    ODIN_LOG(Error, "%hs, Error: %s", prefix, *ErrorMessage);
    FDebug::DumpStackTraceToLog(TEXT("Odin SDK Stack"), ELogVerbosity::Warning);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FOdinModule, Odin)