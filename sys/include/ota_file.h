/*
 * Copyright (c) 2017, Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * @defgroup    sys_ota_file Firmware update file management
 * @ingroup     sys
 * @brief       Management for OTA firmware update files
 * @{
 *
 * @file
 * @brief       OTA Update File Read and Verification
 *
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 */

#ifndef OTA_FILE_H
#define OTA_FILE_H

#include "hashes/sha256.h"
#include "ota_slots.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief OTA_FW_SIGN_LEN:
 *         length of the (encrypted) firmware signature.
 */
#define OTA_FILE_SIGN_LEN   88      /* 32 Byte SHA256 + 24 Byte nonce + 32 Byte null padding */

/*
 * TODO_JB
 */

#ifdef __cplusplus
}
#endif

#endif /* OTA_FILE_H */
/** @} */
