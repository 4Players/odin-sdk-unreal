/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "OdinCore/include/odin.h"
#include "UObject/Object.h"

#include "OdinNativeHandle.generated.h"

/**
 * UOdinHandle an internal handle wrapper to represent either incomplete OdinDecoder, OdinEncoder, OdinPipeline, ... or OdinRoom handle.
 * Handles set are arbitrary and have to be mapped by bookkeeping or any other ways.
 */
UCLASS(BlueprintType)
class ODIN_API UOdinHandle : public UObject
{
    GENERATED_BODY()

  public:
    inline void *GetHandle() const
    {
        return handle_;
    }

    inline void SetHandle(void *handle)
    {
        handle_ = handle;
    }

    inline void Invalidate()
    {
        this->handle_ = nullptr;
    }

    /**
     * UOdinHandle represents a decoder for media streams from remote voice chat clients, which encapsulates all
     * the components required to process incoming audio streams. It includes an egress resampler for
     * sample rate conversion, an Opus decoder for decompressing audio data, and a customizable audio
     * pipeline that enables the application of effects to modify the raw audio samples.
     */
    UOdinHandle(OdinDecoder *handle);
    /**
     * UOdinHandle represents an encoder for local media streams, which encapsulates the components required to
     * process outgoing audio streams captured from local sources (e.g. a microphone). It includes
     * an ingress resampler for sample rate conversion, an Opus encoder for compressing the audio
     * data and a customizable audio pipeline that allows the application of effects to modify the
     * raw audio samples before transmission.
     */
    UOdinHandle(OdinEncoder *handle);
    /**
     * UOdinHandle represents a pipeline; a highly dynamic audio processing chain that manages a thread-safe collection of filters like
     * voice activity detection, echo cancellation, noise suppression and even custom effects.
     * This allows sequential processing and real-time modification of audio streams through operations
     * like insertion, removal, reordering and configuration updates.
     */
    UOdinHandle(const OdinPipeline *handle);
    /**
     * UOdinHandle represents a room; an opaque type representing an ODIN room handle, which is managed by the underlying connection.
     * This abstraction provides a high-level interface for joining rooms, managing persistent state
     * and sending/receiving data, making it easier to integrate room-based interactions into your
     * application.
     */
    UOdinHandle(OdinRoom *handle);
    /**
     * UOdinHandle represents a token generator; a struct for generating ODIN tokens, employed for generating signed room tokens predicated on
     * an access key. Be aware that access keys serve as your unique authentication keys, requisite for
     * generating room tokens to access the ODIN server network. To ensure your security, it's strongly
     * recommended that you _NEVER_ embed an access key within your client code, and instead generate
     * room tokens on a server.
     */
    UOdinHandle(OdinTokenGenerator *handle);
    /**
     * Optional, pluggable encryption module for room communications. A cipher can be attached to an
     * ODIN room handle on creation to enable customizable, end-to-end encryption (E2EE). When enabled,
     * it intercepts data right before transmission and immediately after reception, allowing custom
     * processing of datagrams, messages and custom peer user data. The structure provides a suite of
     * callback functions for initialization, cleanup, event handling and encryption/decryption tasks,
     * along with parameters to adjust for any additional capacity overhead.
     */
    UOdinHandle(OdinCipher *handle);
    /**
     * Uninitialized handle
     */
    UOdinHandle()
    {
        Invalidate();
    }

  protected:
    UOdinHandle(void *handle)
    {
        handle_ = handle;
    }

  private:
    void *handle_ = nullptr;
};