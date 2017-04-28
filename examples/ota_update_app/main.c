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
#include "thread.h"
#include "xtimer.h"
#include "ota_updater.h"
#include "ota_slots.h"

#include "cpu_conf.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

char wdg_thread_stack[THREAD_STACKSIZE_MAIN];

int ota_request_cmd(int argc, char **argv)
{
    int ret_val = ota_updater_request_update();

    printf("ota_updater_request_update returned %i\n", ret_val);
    return 0;
}

int ota_download_cmd(int argc, char **argv)
{
    int ret_val = ota_updater_download();

    printf("ota_updater_download returned %i\n", ret_val);
    return 0;
}

int ota_install_cmd(int argc, char **argv)
{
    int ret_val = ota_updater_install();

    printf("ota_updater_install returned %i\n", ret_val);
    return 0;
}

int ota_reboot_cmd(int argc, char **argv)
{
    ota_updater_reboot();
    return 0;
}

int view_slots_cmd(int argc, char **argv)
{
    ota_slots_print_available_slots();
    return 0;
}

int fw_info_cmd(int argc, char **argv)
{
    OTA_FW_metadata_t slot_metadata;

    ota_slots_get_int_slot_metadata(FW_SLOT, &slot_metadata);
    printf("FW version %d, slot %d\n", slot_metadata.fw_vers, FW_SLOT);
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "ota_request", "Send request to update server", ota_request_cmd },
    { "ota_download", "Download the requested update", ota_download_cmd },
    { "ota_install", "Install the downloaded update", ota_install_cmd },
    { "ota_reboot", "Reboot device", ota_reboot_cmd },
    { "view_slots", "View FW slots", view_slots_cmd },
    { "fw_info", "View FW version and slot number", fw_info_cmd },
    { NULL, NULL, NULL }
};

void *wdg_thread(void *arg)
{
    while (1) {
        /* reset the watchdog (IWGD) timer */
        IWDG->KR = 0xAAAA;

        xtimer_usleep(250000);      /* wdg timeout is 512 ms */
    }
    return NULL;
}

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    OTA_FW_metadata_t slot_metadata;
    ota_slots_get_int_slot_metadata(FW_SLOT, &slot_metadata);
    printf("RIOT OTA update module example, FW version %d\n", slot_metadata.fw_vers);

    /* add a thread to reset the watchdog periodically */
    thread_create(wdg_thread_stack, sizeof(wdg_thread_stack),
                  THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                  wdg_thread, NULL, "wdg_thread");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
