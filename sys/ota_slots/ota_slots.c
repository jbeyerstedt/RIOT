/*
 * Copyright (c) 2016, Mark Solters <msolters@gmail.com>.
 *               2016, Francisco Acosta <francisco.acosta@inria.fr>
 *               2017, Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
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
 * @file
 * @author      Mark Solters <msolters@gmail.com>
 * @author      Francisco Acosta <francisco.acosta@inria.fr>
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 */

#include <stdio.h>
#include <string.h>

#include <tweetnacl.h>
#include "ota_slots.h"
#include "cpu_conf.h"
#include "irq.h"
#if !defined(FLASH_SECTORS)        /* defined by Makefile.include of the CPU */
#include "periph/flashpage.h"
#else
#include "periph/flashsector.h"
#endif
#include "hashes/sha256.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static uint8_t firmware_buffer[1024];

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

/**
 * @brief      Get the internal metadata belonging to an FW slot in internal
 *             flash, using the flash page.
 *
 * @param[in]  fw_address           The FW slot address to be read for metadata.
 *
 * @param[out] *fw_metadata         Pointer to the FW_metadata_t struct where
 *                                  the metadata is to be written.
 *
 * @return     0 on success or error code
 */
int ota_slots_get_int_metadata(uint32_t fw_address, OTA_FW_metadata_t *fw_metadata)
{
    uint32_t fw_meta_address = fw_address + OTA_VTOR_ALIGN - OTA_FW_METADATA_SPACE;

    DEBUG("[ota_slots] Getting internal metadata at address %#lx\n", fw_meta_address);
    int_flash_read((uint8_t *)fw_metadata, fw_meta_address, sizeof(OTA_FW_metadata_t));

    return 0;
}

/**
 * @brief      Get the firmware signature (inner signature) of the given
 *             firmware slot.
 *
 * @param[in]  fw_slot              The FW slot to get the signature from.
 *
 * @return     flash address of the firmware signature
 */
uint32_t ota_slots_get_fw_signature_addr(uint8_t fw_slot)
{
    uint32_t slot_addr = ota_slots_get_slot_address(fw_slot);

    return slot_addr + OTA_VTOR_ALIGN - OTA_FW_HEADER_SPACE;
}

void ota_slots_print_metadata(OTA_FW_metadata_t *metadata)
{
    printf("\nFirmware metadata dump:\n");
    printf("Firmware HW ID: ");
    for (unsigned long i = 0; i < sizeof(metadata->hw_id); i++) {
        printf("%02x ", metadata->hw_id[i]);
    }
    printf("\n");
    printf("Chip ID: ");
    for (unsigned long i = 0; i < sizeof(metadata->chip_id); i++) {
        printf("%02x ", metadata->chip_id[i]);
    }
    printf("\n");
    printf("Firmware Version: %#x\n", metadata->fw_vers);
    printf("Firmware Base Address: %#lx\n", metadata->fw_base_addr);
    printf("Firmware Size: %ld Byte (0x%02lx)\n", metadata->size, metadata->size);
    printf("\n");
}

