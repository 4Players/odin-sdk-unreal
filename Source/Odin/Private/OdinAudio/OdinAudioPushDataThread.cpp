#include "OdinAudio/OdinAudioPushDataThread.h"

#include "GenericPlatform/GenericPlatformAffinity.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "HAL/Event.h"
#include "OdinSubsystem.h"
#include "OdinVoice.h"
#include "odin.h"
#include "OdinFunctionLibrary.h"

FOdinAudioPushDataThread::FOdinAudioPushDataThread()
    : bIsRunning(false)
    , PushEvent(nullptr)
    , PushFrequencyInMs(10)
{
}

FOdinAudioPushDataThread::~FOdinAudioPushDataThread() = default;

void FOdinAudioPushDataThread::LinkEncoder(OdinEncoder* Encoder, OdinRoom* TargetRoom)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinAudioPushDataThread::LinkEncoder)
    if (!bIsRunning) {
        bIsRunning = true;
        PushEvent  = FGenericPlatformProcess::GetSynchEventFromPool();
        check(PushEvent);
        Thread.Reset(FRunnableThread::Create(this, TEXT("OdinPushAudioThread"), 0, TPri_TimeCritical));
    }

    if (Encoder && TargetRoom && UOdinSubsystem::GlobalIsRoomValid(TargetRoom)) {
        UnlinkEncoder(Encoder);

        FScopeLock Lock(&EncoderRoomLinkCS);
        EncoderRoomLinks.Add(Encoder, TargetRoom);
        ODIN_LOG(Verbose, "Linking Encoder %p to Odin Room %p", Encoder, TargetRoom);
    }
}

bool FOdinAudioPushDataThread::UnlinkEncoder(OdinEncoder* EncoderHandle)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinAudioPushDataThread::UnlinkEncoder)

    OdinRoom*  RemovedRoomHandle;
    FScopeLock Lock(&EncoderRoomLinkCS);
    const bool bFoundEntry = EncoderRoomLinks.RemoveAndCopyValue(EncoderHandle, RemovedRoomHandle);
    if (bFoundEntry && RemovedRoomHandle) {
        ODIN_LOG(Verbose, "Unlinked Encoder %p from Room %p", EncoderHandle, RemovedRoomHandle);
    }
    return bFoundEntry;
}

void FOdinAudioPushDataThread::PushAudioToEncoder(OdinEncoder* TargetEncoder, TArray<float>&& Audio)
{
    if (!TargetEncoder) {
        ODIN_LOG(Error, "Tried pushing audio to null encoder");
        return;
    }

    if (!Audio.IsEmpty()) {
        FOdinEncoderAudioFrame NewFrame{.EncoderHandle = TargetEncoder, .Audio = MoveTemp(Audio)};
        AudioPushQueue.Enqueue(MoveTemp(NewFrame));
    }
}

uint32 FOdinAudioPushDataThread::Run()
{
    TArray<uint8> ByteArray;
    ByteArray.SetNumZeroed(1920);
    while (bIsRunning) {
        check(PushEvent);
        PushEvent->Wait(PushFrequencyInMs);

        if (bIsRunning) {
            TRACE_CPUPROFILER_EVENT_SCOPE(FOdinAudioPushDataThread::Run);
            CleanupLinks();
            PushQueuedAudio();
            PopAllEncoders(ByteArray);
        }
    }
    return 0;
}

void FOdinAudioPushDataThread::CleanupLinks()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinAudioPushDataThread::CleanupLinks);

    FScopeLock Lock(&EncoderRoomLinkCS);
    for (auto EncoderIterator = EncoderRoomLinks.CreateIterator(); EncoderIterator; ++EncoderIterator) {
        OdinRoom* NativeOdinRoomHandle = EncoderIterator.Value();
        if (nullptr == EncoderIterator.Key() || nullptr == NativeOdinRoomHandle) {
            EncoderIterator.RemoveCurrent();
            ODIN_LOG(Verbose, "%s: Removed Encoder Linking due to invalid Encoder.", ANSI_TO_TCHAR(__FUNCTION__))
            continue;
        }

        const bool bIsRoomValid = UOdinSubsystem::GlobalIsRoomValid(NativeOdinRoomHandle);
        if (!bIsRoomValid) {
            ODIN_LOG(Verbose, "%s: Removed Encoder Linking due to invalid Room.", ANSI_TO_TCHAR(__FUNCTION__))
            EncoderIterator.RemoveCurrent();
        }
    }
}

