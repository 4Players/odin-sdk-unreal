/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "OdinChannelMask.h"
#include "UObject/Object.h"
#include "AudioDefines.h"

#include "OdinCore/include/odin.h"
#include "OdinNative/OdinNativeHandle.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "UObject/StrongObjectPtr.h"

#include "OdinDecoder.generated.h"

/**
 * Represents a decoder for media streams from remote voice chat clients, which encapsulates all
 * the components required to process incoming audio streams.
 */
UCLASS(ClassGroup = (Odin), Blueprintable, BlueprintType)
class ODIN_API UOdinDecoder : public UObject
{
    GENERATED_BODY()

    friend class UOdinPipeline;

  public:
    UOdinDecoder(const class FObjectInitializer &PCIP);

    /**
     * Create uobject decoder with already created decoder handle
     * @param WorldContextObject   context lifetime object
     * @param Handle   incomplete internal handle type
     * @return uobject decoder or null
     */
    static UOdinDecoder *ConstructDecoder(UObject *WorldContextObject, OdinDecoder *Handle);

    /**
     * Create uobject decoder with already created decoder handle
     * @param WorldContextObject   context lifetime object
     * @param SampleRate   samplerate for f32bit
     * @param bUseStereo   channels interleaved
     * @return uobject decoder or null
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Construct Decoder", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Audio Pipeline")
    static UOdinDecoder *ConstructDecoder(UObject *WorldContextObject, int32 SampleRate = 48000, bool bUseStereo = false);

    /**
     * Internally Creates a new instance of an ODIN decoder with default settings used to process the remote
     * media stream. Will re-setup
     * @param DecoderSampleRate   samplerate for f32bit
     * @param bUseStereo   channels interleaved
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Decoder", ToolTip = "Creates a new decoder handle"), Category = "Odin|Audio Pipeline")
    UOdinDecoder *SetupInternalDecoder(int32 DecoderSampleRate, bool bUseStereo);

    /**
     * Frees the resources associated with the specified decoder.
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Free Decoder", ToolTip = "Frees a Decoder and handle"), Category = "Odin|Audio Pipeline")
    static bool FreeDecoder(UOdinDecoder *Decoder);

    static bool FreeDecoderInternal(OdinDecoder *DecoderHandle);

    /**
     * Returns an object to the pointer of the internal ODIN audio pipeline instance used by the given decoder.
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Decoder Pipeline", ToolTip = "Get the pipeline of a decoder"), Category = "Odin|Audio Pipeline")
    static UOdinPipeline *GetOrCreateDecoderPipeline(UOdinDecoder *Decoder);
    /**
     * Returns an object to the pointer of the internal ODIN audio pipeline instance used by the given decoder.
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Pipeline", ToolTip = "Get the pipeline of a decoder"), Category = "Odin|Audio Pipeline")
    UOdinPipeline *GetOrCreatePipeline();

    /**
     * Returns whether the decoder is currently processing silence. This reflects the internal
     * silence detection state of the decoder, which updates as audio is processed.
     * @remarks Always returns true, if the passed decoder is invalid.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Silent Flag", ToolTip = "Get the silent flag of a decoder", KeyWords = "IsSilent Silent"),
              Category = "Odin|Audio Pipeline")
    bool GetIsSilent() const;

    /**
     * Registers a callback to receive decoder audio events. The `filter` determines which event
     * types will trigger the callback, allowing selective handling.
     * @remarks Any previously registered callback is replaced.
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Register AudioEvent Handler", ToolTip = "Set OnAudioEventCallback of the decoder", DefaultToSelf = "UserData"),
              Category = "Odin|Audio Pipeline|Events")
    bool SetAudioEventHandler(UPARAM(meta = (DisplayName = "Filter", Bitmask, BitmaskEnum = "/Script/Odin.EOdinAudioEvents")) int32 EFilter);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinDecoderAudioEventCallbackDelegate, UOdinDecoder *, decoder, EOdinAudioEvents, filter);
    /**
     * A callback invoked when the decoder reports one or more audio events.
     *
     * @attention Please remember to start listening by calling SetAudioEventHandler!
     */
    UPROPERTY(BlueprintAssignable, Category = "Odin|Audio Pipeline|Events",
              meta = (ToolTip = "Invoked when the encoder reports an audio event. Don't forget to start listening by calling SetAudioEventHandler!"))
    FOdinDecoderAudioEventCallbackDelegate OnAudioEventCallbackBP;
    /**
     * Internal decoder callback hook to redirect incoming callback for audio events
     * @remarks This should only be changed if the underlying callback has to call a custom implementation of handling callbacks
     */
    void (*OdinDecoderEventCallbackFunc)(struct OdinDecoder *decoder, enum OdinAudioEvents events,
                                         void *user_data) = [](struct OdinDecoder *decoder, const OdinAudioEvents events, void *user_data) {
        TWeakObjectPtr<UOdinDecoder> data = static_cast<UObject *>(user_data)->IsA<UOdinDecoder>() ? static_cast<UOdinDecoder *>(user_data) : nullptr;
        HandleOdinAudioEventCallback(decoder, events, data);
    };

