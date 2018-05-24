#!/usr/bin/env python3

# pylint: disable=unused-import
import copy
import decimal
import subprocess
import time
import glob
import shutil
import os
import platform
from collections import namedtuple
import re
import string
import signal
import datetime
import inspect
import sys
import random
import json
import shlex
from sys import stdout

EosClientPath="programs/cleos/cleos"
EosServerName="nodeos"

EosLauncherPath="programs/eosio-launcher/eosio-launcher"

def waitForObj(lam, timeout=None):
    if timeout is None:
        timeout=60

    endTime=time.time()+timeout

    while endTime > time.time():
        ret=lam()
        if ret is not None:
            return ret
        sleepTime=3
        print("cmd: sleep %d seconds, remaining time: %d seconds" %
                    (sleepTime, endTime - time.time()))
        time.sleep(sleepTime)

    return None

def waitForBool(lam, timeout=None):
    myLam = lambda: True if lam() else None
    ret=waitForObj(myLam, timeout)
    return False if ret is None else ret

########################################################################
cmd='programs/eosio-launcher/eosio-launcher -p 1 -n 1 -s mesh -d 1 -f'
print("cmd: %s" % (cmd))
cmdArr=cmd.split()
if 0 != subprocess.call(cmdArr):
    print("ERROR: Launcher failed to launch.")
    exit(1)

time.sleep(1)

cmd="pgrep -a nodeos"
sOut=None
print("cmd: %s" % (cmd))
psOut=subprocess.check_output(cmd.split()).decode("utf-8")

pattern=r"[\n]?(\d+) (.* --data-dir var/lib/node_00)"
m=re.search(pattern, psOut, re.MULTILINE)
if m is None:
    print("ERROR: Failed to find %s pid. Pattern %s" % (EosServerName, pattern))
    exit(1)

pid=int(m.group(1))

print("Killing nodeos process. Pid: %d" % (pid))

os.kill(pid, signal.SIGKILL)
#os.kill(pid, signal.SIGHUP)

def myFunc():
    try:
        os.kill(pid, 0) #check if process with pid is running
    except OSError as _:
        return True
    return False

if not waitForBool(myFunc):
    print("ERROR: Failed to validate node shutdown.")
    exit(1)

print("Nodeos shutdown successfully.")

cmd="pkill -9 nodeos"
print("cmd: %s" % (cmd))
subprocess.call(cmd.split())
exit(0)
