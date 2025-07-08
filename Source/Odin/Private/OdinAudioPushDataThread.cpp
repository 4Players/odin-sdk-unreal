#include "OdinAudioPushDataThread.h"
#include "HAL/RunnableThread.h"
#include "GenericPlatform/GenericPlatformAffinity.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Odin.h"
#include "OdinFunctionLibrary.h"
#include "HAL/PlatformProcess.h"

FOdinAudioPushDataThread::FOdinAudioPushDataThread()
{
    Thread =
        FRunnableThread::Create(this, TEXT("Odin Audio Push Data Thread"), 0, TPri_TimeCritical);
}

FOdinAudioPushDataThread::~FOdinAudioPushDataThread()
{
    if (Thread) {
        Thread->Kill();
        delete Thread;
    }
}

void FOdinAudioPushDataThread::PushAudio(OdinMediaStreamHandle MediaHandle, const float* AudioData,
                                         int32 NumSamples)
{
    if (NumSamples > 0) {
        FOdinAudioPushData NewPushData{MediaHandle, TArray<float>(AudioData, NumSamples)};
        PushDataQueue.Enqueue(MoveTemp(NewPushData));
    }
}

void FOdinAudioPushDataThread::RequestShutdown()
{
    bShutdown = true;
}

bool FOdinAudioPushDataThread::Init()
{
    UE_LOG(Odin, Verbose, TEXT("FOdinAudioPushDataThread::Init()"))
    return true;
}

uint32 FOdinAudioPushDataThread::Run()
{
    while (!bShutdown) {
        {
            TRACE_CPUPROFILER_EVENT_SCOPE(
                FOdinAudioPushDataThread::Run : odin_audio_push_data calls)
            FOdinAudioPushData DequeuedData;
            while (PushDataQueue.Dequeue(DequeuedData)) {
                const OdinReturnCode PushResult = odin_audio_push_data(
                    DequeuedData.MediaStreamHandle, DequeuedData.AudioData.GetData(),
                    DequeuedData.AudioData.Num());
                if (odin_is_error(PushResult)) {
                    FString FormatOdinError =
                        UOdinFunctionLibrary::FormatOdinError(PushResult, false);
                    UE_LOG(Odin, Error,
                           TEXT("FOdinAudioPushDataThread failed calling odin_audio_push_data, "
                                "reason: %s"),
                           *FormatOdinError);
                }
            }
        }

        FPlatformProcess::Sleep(0.01);
    }
    return 0;
}

void FOdinAudioPushDataThread::Stop()
{
    UE_LOG(Odin, Verbose, TEXT("FOdinAudioPushDataThread::Stop()"));
    FRunnable::Stop();
    bShutdown = true;
}

void FOdinAudioPushDataThread::Exit()
{
    FRunnable::Exit();
    Stop();
}
