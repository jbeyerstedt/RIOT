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
from test_ethos_helpers import ethos_command

nbsr = None
ethos = None


def kill_ethos(ethos):
    # kill the ethos process properly
    os.killpg(os.getpgid(ethos.pid), signal.SIGTERM)


### call first to prepare and setup things
def prepare(tty_out):
    global nbsr
    global ethos

    print("(Step 0) flashing with test firmware")
    subprocess.call("FW_VERS=0x2 FW_VERS_2=0x3 make merge-test-hex >" + tty_out, shell=True)

    ## flash the devive with factory-hex
    if subprocess.call("FW_VERS=0x1 make flash-test >" + tty_out, shell=True):
        return -1
    time.sleep(1)

    ## start ethos console
    ethos = subprocess.Popen("make ethos 2>/dev/null", stdout=subprocess.PIPE, stdin=subprocess.PIPE, shell=True, preexec_fn=os.setsid)
    time.sleep(1)
    nbsr = NBSR(ethos.stdout)

    # get first diagnostic lines from ethos console
    ret_val = ethos_command(nbsr, ethos, "/dist/tools/ethos")
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] ethos not properly started")
        kill_ethos(ethos)
        return -1
    ret_val, answer = ethos_command(nbsr, ethos, "command not found", command="h")
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] ethos shell does not answer correctly")
        print(answer)
        kill_ethos(ethos)
        return -1

    print("    [OK] both slots populated, ethos console started\n")
    return 0



### Test 2a (update file with invalid file signature)
def do_part_a(tty_out):
    global nbsr
    global ethos

    subprocess.call("cp fw_update-0xabc0123456789def-0x4-s1.bin fw_update-orig-0x4-s1", shell=True)

    print("(Part A) testing FW update file signature validation")
    # manipulate some bits of the vers 4, slot 1 file
    subprocess.call("cat fw_update-orig-0x4-s1 | head -c -16 >fw_update-0xabc0123456789def-0x4-s1.bin", shell=True)

    if subprocess.call("FW_VERS=0x4 make flash-updatefile-slot1 >" + tty_out, shell=True):
        kill_ethos(ethos)
        return -1
    time.sleep(1)

    ## check running FW version
    ret_val, answer = ethos_command(nbsr, ethos, "FW version 3, slot 2", command="fw_info")
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] wrong firmware version or slot started")
        print("dumping fetched answer from device:\n" + answer)
        kill_ethos(ethos)
        return -1
    print("    [OK] correct inital FW running")

    ## start update
    ret_val, answer = ethos_command(nbsr, ethos, "[ota_file] INFO incorrect decrypted hash", command="ota_install", timeout=5)
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] detection of invalid signature not successful")
        print("dumping fetched answer from device:\n\n" + answer)
        kill_ethos(ethos)
        return -1

    print(" ==>[OK] broken file signature successfully detected\n")

    # tidy up
    subprocess.call("rm fw_update-0xabc0123456789def-0x4-s1.bin", shell=True)
    subprocess.call("mv fw_update-orig-0x4-s1 fw_update-0xabc0123456789def-0x4-s1.bin", shell=True)

    return 0



### Test 2b (update file with invalid hw_id)
def do_part_b(tty_out):
    global nbsr
    global ethos

    print("(Part B) testing hardware ID validation")

    if subprocess.call("HW_ID=0xbaadf00dbaadf00d FW_VERS=0x4 make flash-updatefile-slot1 >" + tty_out, shell=True):
        kill_ethos(ethos)
        return -1
    time.sleep(1)

    ## check running FW version
    ret_val, answer = ethos_command(nbsr, ethos, "FW version 3, slot 2", command="fw_info")
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] TODO")
        print("dumping fetched answer from device:\n" + answer)
        kill_ethos(ethos)
        return -1
    print("    [OK] correct inital FW running")

    ## start update
    ret_val, answer = ethos_command(nbsr, ethos, "[ota_updater] ERROR update file is invalid", command="ota_install", timeout=5)
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] detection of invalid HW_ID not successful")
        print("dumping fetched answer from device:\n\n" + answer)
        kill_ethos(ethos)
        return -1

    print(" ==>[OK] file with wrong hardware id successfully detected\n")
    return 0



### Test 2c (update file with lower fw_vers)
def do_part_c(tty_out):
    global nbsr
    global ethos

    print("(Part C) testing FW update file signature validation")
    if subprocess.call("FW_VERS=0x1 make flash-updatefile-slot1 >" + tty_out, shell=True):
        kill_ethos(ethos)
        return -1
    time.sleep(1)

    ## check running FW version
    ret_val, answer = ethos_command(nbsr, ethos, "FW version 3, slot 2", command="fw_info")
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] TODO")
        print("dumping fetched answer from device:\n" + answer)
        kill_ethos(ethos)
        return -1
    print("    [OK] correct inital FW running")

    ## start update
    ret_val, answer = ethos_command(nbsr, ethos, "[ota_updater] ERROR update file is invalid", command="ota_install", timeout=5)
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] detection of downgrade attempt not successful")
        print("dumping fetched answer from device:\n\n" + answer)
        kill_ethos(ethos)
        return -1

    print(" ==>[OK] file with lower FW version successfully detected\n")
    return 0



### call last to tidy up afterwards
def finish(tty_out):
    global nbsr
    global ethos

    kill_ethos(ethos)
    print("(Finish) tidying up done")
    return 0
