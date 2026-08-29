/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#include "OdinAudio/OdinDatagramProcessingThread.h"

#include "OdinFunctionLibrary.h"
#include "OdinSubsystem.h"
#include "OdinVoice.h"

#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "HAL/Event.h"

FOdinDatagramProcessingThread::FOdinDatagramProcessingThread()
    : bIsRunning(false)
    , PushEvent(nullptr)
    , PushFrequencyInMs(10)
{
}

FOdinDatagramProcessingThread::~FOdinDatagramProcessingThread() = default;

void FOdinDatagramProcessingThread::LinkDecoderToPeer(OdinDecoder* DecoderHandle, OdinRoom* TargetRoom, const uint32 PeerId, uint64 ChannelMask)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinDatagramProcessingThread::LinkDecoderToPeer);

    if (DecoderHandle && TargetRoom) {
        if (!bIsRunning) {
            bIsRunning = true;
            PushEvent  = FGenericPlatformProcess::GetSynchEventFromPool();
            check(PushEvent);
            Thread.Reset(FRunnableThread::Create(this, TEXT("OdinDatagramProcessingThread"), 0, TPri_TimeCritical));
        }

        FScopeLock                  RegisterDecoderLock(&DecoderHandlesCS);
        TMap<OdinDecoder*, uint64>& OdinDecoders = RegisteredDecoderHandles.FindOrAdd(TPair<OdinRoom*, uint32>(TargetRoom, PeerId));
        OdinDecoders.Add(DecoderHandle, ChannelMask);
        ODIN_LOG(Verbose, "Linking Odin Decoder %p to Room Handle %p and Peer Id %u with ChannelMask %llu", DecoderHandle, TargetRoom, PeerId, ChannelMask);

    } else {
        ODIN_LOG(Error, "Linking Odin Decoder requires valid internal Decoder Handle and Room Handle.");
    }
}

void FOdinDatagramProcessingThread::UnlinkDecoder(const OdinDecoder* DecoderHandle)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinDatagramProcessingThread::UnlinkDecoder);

    if (!DecoderHandle) {
        ODIN_LOG(Log, "Tried unlinking invalid Decoder Handle, aborting.");
        return;
    }
    {
        FScopeLock DeregisterDecoderLock(&DecoderHandlesCS);
        int32      RemovedDecoders = 0;
        for (auto It = RegisteredDecoderHandles.CreateIterator(); It; ++It) {
            RemovedDecoders += It.Value().Remove(DecoderHandle);
            if (It.Value().IsEmpty()) {
                It.RemoveCurrent();
            } else if (!UOdinSubsystem::GlobalIsRoomValid(It.Key().Key)) {
                It.RemoveCurrent();
            }
        }

        ODIN_LOG(Verbose, "Deregistered %d Decoders", RemovedDecoders);
        ODIN_LOG(Verbose, "Number of registered decoder handles: %d", RegisteredDecoderHandles.Num());
    }
}

TArray<OdinDecoder*> FOdinDatagramProcessingThread::GetDecoderHandlesFor(OdinRoom* TargetRoom, uint32 PeerId) const
{ return GetDecoderHandlesFor(TargetRoom, PeerId, ~static_cast<uint64>(0)); }

TArray<OdinDecoder*> FOdinDatagramProcessingThread::GetDecoderHandlesFor(OdinRoom* TargetRoom, uint32 PeerId, uint64 ChannelMask) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinDatagramProcessingThread::GetDecoderHandlesFor);

    TArray<OdinDecoder*> Result;
    FScopeLock           DeregisterDecoderLock(&DecoderHandlesCS);
    if (const TMap<OdinDecoder*, uint64>* DecodersForPeer = RegisteredDecoderHandles.Find(FDecoderIdentifier(TargetRoom, PeerId))) {
        for (const auto& DecoderAndMask : *DecodersForPeer) {
            if (ChannelMask == 0 || (DecoderAndMask.Value & ChannelMask) != 0) {
                Result.Add(DecoderAndMask.Key);
            }
        }
    }
    return Result;
}

TArray<OdinDecoder*> FOdinDatagramProcessingThread::GetDecodersByPeer(uint32 PeerId) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinDatagramProcessingThread::GetDecodersByPeer);

    TArray<OdinDecoder*> Result;
    FScopeLock           DeregisterDecoderLock(&DecoderHandlesCS);
    for (const auto& RegisteredDecoderHandle : RegisteredDecoderHandles) {
        if (RegisteredDecoderHandle.Key.Value == PeerId) {
            TArray<OdinDecoder*> Keys;
            RegisteredDecoderHandle.Value.GetKeys(Keys);
            Result.Append(Keys);
        }
    }
    return Result;
}

void FOdinDatagramProcessingThread::HandleDatagram(OdinRoom* RoomHandle, uint32 PeerId, uint64 ChannelMask, uint32 SsrcId, TArray<uint8>&& Datagram)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FOdinDatagramProcessingThread::HandleDatagram)
    FOdinDatagramEvent EventData =
        FOdinDatagramEvent{.Handle = RoomHandle, .PeerId = PeerId, .ChannelMask = ChannelMask, .SsrcId = SsrcId, .Datagram = MoveTemp(Datagram)};

    DatagramQueue.Enqueue(MoveTemp(EventData));
}

uint32 FOdinDatagramProcessingThread::Run()
{
    while (bIsRunning) {
        check(PushEvent);
        PushEvent->Wait(PushFrequencyInMs);

        {
            TRACE_CPUPROFILER_EVENT_SCOPE(FOdinDatagramProcessingThread - Queue Processing)
            FOdinDatagramEvent DatagramEvent;
            while (DatagramQueue.Dequeue(DatagramEvent) && bIsRunning) {
                TRACE_CPUPROFILER_EVENT_SCOPE(FOdinDatagramProcessingThread - Single Datagram Processing)
                if (const UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {
                    if (OdinSubsystem->IsRoomRegistered(DatagramEvent.Handle)) {
                        for (OdinDecoder* Decoder :
                             OdinSubsystem->GetDecoderHandlesFor(DatagramEvent.Handle, DatagramEvent.PeerId, DatagramEvent.ChannelMask)) {
                            {
                                TRACE_CPUPROFILER_EVENT_SCOPE(FOdinDatagramProcessingThread - decoder_push)
                                const OdinError Result = odin_decoder_push(Decoder, DatagramEvent.Datagram.GetData(), DatagramEvent.Datagram.Num());
                                if (Result != OdinError::ODIN_ERROR_SUCCESS) {
                                    ODIN_LOG(Error, "Aborting Push due to invalid odin_decoder_push call: %s",
                                             *UOdinFunctionLibrary::FormatOdinError(static_cast<EOdinError>(Result), false));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

void FOdinDatagramProcessingThread::Exit()
{
    if (!bIsRunning) {
        return;
    }

    bIsRunning = false;

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