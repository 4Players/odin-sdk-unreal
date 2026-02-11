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

    /**
     * Associates an Odin decoder with a specific peer within a room.
     * @param DecoderHandle The decoder to link.
     * @param TargetRoom The room the peer belongs to.
     * @param PeerId The unique identifier of the peer.
     */
    void LinkDecoderToPeer(OdinDecoder* DecoderHandle, OdinRoom* TargetRoom, const uint32 PeerId);

    /**
     * Removes a decoder from all peer associations.
     * @param DecoderHandle The decoder to unlink.
     */
    void UnlinkDecoder(const OdinDecoder* DecoderHandle);

    /**
     * Retrieves all decoders currently associated with a specific peer in a room.
     * @param TargetRoom The room the peer belongs to.
     * @param PeerId The unique identifier of the peer.
     * @return An array of pointers to associated OdinDecoders.
     */
    TArray<OdinDecoder*> GetDecoderHandlesFor(OdinRoom* TargetRoom, uint32 PeerId) const;

    /**
     * Enqueues an incoming datagram for asynchronous processing by the thread.
     * @param RoomHandle The room from which the datagram originated.
     * @param PeerId The ID of the peer who sent the datagram.
     * @param ChannelMask The channel mask associated with the audio data.
     * @param SsrcId The synchronization source identifier.
     * @param Datagram The raw packet data to be processed.
     */
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
