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
static unsigned char m[OTA_FILE_SIGN_LEN];

static int hex_char_to_int(char hex)
{
    if ((hex >= 0x30) && (hex <= 0x39)) {
        return hex - 0x30;
    }
    else if ((hex >= 0x61) && (hex <= 0x66)) {
        return hex - 0x61 + 10;
    }
    else if ((hex >= 0x41) && (hex <= 0x46)) {
        return hex - 0x41 + 10;
    }
    return -1;
}

int main(int argc, char *argv[])
{
    uint8_t fread_buffer[1024];
    int bytes_read = 0;
    OTA_FW_metadata_t metadata;
    uint8_t fw_metadata_block[OTA_FW_METADATA_SPACE];

    sha256_context_t sha256_ctx;
    uint8_t sha256_hash[SHA256_DIGEST_LENGTH];

    uint8_t fw_signature[OTA_FW_SIGN_LEN];
    uint8_t sign_nonce[crypto_box_NONCEBYTES];

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
        printf("WARNING: THIS IMAGE WILL ONLY WORK ON ONE DEVICE!\n");
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
    }
    else {
        printf("ERROR! Cannot open firmware public key\n");
        return -1;
    }

    printf("Reading server secret key...\n");
    server_sk = fopen(argv[2], "r");
    if (server_sk != NULL) {
        size_t size = fread(server_skey, 1, sizeof(server_skey), server_sk);
        printf("Read %zu bytes from server_skey\n", size);
        fclose(server_sk);
    }
    else {
        printf("ERROR! Cannot open server secret key\n");
        return -1;
    }


    /* open the firmware image .bin file */
    slot_bin = fopen(argv[1], "r");
    if (slot_bin == NULL) {
        printf("ERROR! Cannot open firmware binary file\n");
        return -1;
    }

    /*
     *  if neccesary, set chip id
     */
    /* read metadata section from file */
    bytes_read = fread(fread_buffer, 1, OTA_FW_METADATA_SPACE, slot_bin);
    if (bytes_read < OTA_FW_METADATA_SPACE) {
        printf("ERROR: something went wrong reading the metadata section\n");
        return -1;
    }
    memcpy(&metadata, &fread_buffer, sizeof(OTA_FW_metadata_t));

    if (argv[4]) {
        /* read chip id from argument, write to metadata.chip_id */
        char *char_ptr = argv[4];

        printf("chip_id set to: 0x");

        if (*(char_ptr + 1) == 'x') {     /* skip 0x prefix */
            char_ptr += 2;
        }
        for (uint8_t i = 0; i < (sizeof(metadata.chip_id)); i++) {
            int value = 0;

            value = hex_char_to_int(*(char_ptr++));
            if (value < 0) {
                printf("\nERROR invalid hex character\n");
                return -1;
            }
            metadata.chip_id[i] = ((uint8_t)value << 4) & 0xF0;

            value = hex_char_to_int(*(char_ptr++));
            if (value < 0) {
                printf("\nERROR invalid hex character\n");
                return -1;
            }
            metadata.chip_id[i] |= ((uint8_t)value) & 0x0F;

            printf("%02x ", metadata.chip_id[i]);
        }

        printf("\n");
    }

    /*
     *  copy metadata with correct spacing to fw_metadata_block
     */
    memcpy(fw_metadata_block, (uint8_t *)&metadata, sizeof(OTA_FW_metadata_t));
    for (uint8_t i = sizeof(OTA_FW_metadata_t); i < OTA_FW_METADATA_SPACE; i++) {
        fw_metadata_block[i] = 0xff;
    }

    /*
     *  generate firmware signature (inner signature)
     */
    sha256_init(&sha256_ctx);

    /* begin hash with (modified) metadata block */
    sha256_update(&sha256_ctx, fw_metadata_block, sizeof(fw_metadata_block));

    /* continue hash with binary data from image file */
    fseek(slot_bin, OTA_FW_METADATA_SPACE, SEEK_SET);
    while ((bytes_read = fread(fread_buffer, 1, sizeof(fread_buffer), slot_bin))) {
        sha256_update(&sha256_ctx, fread_buffer, bytes_read);
    }
    sha256_final(&sha256_ctx, sha256_hash);

    /* encrypt hash using crypto_box of tweetNaCl */
    memset(m, 0, crypto_box_ZEROBYTES);
    memcpy(m + crypto_box_ZEROBYTES, sha256_hash, OTA_FW_SIGN_LEN - crypto_box_ZEROBYTES);

    crypto_box(fw_signature, m, OTA_FW_SIGN_LEN, sign_nonce, firmware_pkey, server_skey);

