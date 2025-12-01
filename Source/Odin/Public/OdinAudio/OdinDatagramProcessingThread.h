#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "odin.h"

typedef TPair<OdinRoom*, uint32> FDecoderIdentifier;

class FOdinDatagramProcessingThread : public FRunnable
{
  public:
    FOdinDatagramProcessingThread();
    virtual ~FOdinDatagramProcessingThread() override;

    void                 LinkDecoderToPeer(OdinDecoder* DecoderHandle, OdinRoom* TargetRoom, const uint32 PeerId);
    void                 UnlinkDecoder(const OdinDecoder* DecoderHandle);
    TArray<OdinDecoder*> GetDecoderHandlesFor(OdinRoom* TargetRoom, uint32 PeerId) const;

    void HandleDatagram(OdinRoom* RoomHandle, uint32 PeerId, uint64 ChannelMask, uint32 SsrcId, TArray<uint8>&& Datagram);

    virtual uint32 Run() override;
    virtual void   Exit() override;

  private:
    struct FOdinDatagramEvent {
        OdinRoom*     Handle;
        uint32        PeerId;
        uint64        ChannelMask;
        uint32        SsrcId;
        TArray<uint8> Datagram;
    };

    mutable FCriticalSection                     DecoderHandlesCS;
    TMap<FDecoderIdentifier, TSet<OdinDecoder*>> RegisteredDecoderHandles;

    TQueue<FOdinDatagramEvent, EQueueMode::Mpsc> DatagramQueue;

    FThreadSafeBool             bIsRunning;
    TUniquePtr<FRunnableThread> Thread;
    FEvent*                     PushEvent;
    float                       PushFrequencyInMs;
};
