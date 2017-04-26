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

#include "ota_file.h"
#include "ota_slots.h"

#include "periph/flashsector.h"

#ifndef OTA_UPDATE
#define OTA_UPDATE                      /* should better be set by a CFLAG */
#endif
#include "cpu_conf.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

void safe_state(void)
{
    printf("No bootable image found, go to safe state");
    while (1) {
        ;
    }
}

void boot_firmware_slot(uint8_t slot)
{
    uint32_t address;

    printf("Booting image on slot %d ...\n", slot);
    address = ota_slots_get_slot_address(slot);
    ota_slots_jump_to_image(address);
}

int main(void)
{
    uint8_t boot_slot = 0;
    uint8_t watchdog_dummy = 0;

    (void) puts("Welcome to RIOT bootloader!\n");

    ota_slots_print_available_slots();

    /** first assume, that the newest slot is good **/
    boot_slot = ota_slots_find_newest_int_image();
    if (0 == boot_slot) {
        (void) puts("No bootable slot found!\n");
        safe_state();
    }

    /** check, if both slots are populated **/
    if (boot_slot == ota_slots_find_oldest_int_image()) {   /* just one slot */
        printf("Only one slot is populated, getting watchdog status\n");
        /* TODO_JB: check real watchdog */
        if (iwdg_reset_status() == 0) {         /* watchdog ok, normal behaviour */
            printf("Watchdog ok\n");
            /* check signature before booting */
            printf("validating signature of slot %i\n", boot_slot);
            if (ota_slots_validate_int_slot(boot_slot) == 0) {
                boot_firmware_slot(boot_slot);
            }
            else {
                printf("Slot %u inconsistent!\n", boot_slot);
                safe_state();
            }
        }
        else {                                  /* watchdog was triggered, secondary behaviour */
            printf("Watchdog was triggered\n");
            safe_state();
        }
    }
    else {                                                  /* both slots are populetd */
        printf("Both slots are populated, getting watchdog status\n");
        /* TODO_JB: check real watchdog */
        if (iwdg_reset_status() == 0) {         /* watchdog ok, try normal behaviour */
            printf("Watchdog ok\n");
            /* check signature of newest slot */
            printf("validating signature of slot %i\n", boot_slot);
            if (ota_slots_validate_int_slot(boot_slot) == 0) {
                boot_firmware_slot(boot_slot);
            }
            else {
                printf("Slot %u inconsistent!\n", boot_slot);

                /* compare fw versions of the update file and the newest slot */
                printf("trying to check, if the cause was an incomplete installation\n");
                OTA_File_header_t *file_header = (OTA_File_header_t *)(OTA_FILE_SLOT);
                /* first check if a file is available */
                if ((file_header->magic_h == (uint32_t)OTA_FW_FILE_MAGIC_H) && (file_header->magic_l == (uint32_t)OTA_FW_FILE_MAGIC_L)) {
                    OTA_FW_metadata_t slot_metadata;
                    ota_slots_get_int_slot_metadata(boot_slot, &slot_metadata);

                    uint16_t file_fw_vers = file_header->fw_header.fw_metadata.fw_vers;
                    uint16_t slot_fw_vers = slot_metadata.fw_vers;
                    if (file_fw_vers <= slot_fw_vers) {
                        /* delete newest FW slot */
                        printf("erasing newest FW slot\n");
                        flashsector_write(get_slot_page(boot_slot), NULL, 0);
                    }
                }

                /* continue after this if-else-statement */
            }
        }
        else {                                  /* watchdog was triggered, secondary behaviour */
            printf("Watchdog was triggered\n");

            /* compare fw versions of the update file and the newest slot */
            printf("trying to check, if the cause was an incomplete installation\n");
            OTA_File_header_t *file_header = (OTA_File_header_t *)(OTA_FILE_SLOT);
            /* first check if a file is available */
            if ((file_header->magic_h == (uint32_t)OTA_FW_FILE_MAGIC_H) && (file_header->magic_l == (uint32_t)OTA_FW_FILE_MAGIC_L)) {
                OTA_FW_metadata_t slot_metadata;
                ota_slots_get_int_slot_metadata(boot_slot, &slot_metadata);

                uint16_t file_fw_vers = file_header->fw_header.fw_metadata.fw_vers;
                uint16_t slot_fw_vers = slot_metadata.fw_vers;
                if (file_fw_vers <= slot_fw_vers) {
                    /* delete the malicious update file */
                    printf("erasing update file with non executable code\n");
                    flashsector_write(OTA_FILE_SLOT_START_SECTOR, NULL, 0);
                }
            }

            /* continue after this if-else-statement */
        }

        /* set boot_slot to the old slot */
        boot_slot = ota_slots_find_oldest_int_image();

        /* check signature before booting */
        if (ota_slots_validate_int_slot(boot_slot) == 0) {
            boot_firmware_slot(boot_slot);
        }
        else {
            printf("Slot %u inconsistent!\n", boot_slot);
            safe_state();
        }
    }

    /* Should not happen */
    return 0;
}
