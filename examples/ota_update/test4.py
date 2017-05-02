#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

# module for integration in tests.py. No standalone use intended

import subprocess
import time
import sys
import os
import signal

from nbstreamreader import NonBlockingStreamReader as NBSR
from test_bootloader_helpers import bootloader_init, bootloader_get_cmd


def do_test(tty_out):
    subprocess.call("cp -p fw_image-0xabc0123456789def-0x2-s2.bin fw_image-orig-0x2-s2", shell=True)

    ## prepare everything for this test - manipulate slot 2 fw_image for signature mismatch
    subprocess.call("FW_VERS=0x2 make sign-image-slot2 >" + tty_out, shell=True)
    subprocess.call("cat fw_image-orig-0x2-s2 | head -c -16 >fw_image-0xabc0123456789def-0x2-s2.bin", shell=True)
    subprocess.call("FW_VERS=0x1 FW_VERS_2=0x2 make -f test-Makefile test4-hex >" + tty_out, shell=True)

    print("(Step 1) initial flashing with factory firmware")
    ## flash the update file
    if subprocess.call("FW_VERS=0x2 make flash-updatefile-slot2 >" + tty_out, shell=True):
        clean()
        return -1
    time.sleep(1)
    print("  [INFO] update file flashed")

    ## start serial communication with bootloader
    if bootloader_init("/dev/ttyACM0"):
        clean()
        return -1
    print("  [INFO] bootloader communication running, flashing test-hex now")

    ## flash the devive with factory-hex
    if subprocess.call("FW_VERS=0x1 FW_VERS_2=0x2 make flash-test >" + tty_out, shell=True):
        clean()
        return -1
    # time.sleep(0.5)
    print("  [INFO] test hex flashed")

    print(" ==>[OK] firmware flashed and communication interfaces started\n")


    print("(Step 2) checking bootloader messages")

    ret_val, answer = bootloader_get_cmd("welcome to OTA bootloader")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] could not get bootloader welcome message")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1

    ret_val, answer = bootloader_get_cmd("both slots populated")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] bootloader has reported wrong number of populated slots")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] both slots detected")

    ret_val, answer = bootloader_get_cmd("reset flag normal, validating signature")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] watchdog status was not correct")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] correct watchdog status detected")

    ret_val, answer = bootloader_get_cmd("slot 2 inconsistent")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] inconsistent slot 2 not detected by bootloader")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] inconsistent slot 2 detected")

    ret_val, answer = bootloader_get_cmd("incomplete installation detected, erasing slot")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] wrong decision after detecting the invalid signature")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] attempting to erase slot 2")

    ret_val, answer = bootloader_get_cmd("booting FW slot 1")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] no attempt to boot slot 1 reported")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] booting slot 1")

    ret_val, answer = bootloader_get_cmd("OTA update module example, FW version 1")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] firmware in slot 1 has not started")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] slot 1 (fw_vers 1) successfully started")

    print(" ==>[OK] bootloader behaviour for an invalid slot successfully validated\n")

    clean()
    return 0


def clean():
    # tidy up
    subprocess.call("rm fw_image-0xabc0123456789def-0x2-s2.bin", shell=True)
    subprocess.call("mv fw_image-orig-0x2-s2 fw_image-0xabc0123456789def-0x2-s2.bin", shell=True)
