#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

# for python 2.7 and later. python 3 not tested


import subprocess
import sys
import os
import signal
import argparse

import test1
import test2
import test3
import test4

### parse the command line arguments
print("----------         Welcome to the OTA Update Tests          ----------")

parser = argparse.ArgumentParser()
parser.add_argument("-t", "--test", help="execute a single test only. give test id {1-4}",
                    action="append")
parser.add_argument("-o", help="(tty) output for otherwise suppressed output of compiling and flashing commands")
group = parser.add_mutually_exclusive_group()
group.add_argument("-f", "--fast", help="skip compiling all files (counterpart to -p option)",
                    action="store_true")
group.add_argument("-p", "--prepare", help="only prepare all binaries and update files",
                    action="store_true")
args = parser.parse_args()

tty_out = "/dev/null"
if args.o:
    tty_out = args.o
    print("[OPTIONS] make and flash commands will output to: " + args.o)

tests_en = {'t1':True, 't2':True, 't3':True, 't4':True}
if args.test:
    sys.stdout.write("[OPTIONS] only executing test(s): ")

    for t_no in tests_en:
        tests_en[t_no] = False
    for t in args.test:
        tests_en['t'+t] = True
        sys.stdout.write(t + ", ")

    sys.stdout.write("!\n")

if args.fast:
    print("[OPTIONS] skipping compilation of all binaries and update files")

if args.prepare:
    print("[OPTIONS] only compiling binaries and update files, no tests will be done")


##### SOME FUNCTIONS #####
def signal_handler(signal, frame):
    sys.exit(0)
signal.signal(signal.SIGINT, signal_handler)


##### START OF PROGRAM #####

### COMPILING: make all necessary tools and files first
if not args.fast:
    print("\n----------------------------------------------------------------------")
    print("--------              COMPILING NECESSARY FILES               --------")
    print("----------------------------------------------------------------------")
    subprocess.call("make bootloader >" + tty_out, shell=True); print("compiling part 1 done");
    subprocess.call("FW_VERS=0x1 make ota_update_app-file-slot1 >" + tty_out, shell=True); print("compiling part 2 done");
    subprocess.call("FW_VERS=0x2 make ota_update_app-file-slot1 >" + tty_out, shell=True); print("compiling part 3 done");
    subprocess.call("FW_VERS=0x2 make ota_update_app-file-slot2 >" + tty_out, shell=True); print("compiling part 4 done");
    subprocess.call("FW_VERS=0x3 make ota_update_app-file-slot1 >" + tty_out, shell=True); print("compiling part 5 done");
    subprocess.call("FW_VERS=0x3 make ota_update_app-file-slot2 >" + tty_out, shell=True); print("compiling part 6 done");
    subprocess.call("FW_VERS=0x4 make ota_update_app-file-slot1 >" + tty_out, shell=True); print("compiling part 7 done");
    subprocess.call("FW_VERS=0x4 make ota_update_app-file-slot2 >" + tty_out, shell=True); print("compiling part 8 done");

    print("now signing the update files")
    subprocess.call("FW_VERS=0x1 make sign-update-slot1 >" + tty_out, shell=True)
    subprocess.call("FW_VERS=0x2 make sign-update-slot1 >" + tty_out, shell=True)
    subprocess.call("FW_VERS=0x2 make sign-update-slot2 >" + tty_out, shell=True)
    subprocess.call("FW_VERS=0x3 make sign-update-slot1 >" + tty_out, shell=True)
    subprocess.call("FW_VERS=0x3 make sign-update-slot2 >" + tty_out, shell=True)
    subprocess.call("FW_VERS=0x4 make sign-update-slot1 >" + tty_out, shell=True)
    subprocess.call("FW_VERS=0x4 make sign-update-slot2 >" + tty_out, shell=True)

    if tests_en['t2']:
        print("compiling and signing special file for Test 2A")
        subprocess.call("HW_ID=0xbaadf00dbaadf00d FW_VERS=0x4 make ota_update_app-file-slot1 sign-update-slot1 >" + tty_out, shell=True)



