#include "Odin.h"
#include "Core.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"

#include "OdinLibrary/include/odin.h"

#define LOCTEXT_NAMESPACE "FOdinModule"

void FOdinModule::StartupModule()
{
    FString BaseDir = IPluginManager::Get().FindPlugin("Odin")->GetBaseDir();

    FString LibraryPath;
#if PLATFORM_WINDOWS
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/OdinLibrary/x64/Win/odin.dll"));
#elif PLATFORM_MAC
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("$(PluginDir)/TODO"));
#elif PLATFORM_LINUX
    LibraryPath =
        FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/OdinLibrary/x64/Linux/libodin.so"));
#endif // PLATFORM_WINDOWS

    OdinLibraryHandle =
        !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

    odin_startup(ODIN_VERSION);
}

void FOdinModule::ShutdownModule()
{
    odin_shutdown();

    FPlatformProcess::FreeDllHandle(OdinLibraryHandle);
    OdinLibraryHandle = nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FOdinModule, Odin)
