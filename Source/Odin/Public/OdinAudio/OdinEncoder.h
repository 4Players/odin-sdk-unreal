/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */

#pragma once

#include <atomic>

#include "CoreMinimal.h"
#include "ISubmixBufferListener.h"
#include "OdinCore/include/odin.h"
#include "Generators/AudioGenerator.h"
#include "OdinChannelMask.h"
#include "OdinNative/OdinNativeHandle.h"
#include "HAL/ThreadSafeBool.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "AudioDefines.h"
#include "OdinEncoder.generated.h"

struct FOdinPosition;
class UAudioGenerator;
class UOdinPipeline;
class UOdinRoom;
class FOdinSubmixListener;

/**
 * A channel mask paired with the position it should be set to
 * used to batch multiple SetPosition calls
 */
USTRUCT(BlueprintType)
struct ODIN_API FOdinChannelPosition {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin")
    FOdinChannelMask ChannelMask;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin")
    FOdinPosition Position;
};

/**
 * Represents an encoder for local media streams, which encapsulates the components required to
 * process outgoing audio streams captured from local sources(e.g.a microphone).
 */
UCLASS(ClassGroup = (Odin), Blueprintable, BlueprintType)
class ODIN_API UOdinEncoder : public UObject
{
    GENERATED_BODY()

  public:
    UOdinEncoder(const class FObjectInitializer& PCIP);

    /**
     * Create uobject encoder with already created encoder handle
     * @param WorldContextObject   context lifetime object
     * @param Handle   incomplete internal handle type
     * @return uobject encoder or null
     */
    static UOdinEncoder* ConstructEncoder(UObject* WorldContextObject, OdinEncoder* Handle);
    /**
     * Create uobject encoder with create encoder
     * @param WorldContextObject   context lifetime object
     * @param PeerId   binding id
     * @param SampleRate   samplerate for f32bit
     * @param bStereo   channels interleaved
     * @return uobject encoder or null
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Construct Encoder", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline")
    static UOdinEncoder* ConstructEncoder(UObject* WorldContextObject, int64 PeerId, int32 SampleRate, bool bStereo);
    /**
     * Creates a new ODIN encoder instance with default settings used to encode audio captured from
     * local sources, such as a microphone using the given sample rate and channel layout.
     * @remarks Peer id 0 will rewrite header on datagram send for connected room peer id; default bitrate 32000 (with stereo 128000), voip true (with stereo
     * false), disabled interval, and expected packet loss of 15%
     * @param InPeerId   binding id
     * @param InSampleRate   samplerate for f32bit
     * @param bUseStereo   channels interleaved
     * @return Encoder or null
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Encoder", ToolTip = "Creates a new encoder handle (Peer id 0 uses auto-detect)"),
              Category = "Odin|Audio Pipeline")
    UOdinEncoder* CreateEncoder(int64 InPeerId, int32 InSampleRate, bool bUseStereo);
    /**
     * Create uobject encoder with create encoder ex
     * @param WorldContextObject   context lifetime object
     * @param PeerId   binding id
     * @param SampleRate   samplerate for f32bit
     * @param bStereo   channels interleaved
     * @param bApplication_VOIP   Toggle codec type. Set to true for sending voice, false for anything else (like music).
     * @param Bitrate_Kbps   encoding bitrate
     * @param Packet_Loss_Perc   expected packet loss percentage
     * @param Update_Position_Interval_MS   interval where pop will produce position datagram (disable 0)
     * @return uobject encoder or null
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Construct Encoder (Advanced)", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline")
    static UOdinEncoder* ConstructEncoderEx(UObject* WorldContextObject, int64 PeerId, int32 SampleRate, bool bStereo, bool bApplication_VOIP,
                                            int32 Bitrate_Kbps, int32 Packet_Loss_Perc, int64 Update_Position_Interval_MS);

    /**
     * Creates a new ODIN encoder instance with extended codec configuration parameters.
     * In addition to the sample rate and stereo configuration, it allows specification
     * of whether the application is intended for VoIP, a target bitrate and the encoder's expected
     * packet loss percentage.
     * @param InConnectedPeerId   Binding id
     * @param InSampleRate   samplerate for f32bit
     * @param bUseStereo   channels interleaved
     * @param bApplication_VOIP   Toggle codec type. Set to true for sending voice, false for anything else (like music).
     * @param Bitrate_Kbps   encoding bitrate
     * @param Packet_Loss_Perc   expected packet loss percentage
     * @param Update_Position_Interval_MS   interval where pop will produce position datagram (disable 0)
     * @return Encoder or null
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Encoder (Advanced)", ToolTip = "Creates a new encoder handle"), Category = "Odin|Audio Pipeline")
    UOdinEncoder* CreateEncoderEx(int64 InConnectedPeerId, int32 InSampleRate, bool bUseStereo, bool bApplication_VOIP, int32 Bitrate_Kbps,
                                  int32 Packet_Loss_Perc, int64 Update_Position_Interval_MS);

    /**
     * Invalidate internal encoder handle
     * @param Encoder   get handle from
     * @return true if handle was set
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Free Encoder", ToolTip = "Frees a encoder and handle"), Category = "Odin|Audio Pipeline")
    static bool FreeEncoder(UOdinEncoder* Encoder);
    /**
     * Invalidate internal encoder handle
     * @return true if handle is not nullptr
     */
    static bool FreeEncoderHandle(OdinEncoder* Encoder);

