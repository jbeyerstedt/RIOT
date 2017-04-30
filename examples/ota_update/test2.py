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


### call first to prepare and setup things
def prepare(tty_out):
    subprocess.call("FW_VERS=0x2 FW_VERS_2=0x3 make merge-test-hex >" + tty_out, shell=True)
    # TODO: open ethos interface
    return 0



### Test 2a (update file with invalid file signature)
def do_part_a(tty_out):
    # TODO: manipulate some bits of the vers 4, slot 1 file

    return 0



### Test 2b (update file with invalid hw_id)
def do_part_b(tty_out):
    subprocess.call("HW_ID=0xbaadf00dbaadf00d FW_VERS=0x4 make ota_update_app-file-slot1 sign-update-slot1 >" + tty_out, shell=True)
    # if subprocess.call("HW_ID=0xbaadf00dbaadf00d FW_VERS=0x4 make flash-updatefile-slot1 >" + tty_out, shell=True):
    #     return -1

    return 0



### Test 2c (update file with lower fw_vers)
def do_part_c(tty_out):
    # if subprocess.call("FW_VERS=0x1 make flash-updatefile-slot1 >" + tty_out, shell=True):
    #     return -1

    return 0



### call last to tidy up afterwards
def finish(tty_out):
    kill_ethos
    # TODO: delete the "wrong" files
    return 0
