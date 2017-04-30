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
def prepare():
    subprocess.call("FW_VERS=0x2 FW_VERS_2=0x3 make merge-test-hex >/dev/null", shell=True)

    # TODO: open ethos interface



### Test 2a (update file with invalid file signature)
def do_part_a():
    # TODO: manipulate some bits of the vers 4, slot 1 file
    print("todo")



### Test 2b (update file with invalid hw_id)
def do_part_b():
    subprocess.call("HW_ID=0xbaadf00dbaadf00d FW_VERS=0x4 make ota_update_app-file-slot1 sign-update-slot1 >/dev/null", shell=True)
    #subprocess.call("HW_ID=0xbaadf00dbaadf00d FW_VERS=0x4 make flash-updatefile-slot1 >/dev/null", shell=True)



### Test 2c (update file with lower fw_vers)
def do_part_c():
    #subprocess.call("FW_VERS=0x1 make flash-updatefile-slot1 >/dev/null", shell=True)
    print("todo")



### call last to tidy up afterwards
def finish():
    kill_ethos
    # TODO: delete the "wrong" files
