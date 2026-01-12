/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

#include "OdinFunctionLibrary.h"
#include "OdinAudio/OdinDecoder.h"
#include "OdinAudio/OdinEncoder.h"
#include "OdinRoom.h"
#include "OdinNative/OdinNativeBlueprint.h"
#include "OdinSubsystem.h"
#include "OdinVoice.h"
#include "Kismet/KismetNodeHelperLibrary.h"
#include "OdinAudio/OdinAudioCapture.h"

FString UOdinFunctionLibrary::GenerateAccessKey()
{
    OdinTokenGenerator* generator;
    TArray<char>        KeyChars;
    KeyChars.SetNumZeroed(1024 + 1);
    if (OdinError::ODIN_ERROR_SUCCESS == odin_token_generator_create(NULL, &generator)) {
        uint32_t key_length = 1024;
        odin_token_generator_get_access_key(generator, &KeyChars[0], &key_length);
        odin_token_generator_free(generator);
        return FString(StringCast<UTF8CHAR>(reinterpret_cast<UTF8CHAR*>(KeyChars.GetData()), key_length));
    }
    return "";
}

FString UOdinFunctionLibrary::FormatOdinError(EOdinError Code, bool bUETrace)
{
    if (bUETrace) {
        FDebug::DumpStackTraceToLog(ELogVerbosity::All);
    }

    const UEnum* OdinUEnumPtr = StaticEnum<EOdinError>();
    FString      Tag          = UKismetNodeHelperLibrary::GetEnumeratorUserFriendlyName(OdinUEnumPtr, static_cast<uint8>(Code));

    Tag.Append(": ");
    const auto OdinNativeError = odin_error_get_last_error();
    FString    ErrorString     = FString(OdinNativeError);
    return Tag.Append(ErrorString);
}

FString UOdinFunctionLibrary::BytesToString(const TArray<uint8>& Data)
{
    return ::BytesToString(Data.GetData(), Data.Num());
}

void UOdinFunctionLibrary::OdinStringToBytes(const FString& Input, TArray<uint8>& Buffer)
{
    if (0 == Input.Len()) {
        Buffer.Empty(); // '\0' == NULL
        return;
    }

    const auto   astr = StringCast<ANSICHAR>(*Input);
    const uint32 size = astr.Length() + 1; // include '\0'
    Buffer.AddUninitialized(size);
    FMemory::Memcpy(Buffer.GetData(), astr.Get(), size);
}

void UOdinFunctionLibrary::OdinHexStringToBytes(const FString& Input, TArray<uint8>& Buffer)
{
    if (0 == Input.Len()) {
        Buffer.Empty();
        return;
    }
    const uint32 size = Input.Len(); // exclude '\0'
    Buffer.AddUninitialized(size / 2);
    ::HexToBytes(Input, Buffer.GetData());
}

void UOdinFunctionLibrary::OdinBytesToString(const TArray<uint8>& Buffer, FString& Str)
{
    if (0 == Buffer.Num()) {
        Str = "";
        return;
    }

    auto strPtr = StringCast<UTF8CHAR>((UTF8CHAR*)Buffer.GetData(), Buffer.Num());
    Str         = FString(strPtr);
}

void UOdinFunctionLibrary::OdinBytesToHexString(const TArray<uint8>& Buffer, FString& Str)
{
    if (0 == Buffer.Num()) {
        Str = "";
        return;
    }
    Str = ::BytesToHex(Buffer.GetData(), Buffer.Num());
}

UOdinAudioCapture* UOdinFunctionLibrary::CreateOdinAudioCapture(UObject* WorldContextObject)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UOdinFunctionLibrary::CreateOdinAudioCapture)

    UOdinAudioCapture* OdinAudioCapture;
    if (IsValid(WorldContextObject)) {
        OdinAudioCapture = NewObject<UOdinAudioCapture>(WorldContextObject);
    } else {
        OdinAudioCapture = NewObject<UOdinAudioCapture>();
        ODIN_LOG(Warning, "No World Context provided when creating Odin Audio Capture. Audio Capture "
                          "will not be able to react to capture devices being removed.");
    }
    if (OdinAudioCapture->RestartCapturing(false)) {
        return OdinAudioCapture;
    }
    ODIN_LOG(Error, "Failed to open a default audio stream to the audio capture device.");
    return nullptr;
}

UOdinEncoder* UOdinFunctionLibrary::CreateOdinEncoderFromGenerator(UObject* WorldContextObject, UPARAM(ref) UOdinRoom*& OdinRoom,
                                                                   UPARAM(ref) UAudioGenerator*& AudioGenerator)
{
    if (!IsValid(AudioGenerator)) {
        ODIN_LOG(Error, "No UAudioGenerator provided when creating Odin Encoder.");
        return nullptr;
    }

    if (!IsValid(OdinRoom)) {
        ODIN_LOG(Error, "Provided OdinRoom pointer is invalid.");
        return nullptr;
    }

    bool  bStereo             = AudioGenerator->GetNumChannels() >= 2;
    int32 GeneratorSampleRate = AudioGenerator->GetSampleRate();

    UOdinEncoder* OdinEncoder = UOdinEncoder::ConstructEncoder(WorldContextObject, OdinRoom->GetOwnPeerId(), GeneratorSampleRate, bStereo);
    OdinEncoder->SetAudioGenerator(AudioGenerator);
    LinkEncoderToRoom(OdinEncoder, OdinRoom);
    return OdinEncoder;
}

