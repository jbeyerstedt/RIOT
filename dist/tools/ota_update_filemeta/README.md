# Generator for OTA Firmware Update Files, Part 1 - Metadata
This program will generate the metadata section for a OTA firmware update file.
In a second step, the file must be signed by using `ota_update_filesign` to
be accepted by the bootloader or the update module.  
The metadata structure in the update file is defined in the `ota_slots` module
and will be filled with data obtained from the firmware and the input arguments.

## WARNING
This whole process is currently NOT endianess independent!

## Usage
To use, you should call `generate-ota_fw_metadata` with the following arguments:

```console
./bin/generate-ota_fw_metadata firmware.bin fw-vers hw-id fw-base-addr
```

Where:

_firmware.bin:_ The firmware in binary format

_fw-vers:_ The firmware version in 16-bit HEX

_hw-id:_ Unique ID for the application in 64-bit HEX

_fw-base-addr:_ address, where the vector table should be located

The results will be printed if the operation is successful, and a binary
called `ota_fw_metadata.bin` will be created.

The chip unique id of the device, for which this update is meant to work, can
not be set here, because this should be part of the signature generation.

## Notes for Compilation
To setup the metadata generator properly, a few preprocessor constants must be
set via the compiler flags. To set such a constant, it must be appended to the
compiler flags in the following way:
```
-DCNAME=value
```
Where `CNAME` is the name of the constant and `value` is the value.

The following constants are needed:
- OTA_FW_METADATA_SPACE
