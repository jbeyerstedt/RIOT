/*
 * Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @file
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 */

#include <stdio.h>
#include <string.h>

#ifndef OTA_UPDATE
#define OTA_UPDATE      /* should be set by build script */
#endif

#include "ota_file.h"
#include "cpu_conf.h"
#if !defined(FLASH_SECTORS)        /* defined by Makefile.include of the CPU */
#include "periph/flashpage.h"
#else
#include "periph/flashsector.h"
#endif

#include "crypto/modes/cbc.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define BUF_SIZE        (AES_BLOCK_SIZE * 4)

static uint8_t is_aeskey_set = 0;
static uint8_t aes_key[AES_KEY_SIZE];
static uint8_t aes_iv[AES_BLOCK_SIZE];

static uint8_t buffer[BUF_SIZE];

/**
 * @brief      Read internal flash to a buffer at specific address.
 *
 * @param[in]  address - Address to be read.
 * @param[in]  count - count in bytes.
 *
 * @param[out] data_buffer - The buffer filled with the read information.
 *
 */
static void int_flash_read(uint8_t *data_buffer, uint32_t address, uint32_t count)
{
    uint8_t *read_addres = (uint8_t *)address;

    while (count--) {
        *data_buffer++ = *read_addres++;
    }
}

int ota_file_validate_file(uint32_t file_address)
{
    OTA_File_header_t *file_header = (OTA_File_header_t *)(OTA_FILE_SLOT);
    OTA_FW_metadata_t *fw_metadata = &file_header->fw_header.fw_metadata;
    OTA_FW_metadata_t slot_metadata;
    uint32_t address;
    uint16_t rest;
    sha256_context_t sha256_ctx;
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint8_t sign_hash[OTA_FILE_SIGN_LEN];
    uint8_t n[crypto_stream_NONCEBYTES];
    int parts = 0;

    DEBUG("[ota_file] Validating firmware update file with FW version %d\n", fw_metadata->fw_vers);

    /* check magic numbers first */
    if ((file_header->magic_h != (uint32_t)OTA_FW_FILE_MAGIC_H) || (file_header->magic_l != (uint32_t)OTA_FW_FILE_MAGIC_L)) {
        return 1;
    }
    if (OTA_FW_META_MAGIC != fw_metadata->magic) {
        return 1;
    }

    /* check, if the HW_ID of the image is suitable */
    uint64_t hw_id = HW_ID;
    for (int i = 0; i < sizeof(fw_metadata->hw_id); i++) {
        if ((uint8_t)(hw_id >> (i * 8)) != fw_metadata->hw_id[i]) {
            return 1;
        }
    }

    /* check, if the fw_vers is higher then the own fw_vers */
    ota_slots_get_int_slot_metadata(FW_SLOT, &slot_metadata);
    if (fw_metadata->fw_vers <= slot_metadata.fw_vers) {
        return 1;
    }

    /** check file signature **/
    /* calculate hash of metadata section and encrypted firmware binary */
    address = (uint32_t)&file_header->fw_header;
    uint32_t encrypted_size = fw_metadata->size;
    if ((encrypted_size % AES_BLOCK_SIZE) > 0) {
        encrypted_size += AES_BLOCK_SIZE - fw_metadata->size % AES_BLOCK_SIZE;
    }
    parts = (encrypted_size + OTA_FW_HEADER_SPACE) / sizeof(buffer);
    rest = (encrypted_size + OTA_FW_HEADER_SPACE) % sizeof(buffer);
    sha256_init(&sha256_ctx);
    while (parts) {
        int_flash_read(buffer, address, sizeof(buffer));
        sha256_update(&sha256_ctx, buffer, sizeof(buffer));
        address += sizeof(buffer);
        parts--;
    }
    int_flash_read(buffer, address, rest);
    sha256_update(&sha256_ctx, buffer, rest);
    sha256_final(&sha256_ctx, hash);

    /* open the crypto_box to extract the signed hash and compare hash values */
    uint8_t *fw_signature = (uint8_t *)(&file_header->file_signature);
    memset(sign_hash, 0, sizeof(sign_hash));
    memset(n, 0, sizeof(n));

    int res = crypto_box_open(sign_hash, fw_signature, OTA_FILE_SIGN_LEN, n, server_pkey, firmware_skey);
    if (res) {
        printf("[ota_file] ERROR decryption failed.\n");
        return -1;
    }
    else {
        DEBUG("[ota_file] crypto_box decryption successful! verifying...\n");
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            if (hash[i] != (sign_hash[i + crypto_box_ZEROBYTES])) {
                printf("[ota_file] INFO incorrect decrypted hash!\n");
                return 1;
            }
        }
    }

    printf("[ota_file] INFO update file successfully validated\n");

    /* copy aes_iv and aes_key from signature */
    for (int i = 0; i < sizeof(aes_iv); i++) {
        aes_iv[i] = sign_hash[i + (crypto_box_ZEROBYTES + SHA256_DIGEST_LENGTH)];
    }
    for (int i = 0; i < sizeof(aes_key); i++) {
        aes_key[i] = sign_hash[i + (crypto_box_ZEROBYTES + SHA256_DIGEST_LENGTH + AES_BLOCK_SIZE)];
    }
    is_aeskey_set = 1;
    return 0;
}

