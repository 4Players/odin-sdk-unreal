/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinNative/OdinNativeRpc.h"

// event response
const FString FOdinRoomStatusChanged::Name = FString("RoomStatusChanged");
const FString FOdinNewReconnectToken::Name = FString("NewReconnectToken");
const FString FOdinMessageReceived::Name   = FString("MessageReceived");
const FString FOdinJoined::Name            = FString("Joined");
const FString FOdinPeerJoined::Name        = FString("PeerJoined");
const FString FOdinPeerChanged::Name       = FString("PeerChanged");
const FString FOdinPeerLeft::Name          = FString("PeerLeft");

//
const FString FOdinRoomStatusChanged::JoinedStatus  = FString("joined");
const FString FOdinRoomStatusChanged::ClosedStatus  = FString("closed");
const FString FOdinRoomStatusChanged::JoiningStatus = FString("joining");

// rpc request
const FString FOdinChangeSelf::Name = FString("ChangeSelf");

FOdinSetChannelMasks::FOdinSetChannelMasks() {}

FOdinSetChannelMasks::FOdinSetChannelMasks(TMap<int64, uint64> InMasks, bool bReset)
    : FOdinSetChannelMasks()
{
    for (auto Mask : InMasks) {
        masks.Add(Mask.Key, Mask.Value);
    }
    reset = bReset;
}

const FString FOdinSetChannelMasks::Name = FString("SetChannelMasks");
const FString FOdinSendMessage::Name     = FString("SendMessage");
