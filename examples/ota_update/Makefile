# If no BOARD is found in the environment, use this default:
export BOARD ?= nucleo-f411

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Select, if the flash is sectored or paged
# I can not use the FLASH_SECTORS define if the cpu's Makefile.include, because
# it is not included in this Makefile. So set it here again!
SECTORS = 1

# Activate FW slots
FW_SLOTS = 1

FW_METADATA_SPACE = 0x200  # 76 bytes meta-data, 512 byte aligned

# Define the parameters for the FW slot 1
VERSION_IMG1 = 0x1
UUID_IMG1 = 0xabcd1234

# Define the parameters for the FW slot 2
VERSION_IMG2 = 0x2
UUID_IMG2 = 0xabcd1234

ifdef SECTORS     # parameters for sectored flash
	# Define the parameters for the FW slot 1
	FW_IMAGE1_OFFSET = 0x08004000   # start after BOOTLOADER_SPACE
	FW_IMAGE1_LENGTH = 0x1C000      # Reserve a few sectors
	FW_IMAGE1_END = 0x08020000   # FW_IMAGE_OFFSET + FW_IMAGE_LENGTH

	# Define the parameters for the FW slot 2
	FW_IMAGE2_OFFSET = 0x08020000   # start after image 1
	FW_IMAGE2_LENGTH = 0x20000      # Reserve a few sectors
	FW_IMAGE2_END = 0x08040000   # FW_IMAGE_OFFSET + FW_IMAGE_LENGTH
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

metadata-generator:
	@cd ../../dist/tools/ota_update_filegen; \
	CFLAGS="-DFW_METADATA_SPACE=$(FW_METADATA_SPACE)" make clean all

bootloader:
	@cd ../bootloader; \
	CFLAGS="-DFW_METADATA_SPACE=$(FW_METADATA_SPACE)" make -j clean all; \
	cp bin/$(BOARD)/bootloader.hex ../ota_update

ota_update_app-slot1:
	@cd ../ota_update_app; \
	CFLAGS="-DFW_SLOT=1 -DVERSION=$(VERSION_IMG1) -DUUID=$(UUID_IMG1) \
	-DFW_SLOTS=$(FW_SLOTS)" FW_SLOTS=$(FW_SLOTS) \
	FW_METADATA_SPACE=$(FW_METADATA_SPACE) FW_SLOT=1 \
	VERSION=$(VERSION_IMG1) UUID=$(UUID_IMG1) \
	make -j clean all; \
	cp bin/$(BOARD)/slot-image-$(UUID_IMG1)-$(VERSION_IMG1).bin ../ota_update

ota_update_app-slot2:
	@cd ../ota_update_app; \
	CFLAGS="-DFW_SLOT=2 -DVERSION=$(VERSION_IMG2) -DUUID=$(UUID_IMG2) \
	-DFW_SLOTS=$(FW_SLOTS) -O0 -g2" FW_SLOTS=$(FW_SLOTS) \
	FW_METADATA_SPACE=$(FW_METADATA_SPACE) FW_SLOT=2 \
	VERSION=$(VERSION_IMG2) UUID=$(UUID_IMG2) \
	make -j clean all; \
	cp bin/$(BOARD)/slot-image-$(UUID_IMG2)-$(VERSION_IMG2).bin ../ota_update

merge-binary:
	srec_cat bootloader.hex -intel -crop 0x08000000 $(FW_IMAGE1_OFFSET) \
	slot-image-$(UUID_IMG1)-$(VERSION_IMG1).bin -binary -offset $(FW_IMAGE1_OFFSET) \
	-crop $(FW_IMAGE1_OFFSET) $(FW_IMAGE1_END) \
	slot-image-$(UUID_IMG2)-$(VERSION_IMG2).bin -binary -offset $(FW_IMAGE2_OFFSET) \
	-crop $(FW_IMAGE2_OFFSET) $(FW_IMAGE2_END) \
	-o firmware-slots.hex -intel

master-hex: bootloader ota_update_app-slot1 ota_update_app-slot2 merge-binary
	@true

clean:
	@rm *.hex *.bin

flash:
	OPENOCD_CONFIG=../../boards/$(BOARD)/dist/openocd.cfg \
	HEXFILE=firmware-slots.hex \
	../../dist/tools/openocd/openocd.sh flash