if not args.prepare:
    if tests_en['t1']:
        ### TEST 1 (normal/ standard behaviour)
        # dependencies: bootloader
        #               app_binary-*-0x1-s1 for fw_image-*-0x1-s1
        #               app_binary-*-0x2-s2 for fw_update-*-0x2-s2
        #               app_binary-*-0x3-s1 for fw_update-*-0x3-s1
        #               app_binary-*-0x4-s2 for fw_update-*-0x4-s2
        print("\n----------------------------------------------------------------------")
        print("--------                        TEST 1                        --------")
        print("--------                normal update procedure               --------")
        print("----------------------------------------------------------------------")
        if test1.do_test(tty_out) != 0:
            print(" [ERROR] test 1 failed somewhere, aborting test procedure now\n")
            sys.exit(-1)


    if tests_en['t2']:
        ### TEST 2 (keep in mind, that fw_update-*-0x4-s1 is not valid after this test!!)
        # dependencies: bootloader
        #               app_binary-*-0x2-s1 for fw_image-*-0x2-s1
        #               app_binary-*-0x3-s2 for fw_image-*-0x3-s2
        #               manipulated fw_update-*-0x4-s1
        #               app_binary-*-0x4-s1 (with wrong hw_id) for fw_update-*-0x4-s1
        #               app_binary-*-0x1-s1 for fw_update-*-0x1-s1
        print("\n----------------------------------------------------------------------")
        print("--------                        TEST 2                        --------")
        print("--------              misconfigured update files              --------")
        print("----------------------------------------------------------------------")
        ### prepare some things first
        if test2.prepare(tty_out) != 0:
            print(" [ERROR] test 2 (preparation) failed somewhere, aborting test procedure now\n")
            sys.exit(-1)

        ### Test 2a (update file with invalid file signature)
        if test2.do_part_a(tty_out) != 0:
            print(" [ERROR] test 2A failed somewhere, aborting test procedure now\n")
            test2.finish(tty_out)
            sys.exit(-1)

        ### Test 2b (update file with invalid hw_id)
        if test2.do_part_b(tty_out) != 0:
            print(" [ERROR] test 2B failed somewhere, aborting test procedure now\n")
            test2.finish(tty_out)
            sys.exit(-1)

        ### Test 2c (update file with lower fw_vers)
        if test2.do_part_c(tty_out) != 0:
            print(" [ERROR] test 2C failed somewhere, aborting test procedure now\n")
            test2.finish(tty_out)
            sys.exit(-1)

        ### tidy up
        if test2.finish(tty_out) != 0:
            print(" [ERROR] test 2 (finishing) failed somewhere, aborting test procedure now\n")
            sys.exit(-1)


    if tests_en['t3']:
        ### TEST 3 (power outage test)
        # dependencies: bootloader
        #               app_binary-*-0x1-s1 for fw_image-*-0x1-s1
        #               app_binary-*-0x2-s2 for fw_update-*-0x2-s2

        # for this test, the nucleo board must be connected first (using /dev/ttyACM0)
        # and the arduino uno must be connected second (using /dev/ttyACM1)
        # the serial interfaces used are currently hard coded!
        print("\n----------------------------------------------------------------------")
        print("--------                        TEST 3                        --------")
        print("--------              power outage during update              --------")
        print("----------------------------------------------------------------------")
        if test3.do_test(tty_out) != 0:
            print(" [ERROR] test 3 failed somewhere, aborting test procedure now\n")
            sys.exit(-1)


    if tests_en['t4']:
        ### TEST 4 (bitflip in flash)
        # dependencies: bootloader
        #               app_binary-*-0x1-s1 for fw_image-*-0x1-s1
        #               manipulated fw_image-*-0x2-s2
        #               app_binary-*-0x2-s2 for fw_update-*-0x2-s2
        print("\n----------------------------------------------------------------------")
        print("--------                        TEST 4                        --------")
        print("--------              bitflip in installed update             --------")
        print("----------------------------------------------------------------------")
        if test4.do_test(tty_out) != 0:
            print(" [ERROR] test 4 failed somewhere, aborting test procedure now\n")
            sys.exit(-1)


print("\n----------       All Requested OTA Update Tests Done        ----------\n")
