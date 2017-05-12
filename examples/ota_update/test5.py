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

from test_bootloader_helpers import bootloader_init, bootloader_get_cmd


def do_test(tty_out):
    subprocess.call("cp -p app_binary-0xabc0123456789def-0x2-s2.bin app_binary-orig-0x2-s2", shell=True)
    subprocess.call("cp -p fw_image-0xabc0123456789def-0x2-s2.bin fw_image-orig-0x2-s2", shell=True)

    ## prepare everything for this test - compile a
    print("compiling a broken example ...");
    subprocess.call("FW_VERS=0x2 make -f test-Makefile test5-app>" + tty_out, shell=True);
    print("compiling done");
    subprocess.call("FW_VERS=0x1 FW_VERS_2=0x2 make merge-test-hex >" + tty_out, shell=True)

    print("(Step 1) initial flashing with factory firmware")
    ## start serial communication with bootloader
    if bootloader_init("/dev/ttyACM0"):
        clean()
        return -1
    print("  [INFO] bootloader communication running, flashing test-hex now")

    ## flash the update file
    if subprocess.call("FW_VERS=0x1 FW_VERS_2=0x2 make flash-test >" + tty_out, shell=True):
        clean()
        return -1
    time.sleep(1)
    print("  [INFO] test-hex flashed")

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

    ret_val, answer = bootloader_get_cmd("booting FW slot 2")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] no attempt to boot slot 2 reported")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] booting slot 2, waiting for watchdog to expire ...")


    # wait 4 seconds (equals watchdog timeout) for watchdog to trigger
    ret_val, answer = bootloader_get_cmd("welcome to OTA bootloader", timeout=4)
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] bootloader was not started after watchdog reset")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] bootlooader was started again, because of watchdog!?")


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

    ret_val, answer = bootloader_get_cmd("reset reason was watchdog, installed FW is not executable")
    if ret_val < 0:
        print(" [ERROR] no answer from bootloader")
        clean()
        return -1
    elif ret_val == 0:
        print(" [ERROR] the triggered watchdog was not detected")
        print("dumping fetched answer from bootloader:\n" + answer)
        clean()
        return -1
    print("    [OK] the triggered watchdog was detected properly")

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

    print(" ==>[OK] bootloader behaviour for an inresponsive firmware successfully validated\n")

    clean()
    return 0


def clean():
    # tidy up
    subprocess.call("rm fw_image-0xabc0123456789def-0x2-s2.bin", shell=True)
    subprocess.call("mv fw_image-orig-0x2-s2 fw_image-0xabc0123456789def-0x2-s2.bin", shell=True)
    subprocess.call("rm app_binary-0xabc0123456789def-0x2-s2.bin", shell=True)
    subprocess.call("mv app_binary-orig-0x2-s2 app_binary-0xabc0123456789def-0x2-s2.bin", shell=True)
