RIOTBASE := ../../..
RIOT_INCLUDE = $(RIOTBASE)/sys/include

GITCACHE = ../git/git-cache
TWEETNACL_URL = https://github.com/RIOT-OS/tweetnacl.git
TWEETNACL_VERSION = 7ea05c7098a16c87fa66e9166ce301666f3f2623

SHA256_DIR := $(RIOTBASE)/sys/hashes
AES_DIR := $(RIOTBASE)/sys/crypto
CBC_DIR := $(RIOTBASE)/sys/crypto/modes
TWEETNACL_DIR := tweetnacl
TWEETNACL_INC := tweetnacl
TWEETNACL_SRC := $(TWEETNACL_DIR)/tweetnacl.c randombytes.c

SOURCES := generate-ota_update_file.c $(SHA256_DIR)/sha256.c $(CBC_DIR)/cbc.c $(AES_DIR)/ciphers.c $(AES_DIR)/aes.c $(TWEETNACL_SRC)
CFLAGS += -g -O3 -Wall -Wextra -pedantic -std=c99 -DCRYPTO_AES

.PHONY: all bin bin/generate-ota_update_file bin/generate-ota_flash_image

all: clean bin bin/generate-ota_update_file bin/generate-ota_flash_image

bin:
	mkdir bin

git-fetch-tweetnacl:
	rm -Rf $(TWEETNACL_DIR)
	mkdir -p $(TWEETNACL_DIR)
	$(GITCACHE) clone "$(TWEETNACL_URL)" "$(TWEETNACL_VERSION)" "$(TWEETNACL_DIR)"
	touch $@

bin/generate-ota_update_file: git-fetch-tweetnacl
	$(CC) $(CFLAGS) -I$(RIOT_INCLUDE) -I$(TWEETNACL_INC) $(SOURCES) -o $@

bin/generate-ota_flash_image: CFLAGS += -DIMAGE_ONLY=1
bin/generate-ota_flash_image: git-fetch-tweetnacl
	$(CC) $(CFLAGS) -I$(RIOT_INCLUDE) -I$(TWEETNACL_INC) $(SOURCES) -o $@

clean:
	rm -rf bin/
