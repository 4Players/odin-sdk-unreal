/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

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
     * @param ChannelMask The channels this decoder should receive datagrams for.
     * @remarks Defaults to every channel (single decoder per peer receiving everything). When
     * multiple decoders are linked to the same peer with non-overlapping masks each one only receives datagrams tagged with a
     * matching channel bit, allowing independent per-channel output
     */
    void LinkDecoderToPeer(OdinDecoder* DecoderHandle, OdinRoom* TargetRoom, const uint32 PeerId, uint64 ChannelMask = ~0ull);

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
     * Retrieves all decoders associated with a specific peer whose linked channel mask overlaps
     * the given channel mask
     * @param TargetRoom The room the peer belongs to.
     * @param PeerId The unique identifier of the peer.
     * @param ChannelMask The channel mask to filter by
     * @remarks A decoder is included if it shares at least one channel bit with this mask. A ChannelMask of 0 matches every decoder. Used to route an incoming
     * datagram only to the decoders that are interested in the channels it was transmitted on.
     * @return An array of pointers to matching OdinDecoders.
     */
    TArray<OdinDecoder*> GetDecoderHandlesFor(OdinRoom* TargetRoom, uint32 PeerId, uint64 ChannelMask) const;

    /**
     * Retrieves all decoders currently associated with a specific peer, across all rooms.
     * @param PeerId The identifier of the peer which is only unique within one room.
     * @remarks Use this only when the target room is not known; prefer GetDecoderHandlesFor when it is,
     * since a PeerId is only unique within a single room and this scans every registered room.
     * @return An array of pointers to associated OdinDecoders from any room.
     */
    TArray<OdinDecoder*> GetDecodersByPeer(uint32 PeerId) const;

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

    mutable FCriticalSection                             DecoderHandlesCS;
    TMap<FDecoderIdentifier, TMap<OdinDecoder*, uint64>> RegisteredDecoderHandles;

    TQueue<FOdinDatagramEvent, EQueueMode::Mpsc> DatagramQueue;

    FThreadSafeBool             bIsRunning;
    TUniquePtr<FRunnableThread> Thread;
    FEvent*                     PushEvent;
    float                       PushFrequencyInMs;
};
