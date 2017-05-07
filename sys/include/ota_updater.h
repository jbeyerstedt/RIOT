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

/**
 *  @brief OTA_CONTINUE_INSTALL:
 *         return value, if an interrupted update was detected
 */
#define OTA_CONTINUE_INSTALL    (2)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief update_status_t:
 *        possible values of the update status. The status of an update request
 *        must be polled via the ota_updater_get_status() function.
 */
typedef enum update_status {
    NOT_CHECKED,
    INTERRUPTED_UPDATE,
    PENDING_REQUEST,
    UPDATE_AVAILABLE,
    NO_UPDATE_AVAILABLE,
    REQUEST_ERROR
} ota_updater_status_t;

/**
 * @brief      Send request to update server, to check if an update is available
 *             Because of the asyncronous CoAP interface, this function will
 *             not return, if an update is available
 *
 * @return     0 on success, -1 on error or
 *             OTA_CONTINUE_INSTALL if an interrupted update is detected
 */
int ota_updater_request_update(void);

/**
 * @brief      Get the content of the update status variable of type
 *             ota_updater_status_t
 *
 * @return     status of the update request
 */
ota_updater_status_t ota_updater_get_status(void);

/**
 * @brief      Download the requested update file.
 *             Call ota_updater_request_update() first!
 *
 * @return     0 on success, -1 on error or
 *             OTA_CONTINUE_INSTALL if an interrupted update is detected
 */
int ota_updater_download(void);

/**
 * @brief      Decrypt and install the downloaded update file
 *
 * @return     0 on success or -1 on error
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
