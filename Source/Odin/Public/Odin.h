/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(Odin, Log, All);

class ODIN_API FOdinModule : public IModuleInterface
{
  public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

  private:
    void* OdinLibraryHandle = nullptr;
};
