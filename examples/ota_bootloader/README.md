# ota_bootloader example

## Usage
TODO

## Notes for Compilation
To setup the metadata generator properly, a few preprocessor constants must be
set via the compiler flags. To set such a constant, it must be appended to the
compiler flags in the following way:
```
-DCNAME=value
```
Where `CNAME` is the name of the constant and `value` is the value.

The following constants are needed:
- OTA_VTOR_ALIGN
- OTA_FW_METADATA_SPACE
- OTA_FW_SIGN_SPACE
- HW_ID
