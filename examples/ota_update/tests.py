#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

# for python 2.6 and later. python 3 not tested

import subprocess, sys, signal
import time
import os
import signal

from nbstreamreader import NonBlockingStreamReader as NBSR


##### SOME FUNCTIONS #####
def signal_handler(signal, frame):
    os.killpg(os.getpgid(ethos.pid), signal.SIGTERM)
    sys.exit(0)
signal.signal(signal.SIGINT, signal_handler)

# send a command via ethos and search for the <compare> string in the answer
# to just read new output the <command> parameter can be omitted
# returns 0 if
def ethos_command(compare, command = None, timeout=0.2):
    round = 0
    answer = ""

    if command is not None:
        ethos.stdin.write(command + "\n")
    while True:
        output = nbsr.readline(timeout) # give the shell some time to process
        if not output:
            if command is not None:
                print("the compare value was not found for command: " + command)
            else:
                print("the compare value was not found")
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


def kill_ethos_and_exit():
    # kill the ethos process properly
    os.killpg(os.getpgid(ethos.pid), signal.SIGTERM)
    sys.exit(-1);



##### START OF PROGRAM #####
print("Welcome to the ota update tests")

### COMPILING: make all necessary tools and files first
# TODO: have an option to jump over this part (or simply use a seperate script)
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
print("------------------------------------")
## prepare everything for this test
subprocess.call("FW_VERS=0x1 make merge-factory-hex >/dev/null", shell=True)

print("(Step 1) initial flashing with factory firmware")
## flash the devive with factory-hex
subprocess.call("FW_VERS=0x1 make flash-factory >/dev/null", shell=True)
time.sleep(1)

## start ethos console
ethos = subprocess.Popen("make ethos 2>/dev/null", stdout=subprocess.PIPE, stdin=subprocess.PIPE, shell=True, preexec_fn=os.setsid)
time.sleep(1)
# wrap p.stdout with a NonBlockingStreamReader object:
nbsr = NBSR(ethos.stdout)

# get first diagnostic lines from ethos console
ret_val = ethos_command("/dist/tools/ethos")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] ethos not properly started")
    kill_ethos_and_exit()
ret_val, answer = ethos_command("command not found", command="h")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] ethos shell does not answer correctly")
    print(answer)
    kill_ethos_and_exit()

## check running FW version
ret_val, answer = ethos_command("FW version 1, slot 1", command="fw_info")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] wrong firmware version or slot started")
    print("dump fetched answer from device:\n" + answer)
    kill_ethos_and_exit()
print("    [OK] correct inital FW running")

print(" ==>[OK] factory firmware successfully installed and started\n")


print("(Step 2) write update file (fw_vers 2, slot 2) to device and initiate update")
## flash the update file
subprocess.call("FW_VERS=0x2 make flash-updatefile-slot2 >/dev/null", shell=True)
time.sleep(1)

## check running FW version again (after flashing update file)
ret_val, answer = ethos_command("FW version 1, slot 1", command="fw_info")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] wrong firmware version or slot started")
    print("dump fetched answer from device:\n" + answer)
    kill_ethos_and_exit()
print("    [OK] correct FW running after flashing new update file")

## start update
ret_val, answer = ethos_command("ota_updater_install returned 0", command="ota_install", timeout=5)
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] installation of FW update not successful")
    print("dump fetched answer from device:\n\n" + answer)
    kill_ethos_and_exit()
print("    [OK] FW update installation successful")

## reboot
ret_val, answer = ethos_command("[ota_updater] INFO rebooting", command="ota_reboot")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] something bad happend before rebooting")
    print("dump fetched answer from device:\n\n" + answer)
    kill_ethos_and_exit()
print("    [OK] reboot initiated")

time.sleep(4)   # wait for reboot

## check, if the new version is running now
ret_val, answer = ethos_command("FW version 2, slot 2", command="fw_info")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] wrong firmware version or slot started")
    print("dump fetched answer from device:\n" + answer)
    kill_ethos_and_exit()
print("    [OK] correct FW running after flashing new update file")

print(" ==>[OK] firmware update 1 (fw_vers 2, slot 2) was successful\n")


print("(Step 3) write update file (fw_vers 3, slot 1) to device and initiate update")
subprocess.call("FW_VERS=0x3 make flash-updatefile-slot1 >/dev/null", shell=True)
time.sleep(1)

## check running FW version again (after flashing update file)
ret_val, answer = ethos_command("FW version 2, slot 2", command="fw_info")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] wrong firmware version or slot started")
    print("dump fetched answer from device:\n" + answer)
    kill_ethos_and_exit()
print("    [OK] correct FW running after flashing new update file")

## start update
ret_val, answer = ethos_command("ota_updater_install returned 0", command="ota_install", timeout=5)
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] installation of FW update not successful")
    print("dump fetched answer from device:\n\n" + answer)
    kill_ethos_and_exit()
