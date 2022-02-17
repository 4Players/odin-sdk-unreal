#include "OdinFunctionLibrary.h"

#include "OdinCaptureMedia.h"

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
