/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(Odin, Log, All);

#define ODIN_GENERAL_CLIENT_ERROR_CODE (7u << 29)

class ODIN_API FOdinModule : public IModuleInterface
{
  public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static void LogErrorCode(FString Prefix, uint32_t ErrorCode);

  private:
    void* OdinLibraryHandle = nullptr;
};
