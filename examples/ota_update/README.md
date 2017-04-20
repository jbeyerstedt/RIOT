# ota_update example

This example provides a Makefile for developing and testing applications which
use a OTA firmware update process and the ota_updater module.

## Usage
Nearly everything to use the OTA update process is controlled with this
Makefile. Nothing must be set in the source and header files.

The only thing to set manually outside the Makefile are the memory borders,
which must be declared in the linker scripts of your board.


### Options to set
At the top of the Makefile are some options, which configure the whole process.
Currently only sectored flash is fully supported, which is set by `SECTORS = 1`.

Because the Makefile scripts can not calculate numbers, some variables have
their calculation rule as a comment. These must be changed accordingly, if
anything was modified.

##### some memory sizes
```
OTA_FW_METADATA_SPACE = 0x40    # reserved space for metadata
OTA_FW_SIGN_SPACE = 0x40        # reserved space for inner signature
OTA_FW_HEADER_SPACE = 0x80      # OTA_FW_METADATA_SPACE + OTA_FW_SIGN_SPACE
OTA_FILE_SIGN_SPACE = 0x80      # reserved space for outer signature
OTA_FILE_HEADER_SPACE = 0x100   # OTA_FW_METADATA_SPACE + OTA_FW_SIGN_SPACE + OTA_FILE_SIGN_SPACE
OTA_VTOR_ALIGN = 0x200          # only for VTOR alignment
```

##### options for manufacturer specific settings
- `HW_ID`: Define for which hardware id / article id this firmware should work (8 Byte hex)
- `FW_VERS`: Define the version number (must be strictly ascending) of this firmware (2 Byte hex)
- `FW_VERS_2`: for manual testing where both slots will be populated without installins via an update file

##### addresses and sizes of the memory slots
- `FW_IMAGE1_OFFSET`: start address of slot 1 (after BOOTLOADER_SPACE)
- `FW_IMAGE1_LENGTH`: size of slot 1
- `FW_IMAGE1_END`: FW_IMAGE_OFFSET + FW_IMAGE_LENGTH
- `FW_IMAGE1_BASE`: FW_IMAGE1_OFFSET + OTA_VTOR_ALIGN
- `FW_IMAGE2_OFFSET`: usually the same as FW_IMAGE1_END
- `FW_IMAGE2_LENGTH`: size of slot 2
- `FW_IMAGE2_END`: FW_IMAGE_OFFSET + FW_IMAGE_LENGTH
- `FW_IMAGE2_BASE`: FW_IMAGE2_OFFSET + OTA_VTOR_ALIGN

##### address and size of the space for the update file
- `OTA_FILE_OFFSET`: start address
- `OTA_FILE_LENGTH`: size of the reserved memory



### Makefile Targets explained
The list of Makefile targets is divided in some sections for a better overview,
so this manual follows the same sections.

#### Setup Tasks
There are two all-in-one solutions for the setup of the process using firmware
update files:
- `setup-tools`: compiles all necessary tools of `/dist/tools`
- `setup-keys`: generates the server and device keypairs for the update file signatures

The tasks of `setup-tools` can also be done individually:
- `make-keygen`: compiles the key generator
- `make-metagen`: compiles the metadata generator
- `make-filesign`: compiles the file signature generator

#### Creation of Update Files
Normally, these targets are enough for generating update files:
- `updatefiles-prepare`: prepares the update files for being signed. (recompiles the application)

In a production environment, the update file signing should be done by a
seperate signing server. In development this could be done by these commands:
- `updatefiles-ready`: compines `updatefiles-prepare` and `sign-updatefiles`
- `sign-updatefiles`: signs the update files

#### Manual Compilation of the Different Applications
Sometimes it is necessary to do some things manually:
- `bootloader`: compiles the bootloader
- `ota_update_app-test-slot1`: compiles the application for slot 1 with the normal version number
- `ota_update_app-file-slot1`: same as `ota_update_app-test-slot1` for integrity reasons
- `ota_update_app-test-slot2`: compiles the application for slot 2 with the second version number
- `ota_update_app-file-slot2`: compiles the application for slot 2 with the normal version number

#### ALL-IN-ONE Service
These Makefile targets will generate hex files, which are ready to be flashed on
the device.
- `test-hex`: hex file with both slots populated using different firmware version numbers
- `factory-hex`: hex file for initial flashing of the device at the factory

There are some helper targets needed internally:
- `merge-test-hex`: signs both firmware slots images and merges them with the bootloader
- `merge-factory-hex`: signs one firmware slot image and merges it with the bootloader
- `sign-image-slot1`: signs the application binary for direct flashing to slot 1 (firmware-image)
- `sign-image-slot1`: signs the application binary for direct flashing to slot 2 (firmware-image)

#### Flash the Device
Finally, there are some targets to actually flash the connected device:
- `flash-test`: flashes the `test-hex`
- `flash-factory`: flashes the `test-factory`
- `flash-updatefile-slot1`: flashes an update file for slot 1 in the OTA file space
- `flash-updatefile-slot2`: flashes an update file for slot 2 in the OTA file space

#### Ethos Communication
Because ethos is used for bringing network to the device, there are some targets
availabe for your convinience:
- `setup-ethos`: sets up a tap interface (uses sudo)
- `ethos`: start the communitation

#### Clean up Stuff
- `clean`: cleans the generated files, but leaves the keys
- `clean-all`: cleans everything, including the keys
