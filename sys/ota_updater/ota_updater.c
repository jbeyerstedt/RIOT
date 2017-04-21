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

#ifndef OTA_UPDATE
#define OTA_UPDATE      /* should be set by build script */
#endif

#include "ota_updater.h"
#include "cpu_conf.h"
#include "periph/pm.h"

#include "ota_file.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

uint8_t is_update_availabe = 0;
// TODO_JB_2: something to store the server address
// TODO_JB_2: something to store the URI of the update to download

/**
 * @brief      Validate the OTA Update File
 *
 * @return     0 on success, 1 for an invalid signature or -1 on error
 */
int ota_updater_validate_file(void)
{
    return ota_file_validate_file((uint32_t)OTA_FILE_SLOT);
}

/**
 * @brief      Decrypt the update file and write to a flash slot
 *
 * @return     0 on success or -1 on error
 */
int ota_updater_flash_write(void)
{
    OTA_File_header_t *file_header = (OTA_File_header_t *)(OTA_FILE_SLOT);
    uint32_t file_address = OTA_FILE_SLOT;
    uint8_t fw_slot = ota_slots_find_empty_int_slot();

    if (0 == fw_slot) {
        printf("[ota_updater] ERROR getting oldest FW slot\n");
        return -1;
    }

    /* check if update file is suitable for selected slot */
    if (get_slot_address(fw_slot) != (file_header->fw_header.fw_metadata.fw_base_addr - OTA_VTOR_ALIGN)) {
        printf("[ota_updater] ERROR update file does not suit the free FW slot\n");
        return -1;
    }

    printf("[ota_updater] INFO successfully found an empty slot (or simply the oldest available)\n");
    return ota_file_write_image(file_address, fw_slot);
}

// TODO_JB_2: test
int ota_updater_request_update(void)
{
    printf("[ota_updater] INFO requesting update\n");

    // TODO_JB_2: implement some networking protocol to communicate with the server
    // TODO_JB_2: return 1 if new firmware available, 0 if not or error code

    return 1;                           // TODO_JB: only used as dummy
}

// TODO_JB_2: test
int ota_updater_download(void)
{
    printf("[ota_updater] INFO downloading update\n");

    return 0;
}

int ota_updater_install(void)
{
    printf("[ota_updater] INFO starting installation of update file\n");

    printf("[ota_updater] INFO validating signature of update file\n");
    if (ota_updater_validate_file() != 0) {
        return -1;
    }

    printf("[ota_updater] INFO get an empty slot and install update\n");
    return ota_updater_flash_write();
}

void ota_updater_reboot(void)
{
    printf("[ota_updater] INFO rebooting ...\n");
    pm_reboot();
}
