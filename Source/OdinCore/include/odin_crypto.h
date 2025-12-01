/* Copyright (c) 4Players GmbH. All rights reserved. */

#pragma once

/** @file odin_crypto.h */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "odin.h"

#define ODIN_CRYPTO_VERSION "2.0.0"

#define OdinCipherImpl_ADDITIONAL_CAPACITY_DATAGRAM (TAG_SIZE + 12)

enum OdinCryptoPeerStatus
#ifdef __cplusplus
  : int32_t
#endif // __cplusplus
 {
    ODIN_CRYPTO_PEER_STATUS_PASSWORD_MISSMATCH = -1,
    ODIN_CRYPTO_PEER_STATUS_UNKNOWN = 0,
    ODIN_CRYPTO_PEER_STATUS_UNENCRYPTED = 1,
    ODIN_CRYPTO_PEER_STATUS_ENCRYPTED = 2,
};
#ifndef __cplusplus
typedef int32_t OdinCryptoPeerStatus;
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

OdinCipher *odin_crypto_create(const char *version);

OdinCryptoPeerStatus odin_crypto_get_peer_status(OdinCipher *cipher, uint32_t peer_id);

int32_t odin_crypto_set_password(OdinCipher *cipher,
                                 const uint8_t *password,
                                 uint32_t password_length);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
