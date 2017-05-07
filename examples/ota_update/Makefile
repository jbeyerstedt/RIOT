# If no BOARD is found in the environment, use this default:
export BOARD ?= nucleo-f411

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Select, if the flash is sectored or paged
# I can not use the FLASH_SECTORS define if the cpu's Makefile.include, because
# it is not included in this Makefile. So set it here again!
SECTORS = 1

# Activate OTA FW Updates in the Makefiles
OTA_UPDATE = 1

# set FW_SLOTS only as a compiler flag to activate things in cpu.c and cpu_conf.h

OTA_FW_METADATA_SPACE = 0x40    # reserved space for metadata
OTA_FW_SIGN_SPACE = 0x40        # reserved space for inner signature
OTA_FW_HEADER_SPACE = 0x80      # OTA_FW_METADATA_SPACE + OTA_FW_SIGN_SPACE
OTA_FILE_SIGN_SPACE = 0x80      # reserved space for outer signature
OTA_FILE_HEADER_SPACE = 0x100   # OTA_FW_METADATA_SPACE + OTA_FW_SIGN_SPACE + OTA_FILE_SIGN_SPACE
OTA_VTOR_ALIGN = 0x200          # only for VTOR alignment

# Define for which hardware id / article id this firmware should work
HW_ID ?= 0xabc0123456789def

# Define the version number (must be strictly ascending) of this firmware
FW_VERS ?= 0x1
# FW_VERS_2 is used for manual/ testing population of both slots only
FW_VERS_2 ?= 0x2

ifdef SECTORS     # parameters for sectored flash
	# Define the parameters for the FW slot 1
	FW_IMAGE1_OFFSET = 0x08004000   # start after BOOTLOADER_SPACE
	FW_IMAGE1_LENGTH = 0x1C000      # Reserve a few sectors
	FW_IMAGE1_END = 0x08020000      # FW_IMAGE_OFFSET + FW_IMAGE_LENGTH
	FW_IMAGE1_BASE = 0x08004200     # FW_IMAGE1_OFFSET + OTA_VTOR_ALIGN

	# Define the parameters for the FW slot 2
	FW_IMAGE2_OFFSET = 0x08020000   # start after image 1
	FW_IMAGE2_LENGTH = 0x20000      # Reserve sectors 5
	FW_IMAGE2_END = 0x08040000      # FW_IMAGE_OFFSET + FW_IMAGE_LENGTH
	FW_IMAGE2_BASE = 0x08020200     # FW_IMAGE2_OFFSET + OTA_VTOR_ALIGN
else              # parameters for paged flash
	# Define the parameters for the FW slot 1
	FW_IMAGE1_OFFSET = 0x08004000   # Start at page 8
	FW_IMAGE1_LENGTH = 0x3C000      # Reserve 120 pages
	FW_IMAGE1_END = 0x08040000      # FW_IMAGE_OFFSET + FW_IMAGE_LENGTH

	# Define the parameters for the FW slot 2
	FW_IMAGE2_OFFSET = 0x08040000   # Start at page 128
	FW_IMAGE2_LENGTH = 0x3C000      # Reserve 120 pages
	FW_IMAGE2_END = 0x08080000      # FW_IMAGE_OFFSET + FW_IMAGE_LENGTH
endif

OTA_FILE_OFFSET = 0x08060000        # Start at sector 7
OTA_FILE_LENGTH = 0x20000           # Reserve whole sector 7


## some things to make "make term" work
RIOTBOARD      ?= $(RIOTBASE)/boards
override RIOTBASE       := $(abspath $(RIOTBASE))
override RIOTBOARD      := $(abspath $(RIOTBOARD))

ifneq (0,$(shell test -d $(RIOTBOARD)/$(BOARD); echo $$?))
    $(error The specified board $(BOARD) does not exist.)
endif

include $(RIOTBOARD)/$(BOARD)/Makefile.include


### TARGETS TO INITIALIZE ALL TOOLS ###
# compile all necessary tools (dist/tools)
setup-tools: make-keygen make-metagen make-filesign
	@true

# call this one time, if you want to generate new keys for a new device
setup-keys:
	$(RIOTBASE)/dist/tools/nacl_key_generator/bin/./generate-nacl-keys

make-keygen:
	@cd ../../dist/tools/nacl_key_generator; \
	make clean all

make-metagen:
	@cd ../../dist/tools/ota_update_filemeta; \
	CFLAGS="-DOTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE)" make clean all

make-filesign:
	@cd ../../dist/tools/ota_update_filesign; \
	CFLAGS="-DOTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) \
	-DOTA_FW_SIGN_SPACE=$(OTA_FW_SIGN_SPACE) -DOTA_FILE_SIGN_SPACE=$(OTA_FILE_SIGN_SPACE)" \
	make clean all

### TARGETS TO CREATE UPDATE FILES ###
# this does all steps needed to have both update files ready to sign
updatefiles-prepare: ota_update_app-file-slot1 ota_update_app-file-slot2
	@true

