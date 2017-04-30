#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

# ethos helpers for tests.py and the test modules


from nbstreamreader import NonBlockingStreamReader as NBSR


# send a command via ethos and search for the <compare> string in the answer
# to just read new output the <command> parameter can be omitted
# returns 0 if
def ethos_command(nbsr, ethos, compare, command = None, timeout=0.2):
    round = 0
    answer = ""

    if command is not None:
        ethos.stdin.write(command + "\n")
    while True:
        output = nbsr.readline(timeout) # give the shell some time to process
        if not output:
            break
        answer += output
        if command is not None:
            if not (output.startswith(command) or output.startswith('> ' + command)): # ignore the echoed command itself
                if compare in output:
                    return 1, answer
        else:
            if compare in output:
                return 1, answer
        round += 1
        # print output
    if round == 0:
        return -1, answer   # no output was fetched at all
    return 0, answer        # compare string was not found
