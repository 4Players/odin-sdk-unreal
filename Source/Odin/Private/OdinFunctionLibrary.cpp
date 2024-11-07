/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinFunctionLibrary.h"

#include "Odin.h"
#include "OdinAudioCapture.h"
#include "OdinCaptureMedia.h"

#include <string>

static UOdinFunctionLibrary* g_odinFunctionLibrary = nullptr;

UOdinFunctionLibrary::UOdinFunctionLibrary(const class FObjectInitializer& PCIP)
    : Super(PCIP)
{
    g_odinFunctionLibrary = this;
}

UOdinFunctionLibrary* UOdinFunctionLibrary::getOdinFunctionLibrary()
{
    return g_odinFunctionLibrary;
}

UOdinCaptureMedia* UOdinFunctionLibrary::Odin_CreateMedia(UPARAM(ref)
                                                              UAudioGenerator*& audioGenerator)
{
    if (!audioGenerator)
        return nullptr;
    auto capture_media = NewObject<UOdinCaptureMedia>(audioGenerator);
    capture_media->SetAudioGenerator(audioGenerator);
    return capture_media;
}

FString UOdinFunctionLibrary::GenerateAccessKey()
{
    char buf[128] = {0};
    odin_access_key_generate(buf, sizeof(buf));
#if ENGINE_MAJOR_VERSION >= 5
    return FString(128, buf);
#else
    return ANSI_TO_TCHAR(buf);
#endif
}

FString UOdinFunctionLibrary::FormatOdinError(int64 code, bool ueTrace)
{
    return FormatError(code, ueTrace);
}

FString UOdinFunctionLibrary::FormatError(int32 code, bool ueTrace)
{
    if (ueTrace) {
        FDebug::DumpStackTraceToLog(ELogVerbosity::All);
    }

    std::string result;
    result.resize(128);
    auto r = odin_error_format(code, (char*)result.data(), result.size());
    while (r > result.size()) {
        r = odin_error_format(code, (char*)result.data(), result.size());
    }
    result.resize(r);
    return result.c_str();
}

FString UOdinFunctionLibrary::BytesToString(const TArray<uint8>& data)
{
#if ENGINE_MAJOR_VERSION >= 5
    const UTF8CHAR* UTF8Char = reinterpret_cast<const UTF8CHAR*>(data.GetData());
    return FString(data.Num(), UTF8Char);
#else
    // ensure null terminated string is being returned
    if (data.Num() > 0 && data[data.Num() - 1] != TEXT('\0')) {
        TArray<uint8> NullTerminatedArray = data;
        NullTerminatedArray.Add(TEXT('\0'));
        return UTF8_TO_TCHAR(NullTerminatedArray.GetData());
    }
    return FString(data.Num(), UTF8_TO_TCHAR(data.GetData()));
#endif
}

UOdinAudioCapture* UOdinFunctionLibrary::CreateOdinAudioCapture(UObject* WorldContextObject)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinFunctionLibrary::CreateOdinAudioCapture)

    UOdinAudioCapture* OdinAudioCapture;
    if (IsValid(WorldContextObject)) {
        OdinAudioCapture = NewObject<UOdinAudioCapture>(WorldContextObject);
    } else {
        OdinAudioCapture = NewObject<UOdinAudioCapture>();
        UE_LOG(Odin, Warning,
               TEXT("No World Context provided when creating Odin Audio Capture. Audio Capture "
                    "will not be able to react to capture devices being removed."));
    }
    if (OdinAudioCapture->RestartCapturing(false)) {
        return OdinAudioCapture;
    }
    UE_LOG(Odin, Error, TEXT("Failed to open a default audio stream to the audio capture device."));
    return nullptr;
}

bool UOdinFunctionLibrary::Check(const TWeakObjectPtr<UObject> ObjectToCheck,
                                 const FString&                CheckReferenceName)
{
    if (!ObjectToCheck.IsValid()) {
        UE_LOG(Odin, Verbose, TEXT("Aborting %s due to invalid object ptr."), *CheckReferenceName);
        return false;
    }

    return true;
}
