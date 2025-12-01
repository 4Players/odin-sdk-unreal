/* Copyright (c) 2022-2025 4Players GmbH. All rights reserved. */

#include "OdinNative/OdinNativeHandle.h"

UOdinHandle::UOdinHandle(OdinDecoder *handle)
    : UOdinHandle((void *)handle)
{
}
UOdinHandle::UOdinHandle(OdinEncoder *handle)
    : UOdinHandle((void *)handle)
{
}
UOdinHandle::UOdinHandle(const OdinPipeline *handle)
    : UOdinHandle((void *)handle)
{
}
UOdinHandle::UOdinHandle(OdinRoom *handle)
    : UOdinHandle((void *)handle)
{
}
UOdinHandle::UOdinHandle(OdinTokenGenerator *handle)
    : UOdinHandle((void *)handle)
{
}
UOdinHandle::UOdinHandle(OdinCipher *handle)
    : UOdinHandle((void *)handle)
{
}