int ota_slots_validate_int_slot(uint8_t fw_slot)
{
    OTA_FW_metadata_t fw_metadata;
    uint32_t fw_image_address;
    uint32_t address;
    uint16_t rest;
    sha256_context_t sha256_ctx;
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint8_t sign_hash[SHA256_DIGEST_LENGTH + crypto_box_ZEROBYTES];
    unsigned char n[crypto_box_NONCEBYTES];
    int parts = 0;

    /* Determine the external flash address corresponding to the FW slot */
    if (fw_slot > MAX_FW_SLOTS || fw_slot == 0) {
        printf("[ota_slots] FW slot not valid, should be <= %d and > 0\n",
               MAX_FW_SLOTS);
        return -1;
    }

    /* Read the metadata of the corresponding FW slot */
    fw_image_address = ota_slots_get_slot_address(fw_slot);

    if (ota_slots_get_int_slot_metadata(fw_slot, &fw_metadata) == 0) {
        ota_slots_print_metadata(&fw_metadata);
    }
    else {
        printf("[ota_slots] ERROR cannot get slot metadata.\n");
    }

    printf("Verifying slot %d at 0x%lx \n", fw_slot, fw_image_address);

    /* check magic number first */
    if (OTA_FW_META_MAGIC != fw_metadata.magic) {
        return -1;
    }

    /* check, if the HW_ID of the image is suitable */
    uint64_t hw_id = HW_ID;
    for (int i = 0; i < sizeof(fw_metadata.hw_id); i++) {
        if ((uint8_t)(hw_id >> (i * 8)) != fw_metadata.hw_id[i]) {
            return -1;
        }
    }

    /*
     * check the signature
     */
    /* calculate hash of metadata section and firmware binary */
    address = fw_image_address + OTA_VTOR_ALIGN - OTA_FW_METADATA_SPACE;
    parts = (fw_metadata.size + OTA_FW_METADATA_SPACE) / sizeof(firmware_buffer);
    rest = (fw_metadata.size + OTA_FW_METADATA_SPACE) % sizeof(firmware_buffer);
    sha256_init(&sha256_ctx);
    while (parts) {
        int_flash_read(firmware_buffer, address, sizeof(firmware_buffer));
        sha256_update(&sha256_ctx, firmware_buffer, sizeof(firmware_buffer));
        address += sizeof(firmware_buffer);
        parts--;
    }
    int_flash_read(firmware_buffer, address, rest);
    sha256_update(&sha256_ctx, firmware_buffer, rest);
    sha256_final(&sha256_ctx, hash);

    /* open the crypto_box to extract the signed hash and compare hash values */
    uint8_t *fw_signature = (uint8_t *)ota_slots_get_fw_signature_addr(fw_slot);
    memset(sign_hash, 0, sizeof(sign_hash));
    memset(n, 0, sizeof(n));

    int res = crypto_box_open(sign_hash, fw_signature, OTA_FW_SIGN_LEN, n, server_pkey, firmware_skey);
    if (res) {
        printf("Decryption failed.\n");
        return -1;
    }
    else {
        printf("Decryption successful! verifying...\n");
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            if (hash[i] != (sign_hash[i + crypto_box_ZEROBYTES])) {
                printf("[ota_slots] ERROR incorrect decrypted hash!\n");
                return -1;
            }
        }
    }

    printf("[ota_slots] FW slot %i successfully validated!\n", fw_slot);

    return 0;
}

int ota_slots_get_int_slot_metadata(uint8_t fw_slot, OTA_FW_metadata_t *fw_slot_metadata)
{
    uint32_t slot_addr;

    DEBUG("[ota_slots] Getting internal FW slot %d metadata\n", fw_slot);
    if (fw_slot > MAX_FW_SLOTS || fw_slot == 0) {
        printf("[ota_slots] FW slot not valid, should be <= %d and > 0\n",
               MAX_FW_SLOTS);
        return -1;
    }

    slot_addr = ota_slots_get_slot_address(fw_slot);

    return ota_slots_get_int_metadata(slot_addr, fw_slot_metadata);
}

uint32_t ota_slots_get_slot_address(uint8_t fw_slot)
{
    return get_slot_address(fw_slot);
}

uint32_t ota_slots_get_slot_page(uint8_t fw_slot)
{
    return get_slot_page(fw_slot);
}

int ota_slots_validate_metadata(OTA_FW_metadata_t *metadata)
{
    /* Is the FW slot erased?
     * First, we check to see if every byte in the metadata is 0xFF.
     * If this is the case, this metadata is "erased" and therefore we assume
     * the FW slot to be empty.
     */
    int erased = 1;
    uint8_t *metadata_ptr = (uint8_t *)metadata;
    int b = OTA_FW_METADATA_LENGTH;

    while (b--) {
        if (*metadata_ptr++ != 0xff) {
            /* We encountered a non-erased byte.
             * There's some non-trivial data here.
             */
            erased = 0;
            break;
        }
    }

    /* If the FW slot is erased, it's not valid!  No more work to do here. */
    if (erased) {
        return -1;
    }

    /* The slot is not erased, check magic number */
    if (OTA_FW_META_MAGIC != metadata->magic) {
        return -1;
    }

    /* If we get this far, the metadata block seems to be valid / is not erased */
    return 0;
}

