/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */
#include "OdinAudio/OdinChannelMask.h"

FOdinChannelMask::FOdinChannelMask()
    : FOdinChannelMask(0)
{
}

FOdinChannelMask::FOdinChannelMask(const uint64 InitialMask)
{
    SetChannelMask(InitialMask);
}

FOdinChannelMask FOdinChannelMask::CreateFull()
{
    return FOdinChannelMask(~static_cast<uint64>(0));
}

FOdinChannelMask FOdinChannelMask::CreateEmpty()
{
    return FOdinChannelMask();
}

bool FOdinChannelMask::IsSet(uint64 mask) const
{
    return ChannelMask & mask;
}

uint64 FOdinChannelMask::Set(int32 index, bool bSet)
{
    if (index > 63) {
        return ChannelMask;
    }

    ChannelMask = bSet ? (ChannelMask | (static_cast<uint64>(1) << index)) : (ChannelMask & (~(static_cast<uint64>(1) << index)));
    return ChannelMask;
}

uint64 FOdinChannelMask::Flip(int32 index)
{
    if (index > 63)
        return ChannelMask;

    return ChannelMask ^= (static_cast<uint64_t>(1) << index);
}

uint64 FOdinChannelMask::GetChannelMask() const
{
    return ChannelMask;
}

uint64 FOdinChannelMask::SetChannelMask(uint64 mask)
{
    return ChannelMask = mask;
}

uint64 FOdinChannelMask::SetChannelMask(uint32 lower, uint32 higher = 0)
{
    const uint64 Mask = static_cast<uint64_t>(higher) << 32 | lower;
    return SetChannelMask(Mask);
}