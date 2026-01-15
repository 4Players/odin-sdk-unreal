/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "OdinAudio/OdinChannelMask.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinFunctionLibrary.generated.h"

class UOdinDecoder;
class UOdinAudioCapture;
class UOdinPipeline;
class UOdinRoom;
class UOdinEncoder;
class UAudioGenerator;

UCLASS()
class ODIN_API UOdinFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

  public:
    UFUNCTION(BlueprintPure, Category = "Odin|Authentication")
    static FString GenerateAccessKey();

    UFUNCTION(BlueprintPure, Category = "Odin|Error Handling")
    static FString FormatOdinError(EOdinError Code, bool bUETrace);

    UFUNCTION(BlueprintPure, Category = "Odin|Custom Data")
    static FString BytesToString(const TArray<uint8>& Data);

    UFUNCTION(BlueprintPure, Category = "Odin|Custom Data")
    static void OdinStringToBytes(const FString& Str, TArray<uint8>& Buffer);

    UFUNCTION(BlueprintPure, Category = "Odin|Custom Data")
    static void OdinHexStringToBytes(const FString& Str, TArray<uint8>& Buffer);

    UFUNCTION(BlueprintPure, Category = "Odin|Custom Data")
    static void OdinBytesToString(const TArray<uint8>& Buffer, FString& Str);

    UFUNCTION(BlueprintPure, Category = "Odin|Custom Data")
    static void OdinBytesToHexString(const TArray<uint8>& Buffer, FString& Str);

    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture", meta = (WorldContext = "WorldContextObject"))
    static UOdinAudioCapture* CreateOdinAudioCapture(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Odin|Audio Capture",
              meta = (WorldContext = "WorldContextObject",
                      ToolTip      = "Creates encoder with calculated samplerates and clamped channels (or use \"Construct Encoder\")"))
    static UOdinEncoder* CreateOdinEncoderFromGenerator(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& OdinRoom,
                                                        UPARAM(ref) UAudioGenerator*& AudioGenerator);

    static void LinkEncoderToRoom(UOdinEncoder* Encoder, UOdinRoom* Room);
    static void UnlinkEncoderFromRoom(UOdinEncoder* Encoder);

    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Register Decoder to Peer", ToolTip = "Register Decoder to Peer for a specific Odin Room.", Keywords = "Link"),
              Category = "Odin|Audio Pipeline")
    static void RegisterDecoder(UOdinDecoder* Decoder, UOdinRoom* Room, int64 PeerId);

    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "Get Decoders for Peer",
                      ToolTip     = "Retrieves all decoders that have been registered for this room with the given peer id."),
              Category = "Odin|Audio Pipeline")
    static TArray<UOdinDecoder*> GetDecodersForPeer(UOdinRoom* Room, int64 PeerId);

    UFUNCTION(BlueprintPure, meta = (ToolTip = "Retrieves all Odin Rooms that have been created with the given Room Id."), Category = "Odin|Audio Pipeline")
    static TArray<UOdinRoom*> GetRoomsByName(const FString& RoomId);
    /**
     * Deregisters the specified decoder from all active connections.
     * @param Decoder The decoder to deregister.
     */
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Deregister Decoder From All", ToolTip = "Deregisters a Decoder from all connections it receives audio from.",
                      Keywords = "Unlink"),
              Category = "Odin|Audio Pipeline")
    static void DeregisterDecoderFromAllConnections(UOdinDecoder* Decoder);

    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "To Vector", CompactNodeTitle = "->", ScriptMethod = "OdinPosition", Keywords = "cast convert", BlueprintAutocast),
              Category = "Odin|Conversions")
    static FVector Conv_OdinPositionToVector(FOdinPosition InPosition);

    UFUNCTION(BlueprintPure,
              meta     = (DisplayName = "To Odin Position", CompactNodeTitle = "->", ScriptMethod = "Vector", Keywords = "cast convert", BlueprintAutocast),
              Category = "Odin|Conversions")
    static FOdinPosition Conv_VectorToOdinPosition(const FVector& InPosition);

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels", meta = (Keywords = "Make Channel Mask"))
    static FOdinChannelMask CreateChannelMask(const TMap<int32, bool>& Channels, bool bDefaultValue);

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels")
    static void SetChannels(UPARAM(ref) FOdinChannelMask& Mask, const TMap<int32, bool>& Channels);

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels")
    static void SetChannelsEnabled(UPARAM(ref) FOdinChannelMask& Mask, const TArray<int32>& EnabledChannels);

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels")
    static void SetChannelsDisabled(UPARAM(ref) FOdinChannelMask& Mask, const TArray<int32>& DisabledChannels);

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels", meta = (Keywords = "Make Channel Mask From Enabled"))
    static FOdinChannelMask CreateChannelMaskFromEnabled(const TArray<int32>& EnabledChannels);

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels", meta = (Keywords = "Make Channel Mask From Disabled"))
    static FOdinChannelMask CreateChannelMaskFromDisabled(const TArray<int32>& DisabledChannels);

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels")
    static void SetChannelInMask(UPARAM(ref) FOdinChannelMask& Mask, int32 ChannelIndex, bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Odin|Channels")
    static bool IsChannelEnabledInMask(const FOdinChannelMask& Mask, const int32 ChannelIndex);

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels", meta = (Keywords = "Make Full Channel Mask"))
    static FOdinChannelMask CreateFullMask();

    UFUNCTION(BlueprintCallable, Category = "Odin|Channels", meta = (Keywords = "Make Empty Channel Mask"))
    static FOdinChannelMask CreateEmptyMask();

    UFUNCTION(BlueprintPure, Category = "Odin|Audio Pipeline|Events", meta = (CompactNodeTitle = "In Filter"), meta = (Keywords = "Audio Event Filter"))
    static bool DoesAudioEventMatchFilter(EOdinAudioEvents                                                                                      Event,
                                          UPARAM(meta = (DisplayName = "Filter", Bitmask, BitmaskEnum = "/Script/Odin.EOdinAudioEvents")) int32 Filter);
};