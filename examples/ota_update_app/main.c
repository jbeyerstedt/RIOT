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
 * @brief       Example application for demonstrating the OTA Update module
 *
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"
#include "ota_updater.h"
#include "ota_slots.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int ota_request_cmd(int argc, char **argv) {
    return ota_updater_request_update();
}

int ota_download_cmd(int argc, char **argv) {
    return ota_updater_download();
}

int ota_install_cmd(int argc, char **argv) {
    return ota_updater_install();
}

int ota_reboot_cmd(int argc, char **argv) {
    ota_updater_reboot();
    return 0;
}

int view_slots_cmd(int argc, char **argv) {
    ota_slots_find_oldest_int_image();
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "ota_request", "Send request to update server", ota_request_cmd },
    { "ota_download", "Download the requested update", ota_download_cmd },
    { "ota_install", "Install the downloaded update", ota_install_cmd },
    { "ota_reboot", "Reboot device", ota_reboot_cmd },
    { "view_slots", "View FW Slots", view_slots_cmd },
    { NULL, NULL, NULL }
};

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
