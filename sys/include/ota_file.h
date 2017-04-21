/*
 * Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
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

#include "ota_slots.h"

#include "tweetnacl.h"
#include "hashes/sha256.h"
#include "crypto/aes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OTA_FILE_SIGN_SPACE
#define OTA_FILE_SIGN_SPACE     (0x80)  /* default value only */
#endif

#define OTA_FILE_HEADER_SPACE   (OTA_FW_HEADER_SPACE + OTA_FILE_SIGN_SPACE)

/**
 *  @brief OTA_FW_SIGN_LEN:
 *         length of the (encrypted) firmware signature.
 *         (32 Byte SHA256 + 16 Byte AES IV + 16 Byte AES Key + 32 Byte null padding)
 */
#define OTA_FILE_SIGN_LEN       (SHA256_DIGEST_LENGTH + AES_BLOCK_SIZE + AES_KEY_SIZE + crypto_box_ZEROBYTES)

/**
 *  @brief OTA_FW_META_MAGIC:
 *         magic number to identify the firmware metadata block.
 */
#define OTA_FW_FILE_MAGIC_H     (0x544f4952)    /* RIOT as hex, byte order swapped */
#define OTA_FW_FILE_MAGIC_L     (0x31305746)    /* FW01 as hex, byte order swapped */
#define OTA_FW_FILE_MAGIC_LEN   (8)

/**
 * @brief Structure of the file header
 * @{
 */
typedef struct OTA_File_header {
    uint32_t magic_h;
    uint32_t magic_l;
    uint8_t file_signature[OTA_FILE_SIGN_LEN];
#if (OTA_FILE_SIGN_SPACE > OTA_FILE_SIGN_LEN)
    uint8_t file_sign_spacer[OTA_FILE_SIGN_SPACE - OTA_FILE_SIGN_LEN];
#endif
    OTA_FW_header_t fw_header;
} OTA_File_header_t;
/** @} */

/**
 * @brief      Validate the OTA Update File
 *
 * @param[in]  file_address         The memory address, where the update file is
 *                                  located.
 *
 * @return     0 on success, 1 for an invalid signature or -1 on error
 */
int ota_file_validate_file(uint32_t file_address);

/**
 * @brief      Decrypt and write an update file to an internal FW slot.
 *             ota_file_validate_file() must be called before this function!
 *             This function erases the specified slot.
 *
 * @param[in]  file_address         The memory address, where the update file is
 *                                  located.
 * @param[in]  fw_slot              The FW slot index where the update file
 *                                  should be extracted to.
 *
 * @return     0 on success or -1 on error
 */
int ota_file_write_image(uint32_t file_address, uint8_t fw_slot);

#ifdef __cplusplus
}
#endif

#endif /* OTA_FILE_H */
/** @} */