    /**
     * Get encoder audio pipeline
     * @param Encoder   get handle from
     * @return get current pipeline or construct new
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Or Create Encoder Pipeline", ToolTip = "Get the pipeline of an encoder"),
              Category = "Odin|Audio Pipeline")
    static UOdinPipeline* GetOrCreateEncoderPipeline(UOdinEncoder* Encoder);

    /**
     * Get encoder audio pipeline
     * @return get current pipeline or construct new
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Or Create Pipeline", ToolTip = "Get the pipeline of an encoder"), Category = "Odin|Audio Pipeline")
    UOdinPipeline* GetOrCreatePipeline();

    /**
     * Returns whether the encoder is currently processing silence. This reflects the internal
     * silence detection state of the encoder, which updates as audio is processed.
     * @remarks Always returns true, if the passed encoder is invalid.
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Silent Flag", ToolTip = "Get the silent flag of a encoder", KeyWords = "IsSilent Silent"),
              Category = "Odin|Audio Pipeline")
    bool GetIsSilent() const;

    /**
     * Registers a callback to receive encoder audio events. The `filter` determines which event
     * types will trigger the callback, allowing selective handling.
     * @remarks Any previously registered callback is replaced.
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Register AudioEvent Handler", ToolTip = "Set OnAudioEventCallback of the encoder", DefaultToSelf = "UserData"),
              Category = "Odin|Audio Pipeline|Events")
    bool SetAudioEventHandler(UPARAM(meta = (DisplayName = "Filter", Bitmask, BitmaskEnum = EOdinAudioEvents)) int32 EFilter = 1);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinEncoderAudioEventCallbackDelegate, UOdinEncoder*, encoder, EOdinAudioEvents, filter);
    /**
     * A callback invoked when the encoder reports one or more audio events.
     *
     * @attention Please remember to start listening by calling SetAudioEventHandler!
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Audio Pipeline|Events",
              meta = (ToolTip = "Invoked when the encoder reports an audio event. Don't forget to start listening by calling SetAudioEventHandler!"))
    FOdinEncoderAudioEventCallbackDelegate OnAudioEventCallbackBP;
    /**
     * Internal Encoder callback hook to redirect incoming callback for audio events
     * @remarks This should only be changed if the underlying callback has to call a custom implementation of handling callbacks
     */
    void (*OdinEncoderEventCallbackFunc)(struct OdinEncoder* encoder, enum OdinAudioEvents events,
                                         void* user_data) = [](struct OdinEncoder* encoder, const OdinAudioEvents events, void* user_data) {
        HandleOdinAudioEventCallback(encoder, events, static_cast<uint64>(reinterpret_cast<UPTRINT>(user_data)));
    };

