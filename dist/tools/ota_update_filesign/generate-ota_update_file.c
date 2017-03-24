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
 * @brief       Generator for OTA firmware update files
 *
 * @author      Mark Solters <msolters@gmail.com>
 * @author      Francisco Acosta <francisco.acosta@inria.fr>
 * @author      Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/resource.h>

#include "ota_file.h"
#include "tweetnacl/tweetnacl.h"
#include "hashes/sha256.h"

#ifndef OTA_FW_METADATA_SPACE
#define OTA_FW_METADATA_SPACE       (0x040)     /* default value only */
#endif

#ifndef OTA_FW_SIGN_SPACE
#define OTA_FW_SIGN_SPACE           (0x040)     /* default value only */
#endif

#ifndef OTA_FILE_SIGN_SPACE
#define OTA_FILE_SIGN_SPACE         (0x080)     /* default value only */
#endif

/* Input unsigned slot binary .bin file */
FILE *slot_bin;

/* Output signed firmware update .bin file */
FILE *updatefile_bin;

/* temp file for encrypted data */
FILE *tmp_file;

/* Pointer to firmware's public key */
FILE *firmware_pk;

/* Pointer to server's secret key */
FILE *server_sk;

/* Keys buffers */
static unsigned char firmware_pkey[crypto_box_PUBLICKEYBYTES];
static unsigned char server_skey[crypto_box_SECRETKEYBYTES];

/* Crypto helpers */
static const unsigned char n[crypto_box_NONCEBYTES];
static unsigned char m[OTA_FILE_SIGN_LEN];

static const unsigned char nonce[crypto_stream_NONCEBYTES];

