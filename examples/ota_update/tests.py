#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

# for python 2.6 and later. python 3 not tested

# TODO: option to jump over compilation (or simply use a seperate script)
# TODO: option to redirect output of compiling and ethos to another tty (insted of /dev/null)
# TODO: option to do individual tests only


import subprocess
import sys
import os
import signal

from test_ethos_helpers import ethos_command
import test1
import test2
import test3
import test4


##### SOME FUNCTIONS #####
def signal_handler(signal, frame):
    sys.exit(0)
signal.signal(signal.SIGINT, signal_handler)


##### START OF PROGRAM #####
print("Welcome to the ota update tests")

### COMPILING: make all necessary tools and files first
print("\n------------------------------------")
print("---  COMPILING NECESSARY FILES   ---")
print("------------------------------------")
# subprocess.call("make bootloader >/dev/null", shell=True); print("compiling part 1 done");
# subprocess.call("FW_VERS=0x1 make ota_update_app-file-slot1 >/dev/null", shell=True); print("compiling part 2 done");
# subprocess.call("FW_VERS=0x2 make ota_update_app-file-slot1 >/dev/null", shell=True); print("compiling part 3 done");
# subprocess.call("FW_VERS=0x2 make ota_update_app-file-slot2 >/dev/null", shell=True); print("compiling part 4 done");
# subprocess.call("FW_VERS=0x3 make ota_update_app-file-slot1 >/dev/null", shell=True); print("compiling part 5 done");
# subprocess.call("FW_VERS=0x3 make ota_update_app-file-slot2 >/dev/null", shell=True); print("compiling part 6 done");
# subprocess.call("FW_VERS=0x4 make ota_update_app-file-slot1 >/dev/null", shell=True); print("compiling part 7 done");
# subprocess.call("FW_VERS=0x4 make ota_update_app-file-slot2 >/dev/null", shell=True); print("compiling part 8 done");
#
# print("now signing the update files")
# subprocess.call("FW_VERS=0x1 make sign-update-slot1 >/dev/null", shell=True)
# subprocess.call("FW_VERS=0x2 make sign-update-slot1 >/dev/null", shell=True)
# subprocess.call("FW_VERS=0x2 make sign-update-slot2 >/dev/null", shell=True)
# subprocess.call("FW_VERS=0x3 make sign-update-slot1 >/dev/null", shell=True)
# subprocess.call("FW_VERS=0x3 make sign-update-slot2 >/dev/null", shell=True)
# subprocess.call("FW_VERS=0x4 make sign-update-slot1 >/dev/null", shell=True)
# subprocess.call("FW_VERS=0x4 make sign-update-slot2 >/dev/null", shell=True)



### TEST 1 (normal/ standard behaviour)
# dependencies: bootloader
#               app_binary-*-0x1-s1 for fw_image-*-0x1-s1
#               app_binary-*-0x2-s2 for fw_update-*-0x2-s2
#               app_binary-*-0x3-s1 for fw_update-*-0x3-s1
#               app_binary-*-0x4-s2 for fw_update-*-0x4-s2
print("\n------------------------------------")
print("---            TEST 1            ---")
print("---    normal update procedure   ---")
print("------------------------------------")
if test1.do_test() != 0:
    sys.exit(-1)



### TEST 2 (keep in mind, that fw_update-*-0x4-s1 is not valid after this test!!)
# dependencies: bootloader
#               app_binary-*-0x2-s1 for fw_image-*-0x2-s1
#               app_binary-*-0x3-s2 for fw_image-*-0x3-s2
#               manipulated fw_update-*-0x4-s1
#               app_binary-*-0x4-s1 (with wrong hw_id) for fw_update-*-0x4-s1
#               app_binary-*-0x1-s1 for fw_update-*-0x1-s1
print("\n------------------------------------")
print("---            TEST 2            ---")
print("---  misconfigured update files  ---")
print("------------------------------------")
### prepare some things first
if test2.prepare() != 0:
    sys.exit(-1)


### Test 2a (update file with invalid file signature)
if test2.do_part_a() != 0:
    sys.exit(-1)


### Test 2b (update file with invalid hw_id)
if test2.do_part_b() != 0:
    sys.exit(-1)


### Test 2c (update file with lower fw_vers)
if test2.do_part_c() != 0:
    sys.exit(-1)


### tidy up
if test2.finish() != 0:
    sys.exit(-1)



### TEST 3 (power outage test)
# dependencies: bootloader
#               app_binary-*-0x1-s1 for fw_image-*-0x1-s1
#               app_binary-*-0x2-s2 for fw_update-*-0x2-s2
print("\n------------------------------------")
print("---            TEST 3            ---")
print("---  power outage during update  ---")
print("------------------------------------")
subprocess.call("FW_VERS=0x1 make merge-factory-hex >/dev/null", shell=True)

# TODO: everything



### TEST 4 (bitflip in flash)
# dependencies: bootloader
#               app_binary-*-0x1-s1 for fw_image-*-0x1-s1
#               manipulated fw_image-*-0x2-s2
#               app_binary-*-0x2-s2 for fw_update-*-0x2-s2
print("\n------------------------------------")
print("---            TEST 4            ---")
print("---  bitflip in installed update ---")
print("------------------------------------")
subprocess.call("FW_VERS=0x2 FW_VERS_2=0x3 make merge-test-hex >/dev/null", shell=True)

# TODO: everything