# this does all steps needed to have both update files ready to distribute
updatefiles-ready: updatefiles-prepare sign-updatefiles
	@true

sign-updatefiles: sign-update-slot1 sign-update-slot2
	@true

# deploy the update files by sending them to the server
updatefiles-deploy:
	@cp ./fw_update-$(HW_ID)-$(FW_VERS)-s1.bin ../ota_update_server/
	@cp ./fw_update-$(HW_ID)-$(FW_VERS)-s2.bin ../ota_update_server/

sign-update-slot1:
	../../dist/tools/ota_update_filesign/bin/./generate-ota_update_file \
	app_binary-$(HW_ID)-$(FW_VERS)-s1.bin \
	server_skey firmware_pkey.pub; \
	mv ota_update_file.bin fw_update-$(HW_ID)-$(FW_VERS)-s1.bin; \
	rm tmp_file;

sign-update-slot2:
	../../dist/tools/ota_update_filesign/bin/./generate-ota_update_file \
	app_binary-$(HW_ID)-$(FW_VERS)-s2.bin \
	server_skey firmware_pkey.pub; \
	mv ota_update_file.bin fw_update-$(HW_ID)-$(FW_VERS)-s2.bin; \
	rm tmp_file;

### TARGETS FOR COMPILING THE APPLICATIONS SEPERATELY ###
bootloader:
	@cp keys.c ../ota_bootloader
	@cd ../ota_bootloader; \
	CFLAGS="-DOTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) -DHW_ID=$(HW_ID) \
	-DOTA_FW_SIGN_SPACE=$(OTA_FW_SIGN_SPACE) -DOTA_VTOR_ALIGN=$(OTA_VTOR_ALIGN) \
	-DOTA_UPDATE=$(OTA_UPDATE)" \
	make -j 4 clean all; \
	cp bin/$(BOARD)/ota_bootloader.hex ../ota_update

# compile application for manual testing
ota_update_app-test-slot1:
	@cp keys.c ../ota_update_app
	@cd ../ota_update_app; \
	CFLAGS="-DFW_SLOT=1 -DOTA_HW_ID=$(OTA_HW_ID) -DOTA_FW_VERS=$(FW_VERS) \
	-DFW_SLOTS=1 -DOTA_UPDATE=$(OTA_UPDATE) -DOTA_VTOR_ALIGN=$(OTA_VTOR_ALIGN)" \
	FW_SLOT=1 OTA_HW_ID=$(HW_ID) OTA_FW_VERS=$(FW_VERS) \
	OTA_UPDATE=$(OTA_UPDATE) OTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) \
	OTA_FW_BASE_ADDR=$(FW_IMAGE1_BASE) \
	make -j 4 clean all; \
	cp bin/$(BOARD)/app_binary-$(HW_ID)-$(FW_VERS)-s1.bin ../ota_update

# compile application for update file
ota_update_app-file-slot1: ota_update_app-test-slot1
	@true

# compile application for manual testing (use FW_VERS_2)
ota_update_app-test-slot2:
	@cp keys.c ../ota_update_app
	@cd ../ota_update_app; \
	CFLAGS="-DFW_SLOT=2 -DOTA_HW_ID=$(OTA_HW_ID) -DOTA_FW_VERS=$(FW_VERS_2) \
	-DFW_SLOTS=1 -DOTA_UPDATE=$(OTA_UPDATE) -DOTA_VTOR_ALIGN=$(OTA_VTOR_ALIGN)" \
	FW_SLOT=2 OTA_HW_ID=$(HW_ID) OTA_FW_VERS=$(FW_VERS_2) \
	OTA_UPDATE=$(OTA_UPDATE) OTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) \
	OTA_FW_BASE_ADDR=$(FW_IMAGE2_BASE) \
	make -j 4 clean all; \
	cp bin/$(BOARD)/app_binary-$(HW_ID)-$(FW_VERS_2)-s2.bin ../ota_update

# compile application for update file (use FW_VERS)
ota_update_app-file-slot2:
	@cp keys.c ../ota_update_app
	@cd ../ota_update_app; \
	CFLAGS="-DFW_SLOT=2 -DOTA_HW_ID=$(OTA_HW_ID) -DOTA_FW_VERS=$(FW_VERS) \
	-DFW_SLOTS=1 -DOTA_UPDATE=$(OTA_UPDATE) -DOTA_VTOR_ALIGN=$(OTA_VTOR_ALIGN)" \
	FW_SLOT=2 OTA_HW_ID=$(HW_ID) OTA_FW_VERS=$(FW_VERS) \
	OTA_UPDATE=$(OTA_UPDATE) OTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) \
	OTA_FW_BASE_ADDR=$(FW_IMAGE2_BASE) \
	make -j 4 clean all; \
	cp bin/$(BOARD)/app_binary-$(HW_ID)-$(FW_VERS)-s2.bin ../ota_update