void FOdinAudioPushDataThread::PushQueuedAudio()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinAudioPushDataThread::PushQueuedAudio);
    FOdinEncoderAudioFrame        Frame;
    TMap<OdinEncoder*, OdinRoom*> LocalLinksCopy;
    {
        FScopeLock Lock(&EncoderRoomLinkCS);
        LocalLinksCopy = EncoderRoomLinks;
    }

    while (AudioPushQueue.Dequeue(Frame) && bIsRunning) {
        if (Frame.EncoderHandle && LocalLinksCopy.Contains(Frame.EncoderHandle) && !Frame.Audio.IsEmpty()) {
            TRACE_CPUPROFILER_EVENT_SCOPE(FOdinAudioPushDataThread - odin_encoder_push);
            const OdinError Result = odin_encoder_push(Frame.EncoderHandle, Frame.Audio.GetData(), Frame.Audio.Num());
            ODIN_LOG(VeryVerbose, "%s calling odin_encoder_push.", ANSI_TO_TCHAR(__FUNCTION__));
            if (Result != ODIN_ERROR_SUCCESS) {
                ODIN_LOG(Error, "Error on odin_encoder_push: %s", *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(Result), false));
            }
        }
    }
}

void FOdinAudioPushDataThread::PopAllEncoders(TArray<uint8>& DatagramBuffer)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinAudioPushDataThread::PopAllEncoders);

    TMap<OdinEncoder*, OdinRoom*> LocalLinksCopy;
    {
        FScopeLock Lock(&EncoderRoomLinkCS);
        LocalLinksCopy = EncoderRoomLinks;
    }
    for (const TPair<OdinEncoder*, OdinRoom*>& Pair : LocalLinksCopy) {
        OdinEncoder* EncoderHandle = Pair.Key;
        OdinRoom*    TargetRoom    = Pair.Value;

        if (!EncoderHandle) {
            continue;
        }

        bool bHasData = true;
        while (bHasData) {
            uint32 NumSamples = 1300; // fixed resampled datagram and ignore FrameSampleCount
            DatagramBuffer.SetNumUninitialized(NumSamples);

            OdinError EncoderPopResult;
            {
                TRACE_CPUPROFILER_EVENT_SCOPE(odin_encoder_pop);
                ODIN_LOG(VeryVerbose, "odin_encoder_pop for Encoder %p", EncoderHandle);
                EncoderPopResult = odin_encoder_pop(EncoderHandle, DatagramBuffer.GetData(), &NumSamples);
            }

            switch (EncoderPopResult) {
                case ODIN_ERROR_SUCCESS: {
                    TRACE_CPUPROFILER_EVENT_SCOPE(Combined odin_room_send_datagram calls);
                    SendDatagramToRoom(TargetRoom, DatagramBuffer, NumSamples);
                } break;
                case ODIN_ERROR_NO_DATA: {
                    ODIN_LOG(VeryVerbose, "%s: No data on odin_encoder_pop", ANSI_TO_TCHAR(__FUNCTION__));
                    bHasData = false;
                } break;
                case ODIN_ERROR_ARGUMENT_INVALID_HANDLE: {
                    ODIN_LOG(Log,
                             "Invalid handle result during odin_encoder_pop. Single occurences of this line are expected. Many occurences can indicate an "
                             "issue. Message: %s",
                             *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(EncoderPopResult), false));
                    bHasData = false;
                } break;
                default: {
                    ODIN_LOG(Error, "Error on odin_encoder_pop: %s", *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(EncoderPopResult), false));
                    bHasData = false;
                } break;
            }
        }
    }
}

void FOdinAudioPushDataThread::SendDatagramToRoom(OdinRoom* TargetRoom, TArray<uint8>& DatagramBuffer, const uint32 NumSamples)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Single odin_room_send_datagram calls);
    ODIN_LOG(VeryVerbose, "Sending previous encoder data to room %p", TargetRoom);
    if (UOdinSubsystem::GlobalIsRoomValid(TargetRoom)) {
        const auto RoomSendResult = odin_room_send_datagram(TargetRoom, DatagramBuffer.GetData(), NumSamples);
        if (RoomSendResult != OdinError::ODIN_ERROR_SUCCESS) {
            ODIN_LOG(Error, "Error on odin_room_send_datagram: %s", *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(RoomSendResult), false));
        }
    }
}

void FOdinAudioPushDataThread::Exit()
{
    if (!bIsRunning) {
        return;
    }

    bIsRunning = false;
    {
        FScopeLock Lock(&EncoderRoomLinkCS);
        EncoderRoomLinks.Empty();
    }

    if (PushEvent) {
        PushEvent->Trigger();
    }

    if (Thread.IsValid()) {
        Thread->WaitForCompletion();
    }

    if (PushEvent) {
        FGenericPlatformProcess::ReturnSynchEventToPool(PushEvent);
        PushEvent = nullptr;
    }
}