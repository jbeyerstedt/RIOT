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
 * @ingroup     FW
 * @file
 * @brief       Metadata generation for OTA FW images
 *
 * @author      Mark Solters <msolters@gmail.com>
 * @author      Francisco Acosta <francisco.acosta@inria.fr>
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/resource.h>

#include "ota_slots.h"
#include "hashes/sha256.h"

#ifndef OTA_FW_METADATA_SPACE
#define OTA_FW_METADATA_SPACE       (0x040)     /* default value only */
#endif

/* Input firmware .bin file */
FILE *firmware_bin;

/* Output metadata .bin file */
FILE *metadata_bin;

uint32_t firmware_size = 0;

int main(int argc, char *argv[])
{
    OTA_FW_metadata_t metadata;
    uint8_t output_buffer[sizeof(OTA_FW_metadata_t)];

    (void)argc;

    if (!argv[1]) {
        printf("Please provide a .bin file as the first argument.\n");
        return -1;
    }

    if (!argv[2]) {
        printf("Please provide a 16-bit hex firmware version integer as the second argument.\n");
        return -1;
    }

    if (!argv[3]) {
        printf("Please provide a 32-bit hex hardware id integer as the third argument.\n");
        return -1;
    }

    if (!argv[4]) {
        printf("Please provide a 32-bit hex firmware base address as the fourth argument.\n");
        return -1;
    }

    /* Get the size of the firmware .bin file */
    firmware_bin = fopen(argv[1], "r");
    fseek(firmware_bin, 0L, SEEK_END);
    firmware_size = ftell(firmware_bin);
    fclose(firmware_bin);

    /* Generate FW image metadata */
    metadata.magic = 0x544f4952;        /* RIOT as hex */
    sscanf(argv[3], "%lx", (uint64_t *)&(metadata.hw_id));
    for (int i = 0; i < 16; i++) {      /* set chip id to 0 */
        metadata.chip_id[i] = 0x00U;
    }
    sscanf(argv[2], "%hx", &(metadata.fw_vers));
    sscanf(argv[4], "%x", &(metadata.fw_base_addr));
    metadata.size = firmware_size;

    printf("Firmware HW ID: ");
    for (unsigned long i = 0; i < sizeof(metadata.hw_id); i++) {
        printf("%02x ", metadata.hw_id[i]);
    }
    printf("\n");
    printf("Chip ID: ");
    for (unsigned long i = 0; i < sizeof(metadata.chip_id); i++) {
        printf("%02x ", metadata.chip_id[i]);
    }
    printf("\n");
    printf("Firmware Version: %#x\n", metadata.fw_vers);
    printf("Firmware Base Address: %#x\n", metadata.fw_base_addr);
    printf("Firmware Size: %d Byte (0x%02x)\n", metadata.size, metadata.size);


    memcpy(output_buffer, (uint8_t*)&metadata, sizeof(OTA_FW_metadata_t));

    /* Open the output firmware .bin file */
    metadata_bin = fopen("ota_fw_metadata.bin", "w");

    /* Write the metadata */
    printf("Metadata size: 0x%02lx\n", sizeof(OTA_FW_metadata_t));
    fwrite(output_buffer, sizeof(output_buffer), 1, metadata_bin);

    /* 0xff spacing until firmware binary starts */
    uint8_t blank_buffer[OTA_FW_METADATA_SPACE - sizeof(OTA_FW_metadata_t)];
    for (unsigned long b = 0; b < sizeof(blank_buffer); b++) {
        blank_buffer[b] = 0xff;
    }
    fwrite(blank_buffer, sizeof(blank_buffer), 1, metadata_bin);

    /* Close the metadata file */
    fclose(metadata_bin);

    return 0;
}
