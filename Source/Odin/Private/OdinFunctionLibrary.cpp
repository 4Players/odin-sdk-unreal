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

UOdinCaptureMedia* UOdinFunctionLibrary::Odin_CreateMedia(UPARAM(ref) UAudioCapture*& audioCapture)
{
    if (!audioCapture)
        return nullptr;
    auto capture_media = NewObject<UOdinCaptureMedia>(audioCapture);
    capture_media->SetAudioCapture(audioCapture);
    return capture_media;
}

FString UOdinFunctionLibrary::GenerateAccessKey()
{
    char buf[128] = {0};
    odin_access_key_generate(buf, sizeof(buf));
    return ANSI_TO_TCHAR(buf);
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
    return FString(data.Num(), UTF8_TO_TCHAR(data.GetData()));
}

UOdinAudioCapture* UOdinFunctionLibrary::CreateOdinAudioCapture(UObject* WorldContextObject)
{
    UWorld* World = nullptr;
    if (IsValid(WorldContextObject)) {
        World = WorldContextObject->GetWorld();
    } else {
        UE_LOG(Odin, Warning,
               TEXT("No World Context provided when creating Odin Audio Capture. Audio Capture "
                    "will not be able to react to capture devices being removed."));
    }
    UOdinAudioCapture* OdinAudioCapture = NewObject<UOdinAudioCapture>(WorldContextObject);
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