int ota_slots_find_matching_int_slot(uint16_t version)
{
    int matching_slot = -1; /* Assume there is no matching FW slot. */

    /* Iterate through each of the FW slots. */
    for (int slot = 1; slot <= MAX_FW_SLOTS; slot++) {

        /* Get the metadata of the current FW slot. */
        OTA_FW_metadata_t fw_slot_metadata;
        if (ota_slots_get_int_slot_metadata(slot, &fw_slot_metadata) == 0) {
            ota_slots_print_metadata(&fw_slot_metadata);
        }
        else {
            printf("[ota_slots] ERROR cannot get slot metadata.\n");
        }

        /* Is this slot empty? If yes, skip. */
        if (ota_slots_validate_metadata(&fw_slot_metadata) == false) {
            continue;
        }

        /* Does this slot's FW version match our search parameter? */
        if (fw_slot_metadata.fw_vers == version) {
            matching_slot = slot;
            break;
        }
    }

    if (matching_slot == -1) {
        printf("[ota_slots] No FW slot matches Firmware v%i\n", version);
    }
    else {
        printf("[ota_slots] FW slot #%i matches Firmware v%i\n", matching_slot,
               version);
    }

    return matching_slot;
}

int ota_slots_find_empty_int_slot(void)
{
    /* Iterate through each of the MAX_FW_SLOTS internal slots. */
    for (int slot = 1; slot <= MAX_FW_SLOTS; slot++) {

        /* Get the metadata of the current FW slot. */
        OTA_FW_metadata_t fw_slot_metadata;

        if (ota_slots_get_int_slot_metadata(slot, &fw_slot_metadata) == 0) {
            ota_slots_print_metadata(&fw_slot_metadata);
        }
        else {
            printf("[ota_slots] ERROR cannot get slot metadata.\n");
        }

        /* Is this slot invalid? If yes, let's treat it as empty. */
        if (ota_slots_validate_metadata(&fw_slot_metadata) == false) {
            return slot;
        }
    }

    printf("[ota_slots] Could not find any empty FW slots!"
           "\nSearching for oldest FW slot...\n");
    /*
     * If execution goes this far, no empty slot was found. Now, we look for
     * the oldest FW slot instead.
     */
    return ota_slots_find_oldest_int_image();
}

int ota_slots_find_oldest_int_image(void)
{
    /* The oldest firmware should be the v0 */
    int oldest_fw_slot = 1;
    uint16_t oldest_firmware_version = 0;

    /* Iterate through each of the MAX_FW_SLOTS internal slots. */
    for (int slot = 1; slot <= MAX_FW_SLOTS; slot++) {
        /* Get the metadata of the current FW slot. */
        OTA_FW_metadata_t fw_slot_metadata;

        if (ota_slots_get_int_slot_metadata(slot, &fw_slot_metadata) == 0) {
            ota_slots_print_metadata(&fw_slot_metadata);
        }
        else {
            printf("[ota_slots] ERROR cannot get slot metadata.\n");
        }

        /* Is this slot populated? If not, skip. */
        if (ota_slots_validate_metadata(&fw_slot_metadata) == false) {
            continue;
        }

        /* Is this the oldest image we've found thus far? */
        if (oldest_firmware_version) {
            if (fw_slot_metadata.fw_vers < oldest_firmware_version) {
                oldest_fw_slot = slot;
                oldest_firmware_version = fw_slot_metadata.fw_vers;
            }
        }
        else {
            oldest_fw_slot = slot;
            oldest_firmware_version = fw_slot_metadata.fw_vers;
        }
    }

    printf("[ota_slots] Oldest FW slot: #%d; Firmware v%d\n", oldest_fw_slot,
           oldest_firmware_version);

    return oldest_fw_slot;
}

