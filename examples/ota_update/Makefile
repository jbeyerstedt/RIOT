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

OTA_FW_METADATA_SPACE = 0x40    # reserved space for metadata
OTA_FW_SIGN_SPACE = 0x40        # reserved space for inner signature
OTA_FW_HEADER_SPACE = 0x80      # OTA_FW_METADATA_SPACE + OTA_FW_SIGN_SPACE
OTA_FILE_SIGN_SPACE = 0x80      # reserved space for outer signature
OTA_FILE_HEADER_SPACE = 0x100   # OTA_FW_METADATA_SPACE + OTA_FW_SIGN_SPACE + OTA_FILE_SIGN_SPACE
OTA_VTOR_ALIGN = 0x200          # only for VTOR alignment

# Define for which hardware id / article id this firmware should work
HW_ID = 0xabc0123456789def

# Define the parameters for the FW slot 1
VERSION_IMG1 = 0x1

# Define the parameters for the FW slot 2
VERSION_IMG2 = 0x2

ifdef SECTORS     # parameters for sectored flash
	# Define the parameters for the FW slot 1
	FW_IMAGE1_OFFSET = 0x08004000   # start after BOOTLOADER_SPACE
	FW_IMAGE1_LENGTH = 0x1C000      # Reserve a few sectors
	FW_IMAGE1_END = 0x08020000      # FW_IMAGE_OFFSET + FW_IMAGE_LENGTH
	FW_IMAGE1_BASE = 0x08004200     # FW_IMAGE1_OFFSET + OTA_VTOR_ALIGN

	# Define the parameters for the FW slot 2
	FW_IMAGE2_OFFSET = 0x08020000   # start after image 1
	FW_IMAGE2_LENGTH = 0x20000      # Reserve a few sectors
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

generate-keys:
	$(RIOTBASE)/dist/tools/nacl_key_generator/bin/./generate-nacl-keys

setup-tools: file-metagen file-signer key-generator
	@true

key-generator:
	@cd ../../dist/tools/nacl_key_generator; \
	make clean all

file-metagen:
	@cd ../../dist/tools/ota_update_filemeta; \
	CFLAGS="-DOTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE)" make clean all

file-signer:
	@cd ../../dist/tools/ota_update_filesign; \
	CFLAGS="-DOTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) \
	-DOTA_FW_SIGN_SPACE=$(OTA_FW_SIGN_SPACE) -DOTA_FILE_SIGN_SPACE=$(OTA_FILE_SIGN_SPACE)" \
	make clean all

bootloader:
	@cp keys.c ../ota_bootloader
	@cd ../ota_bootloader; \
	CFLAGS="-DOTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) -DHW_ID=$(HW_ID) \
	-DOTA_FW_SIGN_SPACE=$(OTA_FW_SIGN_SPACE) -DOTA_VTOR_ALIGN=$(OTA_VTOR_ALIGN)" \
	make -j clean all; \
	cp bin/$(BOARD)/ota_bootloader.hex ../ota_update

# only set FW_SLOTS as compiler flag to activate things in cpu.c and cpu_conf.h
ota_update_app-slot1:
	@cd ../ota_update_app; \
	CFLAGS="-DFW_SLOT=1 -DVERSION=$(VERSION_IMG1) -DUUID=$(UUID_IMG1) \
	-DFW_SLOTS=1 -DOTA_UPDATE=$(OTA_UPDATE)" OTA_UPDATE=$(OTA_UPDATE) \
	OTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) FW_SLOT=1 \
	OTA_HW_ID=$(HW_ID) OTA_FW_VERS=$(VERSION_IMG1) \
	OTA_FW_BASE_ADDR=$(FW_IMAGE1_BASE) \
	make -j clean all; \
	cp bin/$(BOARD)/slot-binary-$(HW_ID)-$(VERSION_IMG1).bin ../ota_update

