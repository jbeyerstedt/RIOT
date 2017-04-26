/*
 * Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Default bootloader to manage FW slots for OTA updates
 *
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xtimer.h"
#include "ota_file.h"
#include "ota_slots.h"

#include "periph/flashsector.h"

#ifndef OTA_UPDATE
#define OTA_UPDATE                      /* should better be set by a CFLAG */
#endif
#include "cpu_conf.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static inline void iwdg_reload(void)
{
    IWDG->KR = 0xAAAA;
}

void iwdg_start_and_configure(void)
{
    /* configure watchdog */
    IWDG->KR = 0x5555;
    IWDG->PR = 0x02;    /* 2048 ms timeout */

    /* activate */
    IWDG->KR = 0xCCCC;
}

void safe_state(void)
{
    printf("no bootable image found, safe state");
    while (1) {
        iwdg_reload();
        xtimer_usleep(250000);
    }
}

void boot_firmware_slot(uint8_t slot)
{
    uint32_t address;

    printf("booting FW slot %d ...\n", slot);
    address = ota_slots_get_slot_address(slot);
    ota_slots_jump_to_image(address);
}

int main(void)
{
    uint8_t boot_slot = 0;

    /** get reset reason (IWDG) and clear flag */
    uint32_t reset_flag_iwdg = RCC->CSR & RCC_CSR_WDGRSTF;
    RCC->CSR |= RCC_CSR_RMVF;

    /*** activate the watchdog ***/
    iwdg_start_and_configure();

    /*** start of bootloader behaviour ***/
    (void) puts("welcome to OTA bootloader\n");

    ota_slots_print_available_slots();

    /** first assume, that the newest slot is good **/
    boot_slot = ota_slots_find_newest_int_image();
    if (0 == boot_slot) {
        safe_state();
    }

    /** check, if both slots are populated **/
    if (boot_slot == ota_slots_find_oldest_int_image()) {   /* just one slot */
        printf("one slot populated, getting watchdog reset flag\n");
        if (0 == reset_flag_iwdg) {             /* watchdog ok, normal behaviour */
            /* check signature before booting */
            printf("reset flag normal, validating signature of slot %i\n", boot_slot);
            if (ota_slots_validate_int_slot(boot_slot) == 0) {
                boot_firmware_slot(boot_slot);
            }
            else {
                printf("slot %u inconsistent\n", boot_slot);
                safe_state();
            }
        }
        else {                                  /* watchdog not ok, secondary behaviour */
            printf("reset reason was watchdog\n");
            safe_state();
        }
    }
    else {                                                  /* both slots are populetd */
        printf("both slots populated, getting watchdog reset flag\n");
        if (0 == reset_flag_iwdg) {             /* watchdog ok, try normal behaviour */
            /* check signature of newest slot */
            printf("reset flag normal, validating signature of slot %i\n", boot_slot);
            if (ota_slots_validate_int_slot(boot_slot) == 0) {
                boot_firmware_slot(boot_slot);
            }
            else {
                printf("slot %u inconsistent\n", boot_slot);

                /* compare fw versions of the update file and the newest slot */
                DEBUG("checking, if reason was an incomplete installation\n");
                OTA_File_header_t *file_header = (OTA_File_header_t *)(OTA_FILE_SLOT);
                /* first check if a file is available */
                if ((file_header->magic_h == (uint32_t)OTA_FW_FILE_MAGIC_H) && (file_header->magic_l == (uint32_t)OTA_FW_FILE_MAGIC_L)) {
                    OTA_FW_metadata_t slot_metadata;
                    ota_slots_get_int_slot_metadata(boot_slot, &slot_metadata);

                    uint16_t file_fw_vers = file_header->fw_header.fw_metadata.fw_vers;
                    uint16_t slot_fw_vers = slot_metadata.fw_vers;
                    if (file_fw_vers <= slot_fw_vers) {
                        /* delete newest FW slot */
                        printf("> incomplete installation detected, erasing slot\n");
                        flashsector_write(get_slot_page(boot_slot), NULL, 0);
                    }
                    else {
                        DEBUG("> the cause must be a bitflip somewhere\n");
                    }
                }

                /* continue after this if-else-statement */
            }
        }
        else {                                  /* watchdog not ok, secondary behaviour */
            printf("reset reason was watchdog, installed FW is not executable\n");

            /* compare fw versions of the update file and the newest slot */
            DEBUG("checking, if reason was a freshly installed FW\n");
            OTA_File_header_t *file_header = (OTA_File_header_t *)(OTA_FILE_SLOT);
            /* first check if a file is available */
            if ((file_header->magic_h == (uint32_t)OTA_FW_FILE_MAGIC_H) && (file_header->magic_l == (uint32_t)OTA_FW_FILE_MAGIC_L)) {
                OTA_FW_metadata_t slot_metadata;
                ota_slots_get_int_slot_metadata(boot_slot, &slot_metadata);

                uint16_t file_fw_vers = file_header->fw_header.fw_metadata.fw_vers;
                uint16_t slot_fw_vers = slot_metadata.fw_vers;
                if (file_fw_vers <= slot_fw_vers) {
                    /* delete the malicious update file */
                    printf("> update file matches installed FW, erasing update file\n");
                    flashsector_write(OTA_FILE_SLOT_START_SECTOR, NULL, 0);
                }
                else {
                    DEBUG("> reason unknown\n");
                }
            }

            /* continue after this if-else-statement */
        }

        /* set boot_slot to the old slot */
        DEBUG("newest FW slot is not executable, trying old FW\n");
        boot_slot = ota_slots_find_oldest_int_image();

        /* check signature before booting */
        printf("validating signature of slot %i\n", boot_slot);
        if (ota_slots_validate_int_slot(boot_slot) == 0) {
            boot_firmware_slot(boot_slot);
        }
        else {
            printf("slot %u inconsistent\n", boot_slot);
            safe_state();
        }
    }

    /* Should not happen */
    return 0;
}
