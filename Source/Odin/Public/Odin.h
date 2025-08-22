/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(Odin, Log, All);

#define ODIN_GENERAL_CLIENT_ERROR_CODE (7u << 29)
#define ODIN_DEFAULT_SAMPLE_RATE 48000
#define ODIN_DEFAULT_CHANNEL_COUNT 2

class ODIN_API FOdinModule : public IModuleInterface
{
  public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /**
     * Log an error code with a specified prefix.
     *
     * @param Prefix The prefix to be included in the log message.
     * @param ErrorCode The Odin error code that should be formatted and logged.
     */
    static void LogErrorCode(FString Prefix, uint32_t ErrorCode);

    /**
     * Log an Odin Return Code with a specified prefix. Same as LogErrorCode, but with Display
     * Verbosity.
     * @param Prefix The prefix to be included in the log message.
     * @param ReturnCode The Odin code that should be formatted and logged.
     */
    static void LogReturnCode(FString Prefix, uint32_t ReturnCode);

  private:
    void* OdinLibraryHandle = nullptr;
};