# Generator for OTA Firmware Update Files, Part 2 - Signature
This program will sign the binary prepared by `ota_update_filemeta` with the
keypairs located in the same directory.  
Because a different file is needed for flashing the slots directly than the file
used for OTA updates, there are two executables generated. The usage is the same
so you only have to change the name of the executable.

## Usage
The following examples are written for `generate-ota_upate_file`, but since the
arguments are the same, everything also applies to `generate-ota_flash_image`.

To use, you should call `generate-ota_update_file` with the following arguments:

```console
./bin/generate-ota_upate_file slot-binary.bin server-skey device-pkey [chip-id]
```

Where:

_fw-slot-unsigned.bin:_ The firmware with the metadata from
                        `ota_update_filemeta` in front

_server-skey:_ Path to the server's secret key file

_device-pkey:_ Path to the device's public key file

_chip-id:_ (optional) The chip serial number, for which the firmware should be
                     signed. Maximum 16 bytes in HEX

The results will be printed if the operation is successful, and a binary
called `ota_update_file.bin` will be created.

(If `generate-ota_flash_image` is used, the binary output file is named
`ota_flash_image.bin`.)

## Notes for Compilation
To setup this Update File Signer properly, a few preprocessor constants must be
set via the compiler flags. To set such a constant, it must be appended to the
compiler flags in the following way:
```
-DCNAME=value
```
Where `CNAME` is the name of the constant and `value` is the value.

The following constants are needed:
- OTA_FW_METADATA_SPACE
- OTA_FW_SIGN_SPACE
- OTA_FILE_SIGN_SPACE