    /**
     * Returns a bitmask indicating the channels on which the most recently pushed voice packet
     * was transmitted.
     * @remarks If no packet has been processed yet, the value is `0`.
     */
    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Active ChannelMask", ToolTip = "Get the active channel mask of the decoder"),
              Category = "Odin|Audio Pipeline")
    int64 GetActiveChannelMask() const;

    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Positions", ToolTip = "Get the positions registered in the decoder"), Category = "Odin|Audio Pipeline")
    TArray<FOdinPosition> GetPositions(FOdinChannelMask ChannelMask) const;
    /**
     * Retrieves a block of processed audio samples from the decoder's buffer. The samples are
     * interleaved floating-point values in the range[-1, 1] and are written into the provided output buffer.
     * @param Samples   interleaved float buffer
     * @param bSilence   A flag is also set to indicate if the output is silent
     * @return samples count
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Pop Samples", ToolTip = "Get samples of the audio decoder pipeline (runs audio pipeline)"),
              Category = "Odin|Audio Pipeline")
    int32 Pop(TArray<float> &Samples, bool &bSilence) const;
    int32 Pop(float *Samples, int32 Count, bool *bSilence) const;
    /**
     * Pushes an incoming datagram to the specified decoder for processing.
     * @param Datagram audio byte array
     * @return datagram size
     */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Push Datagram", ToolTip = "Push datagram samples to the audio decoder pipeline buffer"),
              Category = "Odin|Audio Pipeline")
    int32 Push(const TArray<uint8> &Datagram) const;

    UPROPERTY(BlueprintReadOnly, Category = "Odin|Audio Pipeline")
    UOdinPipeline *Pipeline = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin|Channels")
    FOdinChannelMask ChannelMask;

    UPROPERTY(BlueprintReadOnly, Category = "Odin")
    int32 SampleRate = 48000;
    UPROPERTY(BlueprintReadOnly, Category = "Odin")
    bool bStereo = true;

    /**
     * Get internal handle
     * @remarks OdinDecoder operations are thread-safe and the same encoder handle may be accessed concurrently from multiple threads.
     * @return incomplete handle type or nullptr
     */
    inline OdinDecoder *GetNativeHandle() const
    {
        return IsValid(Handle) && Handle->IsValidLowLevel() ? reinterpret_cast<OdinDecoder *>(Handle->GetHandle()) : nullptr;
    }

    TWeakObjectPtr<UOdinHandle> GetHandle() const
    {
        return Handle;
    }

    /**
     * Replace internal handle object
     * @param NewHandle internal handle
     */
    void SetHandle(OdinDecoder *NewHandle);

  protected:
    virtual void BeginDestroy() override;

  private:
    UPROPERTY()
    UOdinHandle *Handle;
    static void  HandleOdinAudioEventCallback(OdinDecoder *DecoderHandle, const OdinAudioEvents Events, TWeakObjectPtr<UObject> UserData = nullptr);
};