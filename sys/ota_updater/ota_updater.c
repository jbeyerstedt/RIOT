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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef OTA_UPDATE
#define OTA_UPDATE      /* should be set by build script */
#endif

#include "ota_updater.h"
#include "net/gcoap.h"
#include "od.h"
#include "fmt.h"
#include "cpu_conf.h"
#include "periph/pm.h"

#include "ota_file.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

ota_updater_status_t update_status = NOT_CHECKED;

/* update server address and port */
char server_addr[] = "fe80::b0a6:bff:fedc:c725";
uint16_t server_port = 5683;

/* update server handling global variables */
/* TODO: something better to store the URI of the update to download */
char update_filename[40] = {};

static void _resp_handler(unsigned req_state, coap_pkt_t *pdu);
static size_t _send(uint8_t *buf, size_t len, char *addr_str);


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
    uint8_t fw_slot = (FW_SLOT == 1) ? 2 : 1;

    /* check if update file is suitable for selected slot */
    if (get_slot_address(fw_slot) != (file_header->fw_header.fw_metadata.fw_base_addr - OTA_VTOR_ALIGN)) {
        printf("[ota_updater] ERROR update file does not suit the free FW slot\n");
        return -1;
    }

    DEBUG("[ota_updater] INFO successfully found an empty slot (or simply the oldest available)\n");
    return ota_file_write_image(file_address, fw_slot);
}

int ota_updater_request_update(void)
{
    printf("[ota_updater] INFO requesting update\n");

    OTA_FW_metadata_t own_metadata;
    ota_slots_get_int_slot_metadata(FW_SLOT, &own_metadata);
    update_status = NOT_CHECKED;

    /** first check, if there was an interrupted update **/
    OTA_FW_metadata_t *file_metadata = &(((OTA_File_header_t *)(OTA_FILE_SLOT))->fw_header.fw_metadata);

    /* an update file is available and has a valid signature (complete download) */
    if (ota_updater_validate_file() == 0) {
        /* version number is higher, than own version, indicates an interupted update */
        if (file_metadata->fw_vers > own_metadata.fw_vers) {
            printf("[ota_updater] INFO interrupted update detected\n");
            update_status = INTERRUPTED_UPDATE;
            return OTA_CONTINUE_INSTALL;
        }
        /* otherwise the file is already installed */
    }

    /** send request to update server, if a new version is available **/
    uint8_t buf[GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;

    /* construct update request payload */
    uint8_t payload[11] = {};
    payload[0] = ((uint8_t *) &own_metadata.fw_vers)[1];    /* high byte */
    payload[1] = ((uint8_t *) &own_metadata.fw_vers)[0];    /* low byte */
    payload[2] = (FW_SLOT == 1) ? 2 : 1;
    /* copy hw_id. hard coded upper limit (8) ok, because of request format */
    for (int i = 0; i < 8; i++) {
        /* save the little endian value as big endian */
        payload[3 + i] = own_metadata.hw_id[7 - i];
    }

    /* send the payload */
    gcoap_req_init(&pdu, &buf[0], GCOAP_PDU_BUF_SIZE, 1, "/update");
    memcpy(pdu.payload, payload, sizeof(payload));
    len = gcoap_finish(&pdu, sizeof(payload), COAP_FORMAT_TEXT);

    DEBUG("gcoap_ota: sending msg ID %u, %u bytes\n", coap_get_id(&pdu),
          (unsigned) len);
    if (!_send(&buf[0], len, server_addr)) {
        printf("[ota_updater] ERROR CoAP message send failed \n");
        return -1;
    }

    update_status = PENDING_REQUEST;
    return 0;
}

ota_updater_status_t ota_updater_get_status(void)
{
    return update_status;
}

int ota_updater_download(void)
{
    printf("[ota_updater] INFO downloading update\n");

    /* if an interrupted update was detected, skip the download */
    if (UPDATE_AVAILABLE == update_status) {
        /* download the file from the resource identified by ota_updater_request_update() */
        /* TODO: really download the file and store to OTA_FILE_SLOT */
        return 0;           /* TODO: only a dummy, return real status later */
    }
    else if (INTERRUPTED_UPDATE == update_status) {
        DEBUG("[ota_updater] INFO skipping download because of interrupted update\n");
        return OTA_CONTINUE_INSTALL;
    }

    DEBUG("[ota_updater] INFO do not download before requesting an update at all!\n");
    return -1;  /* only reached, if someone has not followed the interface */
}

int ota_updater_install(void)
{
    printf("[ota_updater] INFO starting installation of update file\n");

    DEBUG("[ota_updater] INFO validating signature of update file\n");
    if (ota_updater_validate_file() != 0) {
        printf("[ota_updater] ERROR update file is invalid\n");
        return -1;
    }

    DEBUG("[ota_updater] INFO get an empty slot and install update\n");
    return ota_updater_flash_write();
}

void ota_updater_reboot(void)
{
    printf("[ota_updater] INFO rebooting ...\n");
    pm_reboot();
}


/*
 * CoAP Response callback.
 */
static void _resp_handler(unsigned req_state, coap_pkt_t *pdu)
{
    if (req_state == GCOAP_MEMO_TIMEOUT) {
        DEBUG("[ota_updater] timeout for msg ID %02u\n", coap_get_id(pdu));
        update_status = REQUEST_ERROR;
        return;
    }
    else if (req_state == GCOAP_MEMO_ERR) {
        printf("[ota_updater] ERROR in response\n");
        update_status = REQUEST_ERROR;
        return;
    }

#if (DEBUG == 1)
    char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
                      ? "Success" : "Error";
#endif
    DEBUG("[ota_updater] INFO response %s, code %1u.%02u", class_str,
          coap_get_code_class(pdu),
          coap_get_code_detail(pdu));
    if (pdu->payload_len) {
        if (coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE
            || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE) {
            /* Expecting diagnostic payload in failure cases */
            DEBUG(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
                  (char *)pdu->payload);
            update_status = REQUEST_ERROR;
            return;
        }
        else {
            /* answer from update server, update available */
#if (DEBUG == 1)
            DEBUG(", %u bytes\n", pdu->payload_len);
            od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
            DEBUG("\n")
#endif
            /* save dummy filename/ path from server answer */
            /* TODO: replace with real URI implementation */
            if (sizeof(pdu->payload) < sizeof(update_filename)) {
                memcpy(update_filename, pdu->payload, sizeof(pdu->payload));
            }

            printf("[ota_updater] INFO update available\n");
            update_status = UPDATE_AVAILABLE;
            return;
        }
    }
    else {
        /* answer from update server, no update available */
        DEBUG(", empty payload\n");

        printf("[ota_updater] INFO no update available\n");
        update_status = NO_UPDATE_AVAILABLE;
        return;
    }

    update_status = REQUEST_ERROR;
}

/*
 * CoAP send wrapper
 */
static size_t _send(uint8_t *buf, size_t len, char *addr_str)
{
    ipv6_addr_t addr;
    size_t bytes_sent;
    sock_udp_ep_t remote;

    remote.family = AF_INET6;
    remote.netif  = SOCK_ADDR_ANY_NETIF;

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("gcoap_ota: unable to parse destination address");
        return 0;
    }
    memcpy(&remote.addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    remote.port = server_port;
    bytes_sent = gcoap_req_send2(buf, len, &remote, _resp_handler);
    return bytes_sent;
}
