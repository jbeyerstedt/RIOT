/*
 * Copyright (c) 2016, Mark Solters <msolters@gmail.com>
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
 * @defgroup    sys_ota_slots Firmware slots management
 * @ingroup     sys
 * @brief       Slots management for several FW in ROM
 * @{
 *
 * @file
 * @brief       FW Image R/W and Verification
 *
 * @author      Mark Solters <msolters@gmail.com>
 * @author      Francisco Acosta <francisco.acosta@inria.fr>
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 */

#ifndef OTA_SLOTS_H
#define OTA_SLOTS_H

#include "hashes/sha256.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief OTA_FW_METADATA_LENGTH:
 *         This is just the size of the OTA_metadata_t struct.
 */
#define OTA_FW_METADATA_LENGTH  sizeof(OTA_FW_metadata_t)

/**
 * @brief Structure to store firmware metadata, 4 byte aligned
 * @{
 */
typedef struct OTA_FW_metadata_t {
    uint32_t magic;                     /**< magic number to identify metadata struct */
    uint8_t hw_id[8];                  /**< id of the specific hardware */
    uint8_t chip_id[16];                /**< optional chip serial number */
    uint16_t fw_vers;                   /**< version number of the firmware */
    uint32_t fw_base_addr;              /**< flash address, where vector table starts */
    uint32_t size;                      /**< Size of firmware image */
} OTA_FW_metadata_t;
/** @} */

/**
 * @brief Structure of the firmware header
 * @{
 */
typedef struct OTA_FW_header {
    uint8_t fw_signature[48];
    uint8_t fw_sign_spacer[16];
    OTA_FW_metadata_t fw_metadata;
} OTA_FW_header_t;
/** @} */

/**
 * @brief  Print formatted FW image metadata to STDIO.
 *
 * @param[in] metadata         Metadata struct to fill with firmware metadata
 *
 */
void ota_slots_print_metadata(OTA_FW_metadata_t *metadata);

/**
 * @brief   Validate internal FW slot as a secure firmware
 *
 * @param[in] fw_slot          The FW slot to be validated.
 *
 * @return  0 on success or error code
 */
int ota_slots_validate_int_slot(uint8_t fw_slot);

/**
 * @brief   Get the internal metadata belonging to an FW slot in internal
 *          flash, using the flash page.
 *
 * @param[in] fw_slot_page       The FW slot page to be read for metadata.
 *
 * @param[in] *fw_metadata       Pointer to the FW_metadata_t struct where
 *                               the metadata is to be written.
 *
 * @return  0 on success or error code
 */
int ota_slots_get_int_metadata(uint8_t fw_slot_page,
                              OTA_FW_metadata_t *fw_metadata);

/**
 * @brief   Get the metadata belonging to an FW slot in external flash.
 *
 * @param[in] fw_slot            The FW slot to be read for metadata.
 *
 * @param[in] *fw_slot_metadata  Pointer to the FW_metadata_t struct where
 *                                the metadata is to be written.
 *
 * @return  0 on success or error code
 */
int ota_slots_get_slot_metadata(uint8_t fw_slot, OTA_FW_metadata_t *fw_slot_metadata);

/**
 * @brief   Get the metadata belonging to an FW slot in internal flash.
 *
 * @param[in] fw_slot             The FW slot to be read for metadata.
 *
 * @param[in] *fw_slot_metadata   Pointer to the FW_metadata_t struct where
 *                                 the metadata is to be written.
 *
 * @return  0 on success or error code
 */
int ota_slots_get_int_slot_metadata(uint8_t fw_slot,
                                   OTA_FW_metadata_t *fw_slot_metadata);

/**
 * @brief   Get the address corresponding to a given slot
 *
 * @param[in] fw_slot             The FW slot to get the address.
 *
 *
 * @return  0 on success or error code
 */
uint32_t ota_slots_get_slot_address(uint8_t fw_slot);

/**
 * @brief   Get the page corresponding to a given slot
 *
 * @param[in] fw_slot             The FW slot to get the page.
 *
 *
 * @return  0 on success or error code
 */
uint32_t ota_slots_get_slot_page(uint8_t fw_slot);

/**
 * @brief   Given an FW slot, verify the firmware content against the metadata.
 *
 * @param[in]  fw_slot    FW slot index to verify. (1-N)
 *
 * @return  0 for success or error code
 */
int ota_slots_verify_int_slot(uint8_t fw_slot);

/**
 * @brief   Returns true only if the metadata provided indicates the FW slot
 *          is populated and valid.
 *
 * @param[in] *metadata   FW metadata to be validated
 *
 * @return  0 if the FW slot is populated and valid. Otherwise, -1.
 */
int ota_slots_validate_metadata(OTA_FW_metadata_t *metadata);

/**
 * @brief   Find a FW slot containing firmware matching the supplied
 *          firmware version number. Will only find the first matching
 *          slot.
 *
 * @param[in] version     FW slot version.
 *
 * @return  The FW slot index of the matching FW slot. Return -1 in the event
 *          of no match.
 */
int ota_slots_find_matching_int_slot(uint16_t version);

/**
 * @brief   Find the first empty FW slot. Failing this, find the slot with the
 *          most out-of-date firmware version.
 *
 * @return  The FW slot index of the empty/oldest FW slot. This will never be
 *          0 because the Golden Image should never be erased.
 */
int ota_slots_find_empty_int_slot(void);

/**
 * @brief   Find the FW slot containing the most out-of-date firmware version.
 *          FW slots are in internal flash.
 *
 * @return  The FW slot index of the oldest firmware version.
 */
int ota_slots_find_oldest_int_image(void);

/**
 * @brief   Find the FW slot containing the most recent firmware version.
 *          FW slots are in internal flash.
 *
 * @return  The FW slot index of the newest firmware version.
 */
int ota_slots_find_newest_int_image(void);

/**
 * @brief   Clear an FW slot in internal flash.
 *
 * @param[in] fw_slot   The FW slot index of the firmware image to be erased.
 *
 * @return  -1 or error code
 */
int ota_slots_erase_int_image(uint8_t fw_slot);

/**
 * @brief   Overwrite firmware located in internal flash with the firmware
 *          stored in an external flash FW slot.
 *
 * @param[in] fw_slot   The FW slot index of the firmware image to be copied.
 *                       0 = "Golden Image" backup, aka factory restore
 *                       1, 2, 3, n = FW Download slots
 *
 * @return  -1 or error code
 */
int ota_slots_update_firmware(uint8_t fw_slot);

/**
 * @brief   Store firmware data in external flash at the specified
 *          address.
 *
 * @param[in] ext_address   External flash address to begin writing data.
 *
 * @param[in] data          Pointer to the data buffer to be written.
 *
 * @param[in] data_length   Length of the buffer
 *
 * @return  -1 or error code
 */
int ota_slots_store_fw_data(uint32_t ext_address, uint8_t *data, size_t data_length);

/**
 * @brief   Begin executing another firmware binary located in internal flash.
 *
 * @param[in] destination_address Internal flash address of the vector table
 *                                for the firmware binary that is to be booted
 *                                into. Since this FW lib prepends metadata
 *                                to each binary, the true VTOR start address
 *                                will be FW_METADATA_SPACE bytes past this
 *                                address.
 *
 */
void ota_slots_jump_to_image(uint32_t destination_address);

#ifdef __cplusplus
}
#endif

#endif /* OTA_SLOTS_H */
/** @} */
