# Generator for OTA Firmware Update Files
This program will generate a OTA firmware update file or a binary file for
initial flashing of the board and debugging.  
The metadata structure in the update file is defined in the fw_slots module and
is defined as followed:

```c
typedef struct FW_metadata {
    uint8_t hash[SHA256_DIGEST_LENGTH]; /* SHA256 Hash of firmware image */
    uint8_t shash[SIGN_LEN];            /* Signed SHA256 */
    uint32_t size;                      /* Size of firmware image */
    uint32_t uuid;                      /* Integer representing unique firmware ID */
    uint16_t version;                   /* Integer representing firmware version */
} FW_metadata_t;
```

This structure will be filled with the data obtained from the firmware.

## Usage
To use, you should call `generate-metadata` with the following arguments:

```console
./generate-metadata <firmware.bin> <version> <uuid>
```

Where:

_<firmware.bin>:_ The firmware in binary format

_<version>:_ The firmware version in 16-bit HEX

_<uuid>_ Unique ID for the application in 32-bit HEX

The results will be printed if the operation is successful, and a binary
called "firmware-metadata.bin" will be created.
