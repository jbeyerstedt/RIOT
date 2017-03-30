/*
 * Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_ota_updater OTA Firmware Update Interface
 * @ingroup     sys
 * @brief       OTA Updater for downloading and installing updates
 * @{
 *
 * @file
 * @brief       OTA Updater for downloading and installing updates
 *
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 */

#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include "ota_file.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Send request to update server, if an update is available
 *
 * @return     1 if new firmware available, 0 if not or error code
 */
int ota_updater_request_update(void);

/**
 * @brief      Download the requested update file
 *
 * @return     0 on success or error code
 */
int ota_updater_download(void);

/**
 * @brief      Decrypt and install the downloaded update file
 *
 * @return     0 on success or error code
 */
int ota_updater_install(void);

/**
 * @brief      Initiate reboot to boot the new firmware
 */
void ota_updater_reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_FILE_H */
/** @} */