    /**
     * Set and add generator delegate handle for current encoder
     * @remarks internal use
     * @param Generator   to add the delegate to
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Audio Generator", ToolTip = "Set the AudioGenerator of an encoder"),
              Category = "Odin|Audio Pipeline")
    void SetAudioGenerator(UAudioGenerator* Generator);

    /**
     * Updates the 3D position of the specified channel mask. To assign different positions to multiple masks, call this function once per mask.
     * @remarks An encoder holds at most 12 channel positions; each masked channel occupies one slot until removed with ClearPosition. Passing a
     * full channel mask (FOdinChannelMask::CreateFull) therefore fails with ODIN_ERROR_AUDIO_POSITION_LIMIT_REACHED. Mask only the channels that
     * are actually transmitted on - for plain voice transmission that is just channel 0.
     * @remarks The server only forwards audio to peers whose positions are within a distance of 1.0 of each other, so coordinates must be scaled
     * such that 1.0 equals the intended hearing range (e.g. divide unreal world units by the distance where the client side attenuation reaches
     * silence). Unscaled world coordinates put peers out of range of each other and the server forwards no audio at all.
     * @param ChannelMask   audio layer
     * @param Position   position for each layer
     * @return true if successful
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Set Encoder Position",
                          ToolTip     = "Set the encoder position. Scale so 1.0 = hearing range (server culls beyond 1.0); mask only transmitted channels"),
              Category = "Odin|Audio Pipeline")
    bool SetPosition(FOdinChannelMask ChannelMask, FOdinPosition Position);

    /**
     * Convenience wrapper to assign a position to several channel masks at once
     * to update position for a spatialized proximity channel alongside fixed/unused position one
     * @remarks Positions follow the same scaling convention as SetPosition: coordinates scaled such that a distance of 1.0 equals the hearing
     * range, beyond which the server stops forwarding audio.
     * @param Positions channel mask + position pairs to apply; equivalent to calling SetPosition once per entry
     * @return true if every SetPosition call succeeded
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Encoder Positions", ToolTip = "Set multiple channel positions of the encoder in one call"),
              Category = "Odin|Audio Pipeline")
    bool SetPositions(const TArray<FOdinChannelPosition>& Positions);

    /**
     * Removes the position entry associated with the given channel mask, which also stops the
     * encoder from transmitting on that channel entirely
     * @remarks this does not reset the position to the origin while leaving the channel active.
     * Use SetPosition instead if the channel should keep transmitting with a unused position.
     * @param ChannelMask   audio layer
     * @return true if successful
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Clear Encoder Position",
                          ToolTip     = "Removes the position of the encoder for the given channel(s), stopping transmission on them"),
              Category = "Odin|Audio Pipeline")
    bool ClearPosition(FOdinChannelMask ChannelMask);

    /**
     * Get a datagram from the current encoder
     * @remarks Will trigger processing of the audio pipeline and returns if processing is done
     * @param Datagram   buffer
     * @return bytes written to buffer or 0
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Pop Datagram", ToolTip = "Pop the datagram samples of an encoder (start audio pipeline)"),
              Category = "Odin|Audio Pipeline")
    int32 Pop(TArray<uint8>& Datagram);

    /**
     * Set tarray samples wrapper to process on pop
     * @param Samples   audio data
     * @return samples send to encoder or 0
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Push Samples", ToolTip = "Push samples to the audio encoder pipeline buffer"),
              Category = "Odin|Audio Pipeline")
    int32 Push(TArray<float> Samples);

    /**
     * The delay  parameter is used to align the reverse stream processing with the forward (capture) stream.
     * @remarks The delay can be expressed as:
     * @remarks `delay = (t_render - t_analyze) + (t_process - t_capture)`
     * @remarks where:
     * @remarks - `t_render` is the time the first sample of the same frame is rendered by the audio hardware.
     * @remarks - `t_analyze` is the time the frame is processed in the reverse stream.
     * @remarks - `t_capture` is the time the first sample of a frame is captured by the audio hardware.
     * @remarks - `t_process` is the time the frame is processed in the forward stream.
     * @param DelayInMs Delay in Milliseconds.
     */
    UFUNCTION(BlueprintCallable, meta = (Category = "Odin|Audio Pipeline"))
    void SetEchoCancellationProcessingDelay(int32 DelayInMs = 15);

    /**
     * Set samples to process
     * @remarks fill encoder with samples
     * @param Samples audio data
     * @param Count sample num
     * @return samples send to encoder or 0
     */
    int32 Push(float* Samples, int32 Count);

    /**
     * Get num based on samplerate and channels
     * @param MS frame length
     * @return sample size
     */
    int32 GetNumSampleSize(const int32 MS = 20) const
    { return SampleRate / 1000 * MS * (bStereo ? 2 : 1); }

