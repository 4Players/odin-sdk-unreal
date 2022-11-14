/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#include "Odin.h"
#include "CoreMinimal.h"
#include "Features/IModularFeatures.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX
#include "HAL/PlatformProcess.h"
#endif

#include "OdinCore/include/odin.h"

#define LOCTEXT_NAMESPACE "FOdinModule"

DEFINE_LOG_CATEGORY(Odin)

void FOdinModule::StartupModule()
{
#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX
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
#elif PLATFORM_MAC
    LibraryPath =
        FPaths::Combine(*BaseDir, TEXT("Source/OdinCore"), PlatformArchitecture, TEXT("Mac"));
    libraryName = "libodin.dylib";
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
    odin_startup(ODIN_VERSION);
}

void FOdinModule::ShutdownModule()
{
    odin_shutdown();

#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX
    FPlatformProcess::FreeDllHandle(OdinLibraryHandle);
    OdinLibraryHandle = nullptr;
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FOdinModule, Odin)
