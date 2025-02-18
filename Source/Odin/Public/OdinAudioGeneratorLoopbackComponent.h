// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SampleBuffer.h"
#include "Components/SynthComponent.h"
#include "Generators/AudioGenerator.h"
#include "OdinAudioGeneratorLoopbackComponent.generated.h"

class UAudioGenerator;

/**
 * UOdinAudioGeneratorLoopbackComponent
 *
 * A SynthComponent that captures audio from a UAudioGenerator and plays it back in the game.
 * This allows loopback of generated audio, such as voice capture.
 * The component uses a circular buffer to store incoming audio data.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class ODIN_API UOdinAudioGeneratorLoopbackComponent : public USynthComponent
{
    GENERATED_BODY()

  public:
    UOdinAudioGeneratorLoopbackComponent(const FObjectInitializer& ObjectInitializer);

    /**
     * Assigns an audio generator to be used as the playback source.
     * For example, passing an OdinAudioCapture object allows real-time monitoring
     * of the audio captured by the capture component.
     *
     * @param Generator The target audio generator that provides the audio data.
     */
    UFUNCTION(BlueprintCallable, Category = "Loopback")
    void SetLoopbackGenerator(UAudioGenerator* Generator);

    /**
     * Retrieves the current buffer size in seconds.
     * The buffer is implemented as a circular buffer
     *
     * @return The buffer size in seconds.
     */
    UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = "Loopback")
    float GetBufferSize() const;

    /**
     * Sets a new buffer size (in seconds).
     * The circular buffer is resized to the new duration while keeping existing data.
     *
     * @param NewSize The new buffer size in seconds.
     */
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Loopback")
    void SetBufferSize(float NewSize);

  protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    /**
     * Initializes the synth component when it is created.
     * Configures the audio buffer size based on the initial sample rate.
     *
     * @param SampleRate sample rate at which the audio is generated.
     * @return True if initialization is successful.
     */
    virtual bool Init(int32& SampleRate) override;

    /**
     * Generates audio output by reading samples from the circular buffer.
     * This function is called  by the audio engine.
     *
     * @param OutAudio output audio buffer.
     * @param NumSamples number of samples requested for playback.
     * @return number of samples actually written to the output buffer.
     */
    virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

    /**
     * Updates the capacity of the circular buffer based on the sample rate,
     * channel count, and buffer size.
     */
    void UpdateAudioBufferCapacity();

    /**
     * Removes and cleans up the audio generator delegate.
     */
    void RemoveGeneratorDelegate();

    /**
     * The duration (in seconds) of audio that can be stored in the circular buffer.
     * Increasing this value allows for more buffered audio data before overwriting old data at the
     * cost of higher memory usage.
     */
    UPROPERTY(BlueprintGetter = GetBufferSize, BlueprintSetter = SetBufferSize,
              Category = "Loopback")
    float BufferSize = 3.0f;

    /** Handle for managing the connection to the audio generator delegate. */
    FAudioGeneratorHandle GeneratorHandle;

    /** Weak reference to the currently assigned audio generator */
    UPROPERTY(BlueprintReadOnly, Category = "Loopback")
    UAudioGenerator* ConnectedGenerator;

    /** Circular buffer for storing audio samples received from the generator. */
    Audio::TCircularAudioBuffer<float> AudioBuffer;

    /** Temporary Audio buffer used for remixing. Reads samples from the circular audio buffer for
     * transfer to the remix buffer */
    TArray<float> TemporaryBuffer;

    /** Sample Buffer used for remixing from the generators num channels to the default 2 channels
     * used for playback */
    Audio::TSampleBuffer<float> RemixBuffer;

    /** The sample rate at which the component processes audio. */
    int32 InitialSampleRate;
};