int ota_slots_find_newest_int_image(void)
{
    /* At first, we only assume knowledge of version v0 */
    int newest_fw_slot = 0;
    uint16_t newest_firmware_version = 0;

    /* Iterate through each of the MAX_FW_SLOTS. */
    for (int slot = 1; slot <= MAX_FW_SLOTS; slot++) {
        /* Get the metadata of the current FW slot. */
        OTA_FW_metadata_t fw_slot_metadata;

        if (ota_slots_get_int_slot_metadata(slot, &fw_slot_metadata) == 0) {
            ota_slots_print_metadata(&fw_slot_metadata);
        }
        else {
            printf("[ota_slots] ERROR cannot get slot metadata.\n");
        }

        /* Is this slot populated? If not, skip. */
        if (ota_slots_validate_metadata(&fw_slot_metadata) == -1) {
            continue;
        }

        /* Is this the newest image we've found thus far? */
        if (fw_slot_metadata.fw_vers > newest_firmware_version) {
            newest_fw_slot = slot;
            newest_firmware_version = fw_slot_metadata.fw_vers;
        }
    }

    printf("Newest FW slot: #%d; Firmware v%d\n", newest_fw_slot,
           newest_firmware_version);

    return newest_fw_slot;
}

// TODO_JB: test
int ota_slots_erase_int_image(uint8_t fw_slot)
{
    /* Get page address of the fw_slot in internal flash */
    uint32_t fw_image_base_address;
    /* Get the page where the fw_slot is located */
    uint8_t slot_page;

    if (fw_slot > MAX_FW_SLOTS || fw_slot == 0) {
        printf("[ota_slots] FW slot not valid, should be <= %d and > 0\n",
               MAX_FW_SLOTS);
        return -1;
    }

    fw_image_base_address = ota_slots_get_slot_address(fw_slot);

#if !defined(FLASH_SECTORS)
    printf("[ota_slots] Erasing FW slot %u [%#lx, %#lx]...\n", fw_slot,
           fw_image_base_address,
           fw_image_base_address + (FW_SLOT_PAGES * FLASHPAGE_SIZE) - 1);
#else
    printf("[ota_slots] Erasing FW slot %u [%#lx, %#lx]...\n", fw_slot,
           fw_image_base_address,
           fw_image_base_address + get_slot_size(fw_slot) - 1);
#endif

    slot_page = ota_slots_get_slot_page(fw_slot);

    /* Erase each page in the FW internal slot! */
#if !defined(FLASH_SECTORS)
    for (int page = slot_page; page < slot_page + FW_SLOT_PAGES; page++) {
        DEBUG("[ota_slots] Erasing page %d\n", page);
        flashpage_write(page, NULL);
    }
#else
    int slot_last_page = flashsector_sector((void *)(fw_image_base_address + get_slot_size(fw_slot) - 1));
    for (int sector = slot_page; sector < slot_last_page; sector++) {
        DEBUG("[ota_slots] Erasing sector %d\n", page);
        flashsector_write(sector, NULL, 0);
    }
#endif


    printf("[ota_slots] Erase successful\n");

    return 0;
}

/*
 * _estack pointer needed to reset PSP position
 */
extern uint32_t _estack;

void ota_slots_jump_to_image(uint32_t destination_address)
{
    if (destination_address) {
        /*
         * Only add the metadata length offset if destination_address is NOT 0!
         * (Jumping to 0x0 is used to reboot the device)
         */
        destination_address += OTA_VTOR_ALIGN;
    }

    /* Disable IRQ */
    (void)irq_disable();

    /* Move PSP to the end of the stack */
    __set_PSP((uint32_t)&_estack);

    /* Move to the second pointer on VTOR (reset_handler_default) */
    destination_address += VTOR_RESET_HANDLER;

    /* Load the destination address */
    __asm("LDR R0, [%[dest]]"::[dest]"r"(destination_address));
    /* Make sure the Thumb State bit is set. */
    __asm("ORR R0, #1");
    /* Branch execution */
    __asm("BX R0");
}