int main(int argc, char *argv[])
{
    OTA_FW_metadata_t metadata;
    sha256_context_t firmware_sha256;
    uint8_t firmware_buffer[1024];
    int bytes_read = 0;
    uint8_t sha256_hash[SHA256_DIGEST_LENGTH];

    uint8_t fw_signature[OTA_FW_SIGN_LEN];
#ifndef IMAGE_ONLY
    uint8_t file_signature[OTA_FILE_SIGN_LEN];
#endif

    (void)argc;

    if (!argv[1]) {
        printf("Please provide a .bin file provided by ota_update_filemeta as the first argument.\n");
        return -1;
    }

    if (!argv[2]) {
        printf("Please provide the path to the server's secret key as the second argument.\n");
        return -1;
    }

    if (!argv[3]) {
        printf("Please provide the path to the device's public key as the third argument.\n");
        return -1;
    }

    if (argv[4]) {
        printf("THIS IMAGE WILL ONLY WORK ON ONE DEVICE!\n");
    }

    /*
     * read the keys
     */
    printf("Reading firmware public key...\n");
    firmware_pk = fopen(argv[3], "r");
    if (firmware_pk != NULL) {
        size_t size = fread(firmware_pkey, 1, sizeof(firmware_pkey), firmware_pk);
        printf("Read %zu bytes from firmware_pkey.pub\n", size);
        fclose(firmware_pk);
    } else {
        printf("ERROR! Cannot open firmware public key\n");
        return -1;
    }

    printf("Reading server secret key \"server_skey\"...\n");
    server_sk = fopen(argv[2], "r");
    if (server_sk != NULL) {
        size_t size = fread(server_skey, 1, sizeof(server_skey), server_sk);
        printf("Read %zu bytes from server_skey\n", size);
        fclose(server_sk);
    } else {
        printf("ERROR! Cannot open server secret key\n");
        return -1;
    }


    /* open the firmware image .bin file */
    slot_bin = fopen(argv[1], "r");

    /*
     *  if neccesary, set chip id
     */
    /* read metadata section from file */
    bytes_read = fread(firmware_buffer, 1, OTA_FW_METADATA_SPACE, slot_bin);
    if (bytes_read < OTA_FW_METADATA_SPACE) {
        printf("ERROR: something went wrong reading the metadata section\n");
        return -1;
    }
    memcpy(&metadata, &firmware_buffer, sizeof(OTA_FW_metadata_t));

    if (argv[4]) {
        /*
         * TODO_JB: read chip id from argument, write to metadata.chip_id
         */
        ;
    }

    /*
     *  generate firmware signature (inner signature)
     */

    /* read the image file and calculate hash */
    sha256_init(&firmware_sha256);
    while((bytes_read = fread(firmware_buffer, 1, sizeof(firmware_buffer), slot_bin))) {
        sha256_update(&firmware_sha256, firmware_buffer, bytes_read);
    }
    sha256_final(&firmware_sha256, sha256_hash);

    /* encrypt hash using crypto_box of tweetNaCl */
    memset(m, 0, crypto_box_ZEROBYTES);
    memcpy(m + crypto_box_ZEROBYTES, sha256_hash, OTA_FW_SIGN_LEN - crypto_box_ZEROBYTES);

    crypto_box(fw_signature, m, OTA_FW_SIGN_LEN, n, firmware_pkey, server_skey);

#ifdef IMAGE_ONLY
    /*
     *  generate output for flash image
     */

    /* Open the output firmware .bin file */
    updatefile_bin = fopen("ota_flash_image.bin", "w+");

    /* save the signature first */
    fwrite(fw_signature, sizeof(fw_signature), 1, updatefile_bin);

#if (OTA_FW_SIGN_SPACE > OTA_FW_SIGN_LEN)
    uint8_t blank_buffer[OTA_FW_SIGN_SPACE - sizeof(fw_signature)];
    for (unsigned long b = 0; b < sizeof(blank_buffer); b++) {
        blank_buffer[b] = 0xff;
    }
    fwrite(blank_buffer, sizeof(blank_buffer), 1, updatefile_bin);
#endif /* (OTA_FW_SIGN_SPACE > OTA_FW_SIGN_LEN) */

    /* then copy the binary to the output file */
    fseek(slot_bin, 0, SEEK_SET);
    while((bytes_read = fread(firmware_buffer, 1, sizeof(firmware_buffer), slot_bin))) {
        fwrite(firmware_buffer, bytes_read, 1, updatefile_bin);
    }

    /* Close the output file */
    fclose(updatefile_bin);

    /* Close the input .bin file. */
    fclose(slot_bin);

#else /* IMAGE_ONLY */

    /*
     *  set up temp file with inner signature and metadata
     */
    tmp_file = fopen("tmp_file", "w+");
    uint8_t blank_buffer[0x200];

    fwrite(fw_signature, sizeof(fw_signature), 1, tmp_file);
#if (OTA_FW_SIGN_SPACE > OTA_FW_SIGN_LEN)
    for (unsigned long b = 0; b < (OTA_FW_SIGN_SPACE - sizeof(fw_signature)); b++) {
        blank_buffer[b] = 0xff;
    }
    fwrite(blank_buffer, sizeof(blank_buffer), 1, tmp_file);
#endif /* (OTA_FW_SIGN_SPACE > OTA_FW_SIGN_LEN) */

    uint8_t output_buffer[sizeof(OTA_FW_metadata_t)];
    memcpy(output_buffer, (uint8_t*)&metadata, sizeof(OTA_FW_metadata_t));
    fwrite(output_buffer, sizeof(output_buffer), 1, tmp_file);
    for (unsigned long b = 0; b < (OTA_FW_METADATA_SPACE - sizeof(metadata)); b++) {
        blank_buffer[b] = 0xff;
    }
    fwrite(blank_buffer, (OTA_FW_METADATA_SPACE - sizeof(metadata)), 1, tmp_file);

    /*
     *  encrypt the binary data and append to temp file
     */
    // TODO_JB: generate random nonce for crypto_stream encryption!

    fseek(slot_bin, OTA_FW_METADATA_SPACE, SEEK_SET);
    while((bytes_read = fread(firmware_buffer, 1, sizeof(firmware_buffer), slot_bin))) {
        // TODO_JB: use crypto_stream, write to tmp_file

        fwrite(firmware_buffer, bytes_read, 1, tmp_file); // TODO_JB: dummy only
    }

    /*
     *  generate file signature (outer signature)
     */

    /* calculate hash of encrypted binary, the metadata and the firmware signature (temp file) */
    fseek(tmp_file, 0, SEEK_SET);
    sha256_init(&firmware_sha256);
    while((bytes_read = fread(firmware_buffer, 1, sizeof(firmware_buffer), tmp_file))) {
        sha256_update(&firmware_sha256, firmware_buffer, bytes_read);
    }
    sha256_final(&firmware_sha256, sha256_hash);

    /* encrypt the hash and nonce with crypto_box */
    memset(m, 0, crypto_box_ZEROBYTES);
    memcpy(m + crypto_box_ZEROBYTES, sha256_hash, SHA256_DIGEST_LENGTH);
    memcpy(m + crypto_box_ZEROBYTES + SHA256_DIGEST_LENGTH, nonce, crypto_stream_NONCEBYTES);

    crypto_box(file_signature, m, OTA_FILE_SIGN_LEN, n, firmware_pkey, server_skey);

    /*
     *  generate output for flash image
     */
    /* Open the output firmware .bin file */
    updatefile_bin = fopen("ota_update_file.bin", "w+");

    fwrite(file_signature, sizeof(file_signature), 1, updatefile_bin);
#if (OTA_FILE_SIGN_SPACE > OTA_FILE_SIGN_LEN)
    for (unsigned long b = 0; b < (OTA_FILE_SIGN_SPACE - sizeof(file_signature)); b++) {
        blank_buffer[b] = 0xff;
    }
    fwrite(blank_buffer, (OTA_FILE_SIGN_SPACE - sizeof(file_signature)), 1, updatefile_bin);
#endif /* (OTA_FILE_SIGN_SPACE > OTA_FILE_SIGN_LEN) */

    /* then copy the temp file to the output file */
    fseek(tmp_file, 0, SEEK_SET);
    while((bytes_read = fread(firmware_buffer, 1, sizeof(firmware_buffer), tmp_file))) {
        fwrite(firmware_buffer, bytes_read, 1, updatefile_bin);
    }

    /* Close the temp file */
    fclose(tmp_file);

    /* Close the output file */
    fclose(updatefile_bin);

    /* Close the input .bin file. */
    fclose(slot_bin);

#endif /* IMAGE_ONLY */

    return 0;
}
