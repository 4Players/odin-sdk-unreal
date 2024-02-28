/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(Odin, Log, All);

class ODIN_API FOdinModule : public IModuleInterface
{
  public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    int32 GetSampleRate() const
    {
        return SampleRate;
    }
    int32 GetChannelCount() const
    {
        return ChannelCount;
    }

  private:
    void *OdinLibraryHandle;

    int32 SampleRate   = 48000;
    int32 ChannelCount = 2;
};
