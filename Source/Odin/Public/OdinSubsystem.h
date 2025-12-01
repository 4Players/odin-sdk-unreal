/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "OdinAudio/OdinAudioPushDataThread.h"
#include "OdinAudio/OdinDatagramProcessingThread.h"
#include "OdinCore/include/odin.h"
#include "Subsystems/EngineSubsystem.h"
#include "OdinAudio/OdinDecoder.h"

#include "OdinSubsystem.generated.h"

class UOdinDecoder;
class UOdinEncoder;
class UOdinRoom;

/**
 * @class UOdinSubsystem
 * @brief A subsystem for managing rooms, encoders, and decoders in Odin.
 *
 * Provides global access to Odin-related operations including room registration,
 * audio data processing, and encoder/decoder management. This class extends
 * UEngineSubsystem to tie its lifecycle to the engine's initialization and
 * deinitialization.
 */
UCLASS()
class ODIN_API UOdinSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

  public:
    static UOdinSubsystem* Get();
    static bool            GlobalIsRoomValid(const OdinRoom* Handle);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void                              LinkEncoder(TWeakObjectPtr<UOdinEncoder> Encoder, TWeakObjectPtr<UOdinRoom> TargetRoom);
    void                              UnlinkEncoder(TWeakObjectPtr<UOdinEncoder> Encoder);
    void                              UnlinkEncoder(OdinEncoder* Encoder);
    void                              PushAudioToEncoder(OdinEncoder* Encoder, TArray<float>&& Audio);
    void                              RegisterRoom(OdinRoom* Handle, UOdinRoom* Room);
    void                              DeregisterRoom(OdinRoom* Handle);
    void                              SwapRoomHandle(OdinRoom* OldHandle, OdinRoom* NewHandle);
    TWeakObjectPtr<UOdinRoom>         GetRoomByHandle(OdinRoom* Handle) const;
    TWeakObjectPtr<UOdinDecoder>      GetDecoderByHandle(OdinDecoder* Handle) const;
    TArray<TWeakObjectPtr<UOdinRoom>> GetRoomsByName(const FString& RoomId) const;
    bool                              IsRoomRegistered(const OdinRoom* Handle) const;

    void                  LinkDecoderToPeer(const UOdinDecoder* Decoder, OdinRoom* TargetRoom, uint32 PeerId);
    void                  RegisterDecoderObject(const TWeakObjectPtr<UOdinDecoder> Decoder);
    void                  DeregisterDecoder(const UOdinDecoder* OdinDecoder);
    void                  DeregisterDecoder(const OdinDecoder* Handle);
    TArray<OdinDecoder*>  GetDecoderHandlesFor(OdinRoom* TargetRoom, uint32 PeerId) const;
    TArray<UOdinDecoder*> GetDecodersFor(OdinRoom* TargetRoom, uint32 PeerId) const;

    void HandleDatagram(OdinRoom* RoomHandle, uint32 PeerId, uint64 ChannelMask, uint32 SsrcId, TArray<uint8>&& Datagram);

  protected:
    mutable FCriticalSection                   RoomsCS;
    TMap<OdinRoom*, TWeakObjectPtr<UOdinRoom>> RegisteredRooms;

    mutable FCriticalSection                         DecoderObjectsCS;
    TMap<OdinDecoder*, TWeakObjectPtr<UOdinDecoder>> DecoderObjects;

    TUniquePtr<FOdinAudioPushDataThread>      PushDataThread;
    TUniquePtr<FOdinDatagramProcessingThread> DatagramProcessingThread;
};