### TARGETS WITH ALL IN ONE SERVICE ###
# this does all steps needed to populate both slots (ready to flash)
test-hex: bootloader ota_update_app-test-slot1 ota_update_app-test-slot2 merge-test-hex
	@true

# this does all steps needed to populate both slots (ready to flash)
factory-hex: bootloader ota_update_app-test-slot1 merge-factory-hex
	@true

merge-test-hex: sign-image-slot1 sign-image-slot2
	srec_cat ota_bootloader.hex -intel -crop 0x08000000 $(FW_IMAGE1_OFFSET) \
	fw_image-$(HW_ID)-$(FW_VERS)-s1.bin -binary -offset $(FW_IMAGE1_OFFSET) \
	-crop $(FW_IMAGE1_OFFSET) $(FW_IMAGE1_END) \
	fw_image-$(HW_ID)-$(FW_VERS_2)-s2.bin -binary -offset $(FW_IMAGE2_OFFSET) \
	-crop $(FW_IMAGE2_OFFSET) $(FW_IMAGE2_END) \
	-o hex-slotstest.hex -intel

merge-factory-hex: sign-image-slot1
	srec_cat ota_bootloader.hex -intel -crop 0x08000000 $(FW_IMAGE1_OFFSET) \
	fw_image-$(HW_ID)-$(FW_VERS)-s1.bin -binary -offset $(FW_IMAGE1_OFFSET) \
	-crop $(FW_IMAGE1_OFFSET) $(FW_IMAGE1_END) \
	-o hex-factory.hex -intel

sign-image-slot1:
	../../dist/tools/ota_update_filesign/bin/./generate-ota_flash_image \
	app_binary-$(HW_ID)-$(FW_VERS)-s1.bin \
	server_skey firmware_pkey.pub; \
	mv ota_flash_image.bin fw_image-$(HW_ID)-$(FW_VERS)-s1.bin

sign-image-slot2:
	../../dist/tools/ota_update_filesign/bin/./generate-ota_flash_image \
	app_binary-$(HW_ID)-$(FW_VERS_2)-s2.bin \
	server_skey firmware_pkey.pub; \
	mv ota_flash_image.bin fw_image-$(HW_ID)-$(FW_VERS_2)-s2.bin

### TARGETS TO FLASH THE DEVICE ###
# flashes the test-hex
flash-test:
	OPENOCD_CONFIG=../../boards/$(BOARD)/dist/openocd.cfg \
	HEXFILE=hex-slotstest.hex \
	../../dist/tools/openocd/openocd.sh flash 2>&1

# flashes the factory-hex
flash-factory:
	OPENOCD_CONFIG=../../boards/$(BOARD)/dist/openocd.cfg \
	OPENOCD_PRE_FLASH_CMDS="-c 'flash erase_sector 0 5 5'" \
	HEXFILE=hex-factory.hex \
	../../dist/tools/openocd/openocd.sh flash 2>&1

# flashes the update file for slot 1 in the OTA file space
flash-updatefile-slot1:
	srec_cat fw_update-$(HW_ID)-$(FW_VERS)-s1.bin -binary -offset $(OTA_FILE_OFFSET) \
	-o hex-updatefile-s1.hex -intel
	OPENOCD_CONFIG=../../boards/$(BOARD)/dist/openocd.cfg \
	HEXFILE=hex-updatefile-s1.hex \
	../../dist/tools/openocd/openocd.sh flash 2>&1

# flashes the update file for slot 2 in the OTA file space
flash-updatefile-slot2:
	srec_cat fw_update-$(HW_ID)-$(FW_VERS)-s2.bin -binary -offset $(OTA_FILE_OFFSET) \
	-o hex-updatefile-s2.hex -intel
	OPENOCD_CONFIG=../../boards/$(BOARD)/dist/openocd.cfg \
	HEXFILE=hex-updatefile-s2.hex \
	../../dist/tools/openocd/openocd.sh flash 2>&1

### TARGETS FOR ETHOS COMMUNICATION
# setup tap interface
setup-ethos:
	sudo ip tuntap add tap0 mode tap user $(USER)
	sudo ip link set tap0 up

# start ethos communitation
ethos:
	../../dist/tools/ethos/ethos tap0 /dev/ttyACM0

# serial terminal (copied from Makefile.include)
term:
	@command -v $(TERMPROG) >/dev/null 2>&1 || \
		{ $(COLOR_ECHO) \
		'${COLOR_RED}Terminal program $(TERMPROG) not found. Aborting.${COLOR_RESET}'; \
		exit 1; }
	$(TERMPROG) $(TERMFLAGS)

### TARGETS TO CLEAN
# cleans everything, but leaves the keys
clean:
	@rm *.hex *.bin

# cleans everything
clean-all: clean
	@rm firmware_skey* server_skey* *.pub keys.c
