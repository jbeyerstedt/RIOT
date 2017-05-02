#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

# arduino helpers for tests.py and the test modules


import serial
import time


arduino = None


def arduino_init(port):
    global arduino

    arduino = serial.Serial(port, 115200, timeout=0.1)
    if not arduino:
        print(" [ERROR] cannot open serial connection on " + port)
        return -1
    time.sleep(2)   # wait quite long for the arduino to reset

    arduino.write("HELLO\n")
    time.sleep(0.1)
    ardu_ack = arduino.readline()[:-1] # read and discard newline
    if not ardu_ack:
        print(" [ERROR] no answer from arduino")
        return -1
    if not ardu_ack.startswith("ACK.HELLO"):
        print(" [ERROR] hello handshake with arduino not successful")
        return -1
    return 0


def arduino_send_cmd(cmd, ack):
    if arduino != None:
        arduino.write(cmd + "\n")
        time.sleep(0.1)
        ardu_ack = arduino.readline()[:-1] # read and discard newline
        if not ardu_ack:
            print(" [ERROR] no answer from arduino")
            kill_ethos(ethos)
            return -1
        if not ardu_ack.startswith(ack):
            print(" [ERROR] sending pin command to arduino not successful")
            kill_ethos(ethos)
            return -1
        return 0

    return -2
