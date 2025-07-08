#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "odin_sdk.h"
#include "Containers/Queue.h"

/**
 * @struct FOdinAudioPushData
 * @brief Represents a structure for encapsulating audio data and its associated media stream
 * handle.
 *
 * This structure is used to package audio data along with a corresponding media stream handle
 * for processing or transmission within the system.
 */
struct FOdinAudioPushData {
    OdinMediaStreamHandle MediaStreamHandle;
    TArray<float>         AudioData;
};

/**
 * @class FOdinAudioPushDataThread
 *
 * This class is designed to handle the transmission of audio data to Odin in a seperate thread.
 * This way we deload the Audio Capture thread and avoid potential audio issues.
 *
 */
class FOdinAudioPushDataThread : public FRunnable
{
  public:
    FOdinAudioPushDataThread();
    virtual ~FOdinAudioPushDataThread() override;

    /**
     * Pushes audio data for the specified media stream to the processing queue.
     *
     * @param MediaHandle The handle identifying the associated media stream.
     * @param AudioData Pointer to the array containing audio sample data in floating-point format.
     * @param NumSamples The total number of audio samples in the provided data array.
     */
    void PushAudio(OdinMediaStreamHandle MediaHandle, const float* AudioData, int32 NumSamples);
    /**
     * @brief Requests the shutdown of the audio push data thread.
     *
     * This method sets an internal flag to indicate that the thread should stop running.
     * It is typically called during the cleanup or deinitialization phase to ensure
     * a graceful termination of the thread's execution.
     */
    void RequestShutdown();

  protected:
    virtual bool   Init() override;
    virtual uint32 Run() override;
    virtual void   Stop() override;
    virtual void   Exit() override;

  private:
    bool                   bShutdown = false;
    class FRunnableThread* Thread;

    /**
     * @var PushDataQueue
     * Thread-safe queue for managing audio push data objects.
     */
    TQueue<FOdinAudioPushData, EQueueMode::Mpsc> PushDataQueue;
};