# only set FW_SLOTS as compiler flag to activate things in cpu.c and cpu_conf.h
ota_update_app-slot2:
	@cd ../ota_update_app; \
	CFLAGS="-DFW_SLOT=2 -DVERSION=$(VERSION_IMG2) -DUUID=$(UUID_IMG2) \
	-DFW_SLOTS=1 -DOTA_UPDATE=$(OTA_UPDATE)" OTA_UPDATE=$(OTA_UPDATE) \
	OTA_FW_METADATA_SPACE=$(OTA_FW_METADATA_SPACE) FW_SLOT=2 \
	OTA_HW_ID=$(HW_ID) OTA_FW_VERS=$(VERSION_IMG2) \
	OTA_FW_BASE_ADDR=$(FW_IMAGE2_BASE) \
	make -j clean all; \
	cp bin/$(BOARD)/slot-binary-$(HW_ID)-$(VERSION_IMG2).bin ../ota_update

sign_binary-slot1:
	../../dist/tools/ota_update_filesign/bin/./generate-ota_flash_image \
	slot-binary-$(HW_ID)-$(VERSION_IMG1).bin \
	server_skey firmware_pkey.pub; \
	mv ota_flash_image.bin slot-image-$(HW_ID)-$(VERSION_IMG1).bin

sign_binary-slot2:
	../../dist/tools/ota_update_filesign/bin/./generate-ota_flash_image \
	slot-binary-$(HW_ID)-$(VERSION_IMG2).bin \
	server_skey firmware_pkey.pub; \
	mv ota_flash_image.bin slot-image-$(HW_ID)-$(VERSION_IMG2).bin

sign-update-files:
	../../dist/tools/ota_update_filesign/bin/./generate-ota_update_file \
	slot-binary-$(HW_ID)-$(VERSION_IMG1).bin \
	server_skey firmware_pkey.pub; \
	mv ota_update_file.bin fw_update-$(HW_ID)-$(VERSION_IMG1).bin; \
	rm tmp_file; \
	../../dist/tools/ota_update_filesign/bin/./generate-ota_update_file \
	slot-binary-$(HW_ID)-$(VERSION_IMG2).bin \
	server_skey firmware_pkey.pub; \
	mv ota_update_file.bin fw_update-$(HW_ID)-$(VERSION_IMG2).bin; \
	rm tmp_file;

merge-binary: sign_binary-slot1 sign_binary-slot2
	srec_cat ota_bootloader.hex -intel -crop 0x08000000 $(FW_IMAGE1_OFFSET) \
	slot-image-$(HW_ID)-$(VERSION_IMG1).bin -binary -offset $(FW_IMAGE1_OFFSET) \
	-crop $(FW_IMAGE1_OFFSET) $(FW_IMAGE1_END) \
	slot-image-$(HW_ID)-$(VERSION_IMG2).bin -binary -offset $(FW_IMAGE2_OFFSET) \
	-crop $(FW_IMAGE2_OFFSET) $(FW_IMAGE2_END) \
	-o firmware-slots.hex -intel

merge-factory-binary: sign_binary-slot1
	srec_cat ota_bootloader.hex -intel -crop 0x08000000 $(FW_IMAGE1_OFFSET) \
	slot-image-$(HW_ID)-$(VERSION_IMG1).bin -binary -offset $(FW_IMAGE1_OFFSET) \
	-crop $(FW_IMAGE1_OFFSET) $(FW_IMAGE1_END) \
	-o factory-firmware.hex -intel

master-hex: bootloader ota_update_app-slot1 ota_update_app-slot2 merge-binary
	@true

factory-hex: bootloader ota_update_app-slot1 merge-factory-binary
	@true

update-files: ota_update_app-slot1 ota_update_app-slot2 sign-update-files
	@true

clean:
	@rm *.hex *.bin firmware_skey* server_skey* keys.c *.pub

flash:
	OPENOCD_CONFIG=../../boards/$(BOARD)/dist/openocd.cfg \
	HEXFILE=firmware-slots.hex \
	../../dist/tools/openocd/openocd.sh flash

factory-flash:
	OPENOCD_CONFIG=../../boards/$(BOARD)/dist/openocd.cfg \
	HEXFILE=factory-firmware.hex \
	../../dist/tools/openocd/openocd.sh flash