    /**
     * Get all rooms this encoder currently broadcasts to
     * @return the rooms currently linked to this encoder
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Linked Rooms", ToolTip = "Get all rooms this encoder currently broadcasts to"),
              Category = "Odin|Audio Pipeline")
    TArray<UOdinRoom*> GetLinkedRooms() const;

    /**
     * Get internal handle
     * @remarks OdinEncoder operations are thread-safe and the same encoder handle may be accessed concurrently from multiple threads.
     * @return incomplete handle type or nullptr
     */
    inline OdinEncoder* GetHandle() const
    { return IsValid(Handle) && Handle->IsValidLowLevel() ? static_cast<OdinEncoder*>(Handle->GetHandle()) : nullptr; }

    /**
     * Replace internal handle object
     * @param handle internal handle
     */
    void SetHandle(OdinEncoder* handle);

    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Capture")
    UAudioGenerator* AudioGenerator = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Odin")
    int64 PeerId;
    UPROPERTY(BlueprintReadOnly, Category = "Odin")
    int32 SampleRate = 48000;
    UPROPERTY(BlueprintReadOnly, Category = "Odin")
    bool bStereo = false;

  protected:
    virtual void BeginDestroy() override;

    UFUNCTION()
    void OnPipelineApmConfigChanged(UOdinPipeline* AffectedPipeline, int32 EffectId, FOdinApmConfig NewApmConfig);

    UPROPERTY()
    UOdinPipeline* Pipeline = nullptr;

  private:
    UPROPERTY()
    UOdinHandle*          Handle;
    FAudioGeneratorHandle Audio_Generator_Handle;
    static void           HandleOdinAudioEventCallback(OdinEncoder* EncoderHandle, const OdinAudioEvents Events, uint64 RegistrationId);

    TSharedPtr<FOdinSubmixListener> SubmixListener;
};

/**
 * Stateful linear resampler for interleaved audio streams. Keeps the fractional read position and
 * the last input frame across blocks, so the produced sample count matches the rate ratio on
 * average and no drift accumulates. Not thread-safe; each instance belongs to a single stream.
 */
struct ODIN_API FOdinStreamResampler {
    void Configure(int32 InRate, int32 OutRate, int32 InChannels);
    void Process(const float* InSamples, uint32 NumSamples, TArray<float>& OutSamples);

  private:
    int32         InputRate   = 0;
    int32         OutputRate  = 0;
    int32         NumChannels = 0;
    double        Step        = 1.0;
    double        Phase       = 0.0;
    TArray<float> LastFrame;
};

class ODIN_API FOdinSubmixListener : public ISubmixBufferListener
{
  public:
    FOdinSubmixListener();
    ~FOdinSubmixListener();

    virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate,
                                   double AudioClock) override;
    void         SetPipelineHandle(UOdinPipeline* NewHandle);
    void         AttachToSubmix();
    void         AddEffectId(uint32 EffectId, int32 PlaybackSampleRate, bool bPlaybackStereo);
    void         DetachFromSubmix();
    void         RemoveEffectId(uint32 EffectId);
    void         SetDelay(int32 NewDelayInMs);
    int32        GetNumEffectsRegistered() const;

  protected:
    void OnAudioDeviceCreated(Audio::FDeviceId Id);
    void OnAudioDeviceDestroyed(Audio::FDeviceId Id);

  private:
    struct FApmEffectInfo {
        uint32 EffectId           = 0;
        int32  PlaybackSampleRate = 48000;
        int32  PlaybackChannels   = 1;

        bool operator==(const FApmEffectInfo& Other) const
        { return EffectId == Other.EffectId; }
    };

    std::atomic<const OdinPipeline*>   NativePipelineHandle{nullptr};
    TArray<FApmEffectInfo>             ApmEffectIds;
    TMap<uint32, FOdinStreamResampler> PlaybackResamplers;
    TArray<FApmEffectInfo>             EffectInfoScratch;
    TArray<float>                      ChannelScratch;
    TArray<float>                      ResampleScratch;
    mutable FCriticalSection           EffectIdAccessSection;
    std::atomic<int32>                 DelayMs      = 15;
    std::atomic<bool>                  bIsListening = false;

    TSharedPtr<Audio::FDeviceId, ESPMode::ThreadSafe> ListenTargetId;
    FDelegateHandle                                   AudioDeviceCreatedCallbackHandle;
    FDelegateHandle                                   AudioDeviceDestroyedCallbackHandle;
};