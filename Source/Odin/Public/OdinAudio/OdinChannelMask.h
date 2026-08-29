/* Copyright (c) 2020-2026 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinChannelMask.generated.h"

USTRUCT(BlueprintType)
struct ODIN_API FOdinChannelMask {
    GENERATED_BODY()

    FOdinChannelMask();

    FOdinChannelMask(uint64 InitialMask);

    /**
     * Creates a mask with all 64 channels enabled.
     * @remarks Do not pass a full mask to UOdinEncoder::SetPosition: an encoder can only hold
     * positions for 12 channels, so setting a position for all 64 channels fails with
     * ODIN_ERROR_AUDIO_POSITION_LIMIT_REACHED. Build a mask containing just the channels that are
     * actually transmitted on instead (usually only channel 0).
     */
    static FOdinChannelMask CreateFull();
    /** Creates a mask with no channels enabled. */
    static FOdinChannelMask CreateEmpty();

    bool operator[](int32 index) const
    {
        if (index >= 64)
            return false;
        return ChannelMask & (static_cast<uint64_t>(1) << index);
    }

    operator uint64() const
    { return ChannelMask; }

    bool   IsSet(uint64 mask) const;
    uint64 Set(int32 index, bool set);
    uint64 Flip(int32 index);
    uint64 GetChannelMask() const;
    uint64 SetChannelMask(uint64 mask);
    uint64 SetChannelMask(uint32 lower, uint32 higher);

  private:
    uint64 ChannelMask = 0;
};