#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "odin.h"

/**
 * @class FOdinAudioPushDataThread
 */
class FOdinAudioPushDataThread : public FRunnable
{
  public:
    FOdinAudioPushDataThread();
    virtual ~FOdinAudioPushDataThread() override;

    /**
     * Links an encoder to a target room.
     *
     * @param Encoder A pointer to the encoder object.
     * @param TargetRoom A pointer to the target room object.
     */
    void LinkEncoder(OdinEncoder* Encoder, OdinRoom* TargetRoom);
    /**
     * Unlinks an encoder from its associated room by using the encoder's handle.
     *
     * @param EncoderHandle A pointer to the encoder handle to be unlinked.
     * @return True if the encoder was successfully unlinked; otherwise, false.
     */
    bool UnlinkEncoder(OdinEncoder* EncoderHandle);

    /**
     * Queues audio data to be pushed to a specific encoder.
     *
     * @param TargetEncoder The encoder to which the audio data belongs.
     * @param Audio The audio data buffer to be processed.
     */
    void PushAudioToEncoder(OdinEncoder* TargetEncoder, TArray<float>&& Audio);

    virtual uint32 Run() override;
    virtual void   Exit() override;

  private:
    void        CleanupLinks();
    void        PushQueuedAudio();
    void        PopAllEncoders(TArray<uint8>& DatagramBuffer);
    static void SendDatagramToRoom(OdinRoom* TargetRoom, TArray<uint8>& DatagramBuffer, uint32 NumSamples);

    struct FOdinEncoderAudioFrame {
        OdinEncoder*  EncoderHandle;
        TArray<float> Audio;
    };

    TQueue<FOdinEncoderAudioFrame, EQueueMode::Mpsc> AudioPushQueue;

    FCriticalSection              EncoderRoomLinkCS;
    TMap<OdinEncoder*, OdinRoom*> EncoderRoomLinks;
    FThreadSafeBool               bIsRunning;
    TUniquePtr<FRunnableThread>   Thread;
    FEvent*                       PushEvent;
    float                         PushFrequencyInMs;
};