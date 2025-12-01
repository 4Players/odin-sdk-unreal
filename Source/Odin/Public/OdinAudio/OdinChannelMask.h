/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
#pragma once

#include "OdinChannelMask.generated.h"

USTRUCT(BlueprintType)
struct FOdinChannelMask {
    GENERATED_BODY()

    FOdinChannelMask();

    FOdinChannelMask(uint64 InitialMask);

    static FOdinChannelMask CreateFull();
    static FOdinChannelMask CreateEmpty();

    bool operator[](int32 index) const
    {
        if (index >= 64)
            return false;
        return ChannelMask & (static_cast<uint64_t>(1) << index);
    }

    operator uint64() const
    {
        return ChannelMask;
    }

    bool   IsSet(uint64 mask) const;
    uint64 Set(int32 index, bool set);
    uint64 Flip(int32 index);
    uint64 GetChannelMask() const;
    uint64 SetChannelMask(uint64 mask);
    uint64 SetChannelMask(uint32 lower, uint32 higher);

  private:
    uint64 ChannelMask = 0;
};