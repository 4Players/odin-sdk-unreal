#pragma once

#include "Modules/ModuleManager.h"

class ODIN_API FOdinModule : public IModuleInterface
{
  public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

  private:
    void *OdinLibraryHandle;
};
