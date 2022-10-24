/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

#include "OdinFunctionLibrary.h"

#include "OdinCaptureMedia.h"

#include <string>

static UOdinFunctionLibrary *g_odinFunctionLibrary = nullptr;

UOdinFunctionLibrary::UOdinFunctionLibrary(const class FObjectInitializer &PCIP)
    : Super(PCIP)
{
    g_odinFunctionLibrary = this;
}

UOdinFunctionLibrary *UOdinFunctionLibrary::getOdinFunctionLibrary()
{
    return g_odinFunctionLibrary;
}

UOdinCaptureMedia *UOdinFunctionLibrary::Odin_CreateMedia(UAudioCapture *audioCapture)
{
    auto capture_media = NewObject<UOdinCaptureMedia>();
    capture_media->SetAudioCapture(audioCapture);
    return capture_media;
}

FString UOdinFunctionLibrary::GenerateAccessKey()
{
    char buf[128] = {0};
    odin_access_key_generate(buf, sizeof(buf));
    return ANSI_TO_TCHAR(buf);
}

FString UOdinFunctionLibrary::FormatError(int32 code, bool ueTrace)
{
    if (ueTrace)
        FDebug::DumpStackTraceToLog(ELogVerbosity::All);

    std::string result;
    result.resize(128);
    auto r = odin_error_format(code, (char *)result.data(), result.size());
    while (r > result.size()) {
        r = odin_error_format(code, (char *)result.data(), result.size());
    };
    result.resize(r);
    return result.c_str();
}

FString UOdinFunctionLibrary::BytesToString(const TArray<uint8> &data)
{
    return ::BytesToString(data.GetData(), data.Num());
}
