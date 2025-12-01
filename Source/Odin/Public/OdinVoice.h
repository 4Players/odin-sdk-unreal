/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(Odin, Log, All);

#define ODIN_LOG(Verbosity, Format, ...)                                                                                                                       \
    UE_LOG(Odin, Verbosity, TEXT("ODIN_API: " #Format "; Location %s:%s@%d"), ##__VA_ARGS__, ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(__FILE__), __LINE__)

class ODIN_API FOdinModule : public IModuleInterface
{
  public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void LoadExtension(FString& libraryPath, FString libraryExtensionName);

    static void LogErrorCode(const char* Prefix, int32 ErrorCode);
    static void LogErrorCode(const char* Prefix, uint8 ErrorCode);
    static void Debug(FString platform, FString path, FString name);

  private:
    bool                 DBG               = false;
    void*                OdinLibraryHandle = nullptr;
    TMap<FString, void*> OdinLibraryExtensionHandles;
};