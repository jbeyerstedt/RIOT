# name of your application
APPLICATION = ota_bootloader

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-f411

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
#CFLAGS += -DDEVELHELP

# Mark this example as the bootloader
BOOTLOADER = 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += shell
USEMODULE += xtimer

# Use ota_slots module to manage images on ROM
USEMODULE += ota_slots

# TweetNaCl needs a lot of stack
CFLAGS += '-DTHREAD_STACKSIZE_MAIN=(THREAD_STACKSIZE_DEFAULT + 2048)'

include $(RIOTBASE)/Makefile.include