int ota_file_write_image(uint32_t file_address, uint8_t fw_slot)
{
    uint32_t fw_slot_base_addr;
    uint8_t slot_sector;
    OTA_File_header_t *file_header = (OTA_File_header_t *)(OTA_FILE_SLOT);

    /*
     *  set up AES128 decryption
     */
    cipher_t cipher_ctx;

    if (cipher_init(&cipher_ctx, CIPHER_AES_128, aes_key, AES_KEY_SIZE) < 0) {
        printf("[ota_file] ERROR cipher init failed!\n");
        return -1;
    }

    printf("[ota_file] INFO writing update file (FW vers. %d) to FW slot %d\n",
           file_header->fw_header.fw_metadata.fw_vers, fw_slot);

    /* check some parameters */
    if (fw_slot > MAX_FW_SLOTS || fw_slot == 0) {
        printf("[ota_slots] ERROR FW slot not valid, should be <= %d and > 0\n",
               MAX_FW_SLOTS);
        return -1;
    }

    if (0 == is_aeskey_set) {
        printf("[ota_slots] ERROR call ota_file_validate_file() first!");
        return -1;
    }

    /* get information about the update */
    uint32_t fw_binary_size = file_header->fw_header.fw_metadata.size;

    /* where should we write the extractes update file to */
    fw_slot_base_addr = ota_slots_get_slot_address(fw_slot);

    /** erase all flash sectors of the FW slot **/
    DEBUG("[ota_file] INFO start erasing the FW slot\n");
    slot_sector = get_slot_page(fw_slot);
    int slot_last_page = flashsector_sector((void *)(fw_slot_base_addr + get_slot_size(fw_slot) - 1));
    for (int sector = slot_sector; sector <= slot_last_page; sector++) {
        flashsector_write(sector, NULL, 0);
    }

    /** decrypt update file and write to FW slot **/

    /* add the correct spacing in front of the file header first */
    DEBUG("[ota_file] INFO add necessary spacing before the header\n");
    uint32_t slot_write_addr = fw_slot_base_addr;
    uint32_t file_read_addr = file_address + OTA_FW_FILE_MAGIC_LEN; /* skip FILE_MAGIC */
#if (OTA_VTOR_ALIGN > OTA_FILE_HEADER_SPACE)
    /* add spacing in front of FILE_HEADER to have correct VTOR alignment */
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = 0xAA;
    }
    for (int i = 0; i < (OTA_VTOR_ALIGN - OTA_FILE_HEADER_SPACE) / sizeof(buffer); i++) {
        flashsector_write_only((void *)slot_write_addr, (void *)buffer, sizeof(buffer));
        slot_write_addr += sizeof(buffer);
    }
#endif

    /* copy FILE_HEADER */
    DEBUG("[ota_file] INFO start copying header information\n");
    flashsector_write_only((void *)slot_write_addr, (void *)file_read_addr, OTA_FILE_HEADER_SPACE);
    slot_write_addr += OTA_FILE_HEADER_SPACE;
    file_read_addr += OTA_FILE_HEADER_SPACE;

    /* copy binary */
    DEBUG("[ota_file] INFO start decrypting and writing binary data\n");
    uint32_t encrypted_size = fw_binary_size + (AES_BLOCK_SIZE - fw_binary_size % AES_BLOCK_SIZE);
    uint32_t binary_sections = encrypted_size / sizeof(buffer);
    uint32_t binary_sections_rest = encrypted_size % sizeof(buffer);
    for (int i = 0; i < binary_sections; i++) {
        /* decrypt a section of the binary to buffer */
        if (cipher_decrypt_cbc(&cipher_ctx, aes_iv, (uint8_t *)file_read_addr, sizeof(buffer), buffer) < 0) {
            printf("[ota_file] ERROR decryption off binary section failed!\n");
            return -1;
        }
        /* manually set iv to last ciphertext block, because of silly CBC interface */
        for (int i = 0; i < AES_BLOCK_SIZE; i++) {
            aes_iv[i] = ((uint8_t *)file_read_addr)[BUF_SIZE - AES_BLOCK_SIZE + i];
        }
        /* move read ptr forward */
        file_read_addr += sizeof(buffer);

        /* copy buffer to flash */
        flashsector_write_only((void *)slot_write_addr, (void *)buffer, sizeof(buffer));
        slot_write_addr += sizeof(buffer);
    }
    if (binary_sections_rest != 0) { /* binary_sections_rest will always be % AES_BLOCK_SIZE */
        /* decrypt a section of the binary to buffer */
        if (cipher_decrypt_cbc(&cipher_ctx, aes_iv, (uint8_t *)file_read_addr, binary_sections_rest, buffer) < 0) {
            printf("[ota_file] ERROR decryption off binary section failed!\n");
            return -1;
        }
        /* omit setting IV for next round, becaue it is not needed any more */
        /* move read ptr forward */
        file_read_addr += binary_sections_rest;

        /* copy buffer to flash */
        flashsector_write_only((void *)slot_write_addr, (void *)buffer, binary_sections_rest);
        slot_write_addr += binary_sections_rest;
    }

    printf("[ota_file] INFO successfully wrote update file to FW slot %d\n", fw_slot);

    return 0;
}
