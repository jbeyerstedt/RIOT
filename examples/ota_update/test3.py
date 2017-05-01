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



def kill_ethos(ethos):
    # kill the ethos process properly
    os.killpg(os.getpgid(ethos.pid), signal.SIGTERM)


def do_test(tty_out):
    ## prepare everything for this test
    subprocess.call("FW_VERS=0x1 make merge-factory-hex >" + tty_out, shell=True)

    print("(Step 1) initial flashing with factory firmware")
    ## flash the devive with factory-hex
    if subprocess.call("FW_VERS=0x1 make flash-factory >" + tty_out, shell=True):
        return -1
    time.sleep(1)

    ## start ethos console
    ethos = subprocess.Popen("make ethos 2>/dev/null", stdout=subprocess.PIPE, stdin=subprocess.PIPE, shell=True, preexec_fn=os.setsid)
    time.sleep(1)
    # wrap p.stdout with a NonBlockingStreamReader object:
    nbsr = NBSR(ethos.stdout)

    # get first diagnostic lines from ethos console
    ret_val = ethos_command(nbsr, ethos, "/dist/tols/ethos")
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

    ## start serial communication with arduino
    # TODO: open screen session or better use some python terminal


    ## check running FW version
    ret_val, answer = ethos_command(nbsr, ethos, "FW version 1, slot 1", command="fw_info")
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

    print(" ==>[OK] test firmware successfully installed and started\n")


    print("(Step 2) write update file (fw_vers 2, slot 2) to device and initiate update")
    ## flash the update file
    if subprocess.call("FW_VERS=0x2 make flash-updatefile-slot2 >" + tty_out, shell=True):
        return -1
    time.sleep(1)

    ## check running FW version again (after flashing update file)
    ret_val, answer = ethos_command(nbsr, ethos, "FW version 1, slot 1", command="fw_info")
    if ret_val < 0:
        print(" [ERROR] no answer from ethos")
        kill_ethos(ethos)
        return -1
    elif ret_val == 0:
        print(" [ERROR] wrong firmware version or slot started")
        print("dumping fetched answer from device:\n" + answer)
        kill_ethos(ethos)
        return -1
    print("    [OK] correct FW running after flashing new update file")

    ## start update
    # TODO: send ota_install command and wait for [ota_file] INFO writing update file

    # TODO: send power off command to arduino

    # TODO: observer bootloader messages?

    # TODO: start another update via ethos "ota_install" and check normal behaviour
