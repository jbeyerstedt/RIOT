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


def do_test(tty_out):
    ## prepare everything for this test
    # TODO: manipulate slot 2 in the test-hex, to have an invalid signature
    subprocess.call("FW_VERS=0x2 FW_VERS_2=0x3 make merge-test-hex >" + tty_out, shell=True)

    print("(Step 1) initial flashing with factory firmware")
    ## flash the devive with factory-hex
    if subprocess.call("FW_VERS=0x2 FW_VERS_2=0x3 make flash-test >" + tty_out, shell=True):
        return -1
    time.sleep(1)

    ## start serial communication with bootloader
    # TODO: open screen session or better use some python terminal

    print(" ==>[OK] test firmware successfully installed\n")


    print("(Step 2) checking bootloadr messages")
    # TODO: do

    print(" ==>[OK] bootloader behaviour for an invalid slot successfully validated\n")