void UOdinFunctionLibrary::LinkEncoderToRoom(UOdinEncoder* Encoder, UOdinRoom* Room)
{
    if (UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {
        OdinSubsystem->LinkEncoder(Encoder, Room);
    }
}

void UOdinFunctionLibrary::UnlinkEncoderFromRoom(UOdinEncoder* Encoder)
{
    if (UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {
        OdinSubsystem->UnlinkEncoder(Encoder);
    }
}

void UOdinFunctionLibrary::RegisterDecoder(UOdinDecoder* Decoder, UOdinRoom* Room, int64 PeerId)
{
    if (!Decoder) {
        ODIN_LOG(Warning, TEXT("Tried registering an invalid Odin Decoder, aborting."))
        return;
    }

    if (!Room) {
        ODIN_LOG(Warning, TEXT("Tried registering a valid Odin Decoder to an invalid Odin Room, aborting."))
        return;
    }

    if (UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {

        OdinSubsystem->LinkDecoderToPeer(Decoder, Room->GetHandle(), PeerId);
    }
}

TArray<UOdinDecoder*> UOdinFunctionLibrary::GetDecodersForPeer(UOdinRoom* Room, const int64 PeerId)
{
    TArray<UOdinDecoder*> Result;
    if (!Room) {
        ODIN_LOG(Error, "Tried retrieving decoders with invalid room pointer.");
        return Result;
    }
    if (UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {

        Result = OdinSubsystem->GetDecodersFor(Room->GetHandle(), PeerId);
    }
    return Result;
}

TArray<UOdinRoom*> UOdinFunctionLibrary::GetRoomsByName(const FString& RoomId)
{
    TArray<UOdinRoom*> FoundRooms;
    if (const UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {
        for (TWeakObjectPtr<UOdinRoom> Room : OdinSubsystem->GetRoomsByName(RoomId)) {
            if (Room.IsValid()) {
                FoundRooms.Add(Room.Get());
            }
        }
    }
    return FoundRooms;
}

void UOdinFunctionLibrary::DeregisterDecoderFromAllConnections(UOdinDecoder* Decoder)
{
    if (UOdinSubsystem* OdinSubsystem = UOdinSubsystem::Get()) {

        OdinSubsystem->DeregisterDecoder(Decoder);
    }
}

FVector UOdinFunctionLibrary::Conv_OdinPositionToVector(const FOdinPosition InPosition)
{
    return InPosition;
}

FOdinPosition UOdinFunctionLibrary::Conv_VectorToOdinPosition(const FVector& InPosition)
{
    return InPosition;
}

FOdinChannelMask UOdinFunctionLibrary::CreateChannelMask(const TMap<int32, bool>& Channels, bool bDefaultValue)
{
    FOdinChannelMask Mask = bDefaultValue ? FOdinChannelMask::CreateFull() : FOdinChannelMask::CreateEmpty();
    SetChannels(Mask, Channels);
    return Mask;
}

void UOdinFunctionLibrary::SetChannels(FOdinChannelMask& Mask, const TMap<int32, bool>& Channels)
{
    for (const TPair<int32, bool> Pair : Channels) {
        SetChannelInMask(Mask, Pair.Key, Pair.Value);
    }
}

void UOdinFunctionLibrary::SetChannelsEnabled(FOdinChannelMask& Mask, const TArray<int32>& EnabledChannels)
{
    for (const int32 Index : EnabledChannels) {
        SetChannelInMask(Mask, Index, true);
    }
}

void UOdinFunctionLibrary::SetChannelsDisabled(FOdinChannelMask& Mask, const TArray<int32>& DisabledChannels)
{
    for (const int32 Index : DisabledChannels) {
        SetChannelInMask(Mask, Index, false);
    }
}

FOdinChannelMask UOdinFunctionLibrary::CreateChannelMaskFromEnabled(const TArray<int32>& EnabledChannels)
{
    FOdinChannelMask Mask = FOdinChannelMask::CreateEmpty();
    for (const int32 ChannelIndex : EnabledChannels) {
        SetChannelInMask(Mask, ChannelIndex, true);
    }
    return Mask;
}

FOdinChannelMask UOdinFunctionLibrary::CreateChannelMaskFromDisabled(const TArray<int32>& DisabledChannels)
{
    FOdinChannelMask Mask = FOdinChannelMask::CreateFull();
    for (const int32 ChannelIndex : DisabledChannels) {
        SetChannelInMask(Mask, ChannelIndex, false);
    }
    return Mask;
}

void UOdinFunctionLibrary::SetChannelInMask(FOdinChannelMask& Mask, const int32 ChannelIndex, const bool bEnabled)
{
    Mask.Set(ChannelIndex, bEnabled);
}

bool UOdinFunctionLibrary::IsChannelEnabledInMask(const FOdinChannelMask& Mask, const int32 ChannelIndex)
{
    return Mask.IsSet(ChannelIndex);
}

FOdinChannelMask UOdinFunctionLibrary::CreateFullMask()
{
    return FOdinChannelMask::CreateFull();
}

FOdinChannelMask UOdinFunctionLibrary::CreateEmptyMask()
{
    return FOdinChannelMask::CreateEmpty();
}

bool UOdinFunctionLibrary::DoesAudioEventMatchFilter(EOdinAudioEvents Event, const int32 Filter)
{
    return (static_cast<int32>(Event) & Filter) != 0;
}