print("    [OK] FW update installation successful")

## reboot
ret_val, answer = ethos_command("[ota_updater] INFO rebooting", command="ota_reboot")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] something bad happend before rebooting")
    print("dump fetched answer from device:\n\n" + answer)
    kill_ethos_and_exit()
print("    [OK] reboot initiated")

time.sleep(4)   # wait for reboot

## check, if the new version is running now
ret_val, answer = ethos_command("FW version 3, slot 1", command="fw_info")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] wrong firmware version or slot started")
    print("dump fetched answer from device:\n" + answer)
    kill_ethos_and_exit()
print("    [OK] correct FW running after flashing new update file")

print(" ==>[OK] firmware update 2 (fw_vers 3, slot 1) was successful\n")


print("(Step 4) write update file (fw_vers 4, slot 2) to device and initiate update")
subprocess.call("FW_VERS=0x4 make flash-updatefile-slot2 >/dev/null", shell=True)
time.sleep(1)

## check running FW version again (after flashing update file)
ret_val, answer = ethos_command("FW version 3, slot 1", command="fw_info")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] wrong firmware version or slot started")
    print("dump fetched answer from device:\n" + answer)
    kill_ethos_and_exit()
print("    [OK] correct FW running after flashing new update file")

## start update
ret_val, answer = ethos_command("ota_updater_install returned 0", command="ota_install", timeout=5)
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] installation of FW update not successful")
    print("dump fetched answer from device:\n\n" + answer)
    kill_ethos_and_exit()
print("    [OK] FW update installation successful")

## reboot
ret_val, answer = ethos_command("[ota_updater] INFO rebooting", command="ota_reboot")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] something bad happend before rebooting")
    print("dump fetched answer from device:\n\n" + answer)
    kill_ethos_and_exit()
print("    [OK] reboot initiated")

time.sleep(4)   # wait for reboot

## check, if the new version is running now
ret_val, answer = ethos_command("FW version 4, slot 2", command="fw_info")
if ret_val < 0:
    print(" [ERROR] no answer from ethos")
    kill_ethos_and_exit()
elif ret_val == 0:
    print(" [ERROR] wrong firmware version or slot started")
    print("dump fetched answer from device:\n" + answer)
    kill_ethos_and_exit()
print("    [OK] correct FW running after flashing new update file")

print(" ==>[OK] firmware update 3 (fw_vers 4, slot 2) was successful\n")


kill_ethos_and_exit()

## TEST 2 (keep in mind, that fw_update-*-0x4-s1 is not valid after this test!!)
# dependencies: bootloader
#               app_binary-*-0x2-s1 for fw_image-*-0x2-s1
#               app_binary-*-0x3-s2 for fw_image-*-0x3-s2
#               manipulated fw_update-*-0x4-s1
#               app_binary-*-0x4-s1 (with wrong hw_id) for fw_update-*-0x4-s1
#               app_binary-*-0x1-s1 for fw_update-*-0x1-s1
print("\n------------------------------------")
print("---            TEST 2            ---")
print("------------------------------------")
subprocess.call("FW_VERS=0x2 FW_VERS_2=0x3 make merge-test-hex >/dev/null", shell=True)

### Test 2a (update file with invalid file signature)
# TODO: manipulate some bits of the vers 4, slot 1 file


### Test 2b (update file with invalid hw_id)
subprocess.call("HW_ID=0xbaadf00dbaadf00d FW_VERS=0x4 make ota_update_app-file-slot1 sign-update-slot1 >/dev/null", shell=True)
#subprocess.call("HW_ID=0xbaadf00dbaadf00d FW_VERS=0x4 make flash-updatefile-slot1 >/dev/null", shell=True)


### Test 2c (update file with lower fw_vers)
#subprocess.call("FW_VERS=0x1 make flash-updatefile-slot1 >/dev/null", shell=True)



## TEST 3 (power outage test)
# dependencies: bootloader
#               app_binary-*-0x1-s1 for fw_image-*-0x1-s1
#               app_binary-*-0x2-s2 for fw_update-*-0x2-s2
print("\n------------------------------------")
print("---            TEST 3            ---")
print("------------------------------------")
subprocess.call("FW_VERS=0x1 make merge-factory-hex >/dev/null", shell=True)

# TODO: everything



## TEST 4 (bitflip in flash)
# dependencies: bootloader
#               app_binary-*-0x1-s1 for fw_image-*-0x1-s1
#               manipulated fw_image-*-0x2-s2
#               app_binary-*-0x2-s2 for fw_update-*-0x2-s2
print("\n------------------------------------")
print("---            TEST 4            ---")
print("------------------------------------")
subprocess.call("FW_VERS=0x2 FW_VERS_2=0x3 make merge-test-hex >/dev/null", shell=True)

# TODO: everything
