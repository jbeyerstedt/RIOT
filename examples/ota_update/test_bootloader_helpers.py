#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

# bootloadr communication helpers for tests.py and the test modules


import serial
import time


bootloader = None


def bootloader_init(port):
    global bootloader

    bootloader = serial.Serial(port, 115200, dsrdtr=0, rtscts=0, timeout=0.1)
    if not bootloader:
        print(" [ERROR] cannot open serial connection on " + port)
        return -1
    time.sleep(1)


def bootloader_get_cmd(compare, timeout=1):
    count = 0
    cnt_max = timeout * 100     # because sleep time is 0.01
    answer = ""

    if bootloader != None:
        while True:
            bldr_msg = bootloader.readline()
            if bldr_msg:
                answer += bldr_msg
                if compare in bldr_msg:
                    return 1, answer

            count += 1
            if count > cnt_max:
                break
            time.sleep(0.01)

        # only reached, if timeout (compare not found)
        if not answer:
            return -1, answer   # no output was fetched at all
        return 0, answer        # compare string was not found

    return -2