#ifdef IMAGE_ONLY
    /*
     *  generate output for flash image
     */

    /* Open the output firmware .bin file */
    updatefile_bin = fopen("ota_flash_image.bin", "w");

    /* generate spacing for correct VTOR alignment */
    uint8_t spacing_buffer[OTA_VTOR_ALIGN - (OTA_FW_METADATA_SPACE + OTA_FW_SIGN_SPACE)];
    for (unsigned long b = 0; b < sizeof(spacing_buffer); b++) {
        spacing_buffer[b] = 0xff;
    }
    fwrite(spacing_buffer, sizeof(spacing_buffer), 1, updatefile_bin);

    /* save the signature first */
    fwrite(fw_signature, sizeof(fw_signature), 1, updatefile_bin);
#if (OTA_FW_SIGN_SPACE > OTA_FW_SIGN_LEN)
    uint8_t blank_buffer[OTA_FW_SIGN_SPACE - sizeof(fw_signature)];
    for (unsigned long b = 0; b < sizeof(blank_buffer); b++) {
        blank_buffer[b] = 0xff;
    }
    fwrite(blank_buffer, sizeof(blank_buffer), 1, updatefile_bin);
#endif /* (OTA_FW_SIGN_SPACE > OTA_FW_SIGN_LEN) */

    /* save the (potentially) modified metadata */
    fwrite(fw_metadata_block, sizeof(fw_metadata_block), 1, updatefile_bin);

    /* then copy the binary to the output file */
    fseek(slot_bin, OTA_FW_METADATA_SPACE, SEEK_SET); /* set to beginning of binary part */
    while ((bytes_read = fread(fread_buffer, 1, sizeof(fread_buffer), slot_bin))) {
        fwrite(fread_buffer, bytes_read, 1, updatefile_bin);
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

    /* save the signature first */
    fwrite(fw_signature, sizeof(fw_signature), 1, tmp_file);
#if (OTA_FW_SIGN_SPACE > OTA_FW_SIGN_LEN)
    for (unsigned long b = 0; b < (OTA_FW_SIGN_SPACE - sizeof(fw_signature)); b++) {
        blank_buffer[b] = 0xff;
    }
    fwrite(blank_buffer, sizeof(blank_buffer), 1, tmp_file);
#endif /* (OTA_FW_SIGN_SPACE > OTA_FW_SIGN_LEN) */

    /* save the (potentially) modified metadata */
    fwrite(fw_metadata_block, sizeof(fw_metadata_block), 1, tmp_file);

    /*
     *  encrypt the binary data and append to temp file
     */
    // TODO_JB: generate random aes_iv!

    fseek(slot_bin, OTA_FW_METADATA_SPACE, SEEK_SET);
    while ((bytes_read = fread(firmware_buffer, 1, sizeof(firmware_buffer), slot_bin))) {
        // TODO_JB: use crypto_stream, write to tmp_file

        fwrite(firmware_buffer, bytes_read, 1, tmp_file); // TODO_JB: dummy only
    }

    /*
     *  generate file signature (outer signature)
     */

    /* calculate hash of encrypted binary, the metadata and the firmware signature (temp file) */
    fseek(tmp_file, 0, SEEK_SET);
    sha256_init(&sha256_ctx);
    while ((bytes_read = fread(fread_buffer, 1, sizeof(fread_buffer), tmp_file))) {
        sha256_update(&sha256_ctx, fread_buffer, bytes_read);
    }
    sha256_final(&sha256_ctx, sha256_hash);

    /* encrypt the hash and aes_iv with crypto_box */
    memset(m, 0, crypto_box_ZEROBYTES);
    memcpy(m + crypto_box_ZEROBYTES, sha256_hash, SHA256_DIGEST_LENGTH);
    memcpy(m + crypto_box_ZEROBYTES + SHA256_DIGEST_LENGTH, aes_iv, AES_BLOCK_SIZE);

    crypto_box(file_signature, m, OTA_FILE_SIGN_LEN, sign_nonce, firmware_pkey, server_skey);

    /*
     *  generate output for flash image
     */
    /* Open the output firmware .bin file */
    updatefile_bin = fopen("ota_update_file.bin", "w");

    uint32_t magic = OTA_FW_FILE_MAGIC_H;
    fwrite(&magic, sizeof(magic), 1, updatefile_bin);
    magic = OTA_FW_FILE_MAGIC_L;
    fwrite(&magic, sizeof(magic), 1, updatefile_bin);

    fwrite(file_signature, sizeof(file_signature), 1, updatefile_bin);
#if (OTA_FILE_SIGN_SPACE > OTA_FILE_SIGN_LEN)
    for (unsigned long b = 0; b < (OTA_FILE_SIGN_SPACE - sizeof(file_signature)); b++) {
        blank_buffer[b] = 0xff;
    }
    fwrite(blank_buffer, (OTA_FILE_SIGN_SPACE - sizeof(file_signature)), 1, updatefile_bin);
#endif /* (OTA_FILE_SIGN_SPACE > OTA_FILE_SIGN_LEN) */

    /* then copy the temp file to the output file */
    fseek(tmp_file, 0, SEEK_SET);
    while ((bytes_read = fread(fread_buffer, 1, sizeof(fread_buffer), tmp_file))) {
        fwrite(fread_buffer, bytes_read, 1, updatefile_bin);
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
