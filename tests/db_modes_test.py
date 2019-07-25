#!/usr/bin/env python3

# This test is intended to verify that switching between DB modes "just works". Addtionally
# it tries to make sure the dirty bit behaves as expected even in heap mode.

from testUtils import Utils

import decimal
import re
import os
import signal
import time
import subprocess
import shlex
import tempfile
import socket
import sys

errorExit=Utils.errorExit

nodeosCmd="./programs/nodeos/nodeos -d {} --config-dir {} \
--chain-state-db-size-mb 8 --chain-state-db-guard-size-mb 0 --reversible-blocks-db-size-mb 1 \
--reversible-blocks-db-guard-size-mb 0 -e -peosio --plugin eosio::chain_api_plugin"

cleosPath="./programs/cleos/cleos"
port=8888

def run_and_kill(extraCmd="", killsig=signal.SIGTERM, preCmd=""):
    global port
    port = 8888
    print("looking for free ports")
    tries = 200
    while (tries > 0):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_address = ('127.0.0.1', port)
            sock.bind(server_address)
            sock.close()
            time.sleep(1)
            tries = 0
        except:
            tries = tries - 1
            port = port + 1
    # now start nodeos
    time.sleep(5)
    cmdArr= preCmd + nodeosCmd + " --http-server-address=127.0.0.1:" + str(port) + extraCmd
    print("port %d is free now, start to run nodeosCmd with extra parameter \"%s\"" % (port, extraCmd))
    proc=subprocess.Popen(cmdArr, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, preexec_fn=os.setsid)
    tries = 30
    ok = False
    getInfoCmd = cleosPath + " -u http://127.0.0.1:" + str(port) + "/ get info"
    while (tries > 0):
        time.sleep(1)
        reply = Utils.checkOutput(getInfoCmd, ignoreError=True)
        if reply.find("server_version") != -1:
            ok = True
            time.sleep(1)
            break
        tries = tries - 1
    if ok is True:
        os.killpg(os.getpgid(proc.pid), killsig)
    print("got result %d, killed with signal %d" % (ok, killsig))
    return ok

print("=== db_modes_test.py starts ===")

tempdir = tempfile.mkdtemp()
nodeosCmd = nodeosCmd.format(tempdir, tempdir)

print("nodeosCmd is %s" % (nodeosCmd))

#new chain with mapped mode
if run_and_kill(" --delete-all-blocks") != True:
    errorExit("Failed in new chain with mapped mode")

#use previous DB with heap mode
if run_and_kill(" --database-map-mode heap") != True:
    errorExit("Failed in restart heap mode")

#test locked mode if max locked memory is unlimited
ulimits = Utils.checkOutput('ulimit -a', ignoreError=True).split("\n")
for row in ulimits:
    if row.find("max locked memory") != -1:
        print("output of ulimit -a is: %s:" % (row))
        if row.find("unlimited") != -1:
            if run_and_kill(" --database-map-mode locked") != True:
                errorExit("Failed in restart with locked mode")
        else:
            print("skip default locked mode test because max locked memory is not unlimited")
        break

#locked mode should fail when it's not possible to lock anything
if run_and_kill(" --database-map-mode locked", preCmd="ulimit -l 0; ") != False:
    errorExit("Test Failed in locked mode with ulimit -l 0")

#But shouldn't result in the dirty flag staying set; so next launch should run
if run_and_kill("") != True:
    errorExit("Failed in restart heap mode")

#Try killing with hard KILL
if run_and_kill("", killsig=signal.SIGKILL) != True:
    errorExit("Failed in restart & hardkill in heap mode")

#should be dirty now
if run_and_kill("") != False:
    errorExit("dirty flag test failed")

#should also still be dirty in heap mode
if run_and_kill(" --database-map-mode heap") != False:
    errorExit("dirty flag test failed")

#start over again! but this time start with heap mode
if run_and_kill(" --delete-all-blocks --database-map-mode heap") != True:
    errorExit("Failed in restart heap mode with --delete-all-blocks")

#Then switch back to mapped
if run_and_kill("") != True:
    errorExit("Failed in restart mapped mode")

#try killing it while in heap mode
if run_and_kill(" --database-map-mode heap", killsig=signal.SIGKILL) != True:
    errorExit("Failed in restart heap mode and hardkill")

#should be dirty if we run in either mode node
if run_and_kill(" --database-map-mode heap") != False:
    errorExit("dirty flag test failed in heap mode")

if run_and_kill("") != False:
    errorExit("dirty flag test failed in mapped mode")

print("=== db_modes_test.py finished successfully ===")

