#!/usr/bin/env python3

from testUtils import Utils
from datetime import datetime
from datetime import timedelta
import time
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper
from TestHelper import AppArgs
from rodeos_utils import RodeosCluster

from enum import IntEnum

import json
import os
import re
import shutil
import signal
import sys

###############################################################
# rodeos_idle_test
# 
# This test verifies in idle state, rodeos receives (empty)
#   blocks from SHiP and the number of RocksDB SST files does
#   not grow uncontrolledly.
#
###############################################################

Print=Utils.Print

appArgs=AppArgs()
appArgs.add(flag="--num-rodeos", type=int, help="How many rodeos' should be started", default=1)
appArgs.add_bool(flag="--unix-socket", help="Run ship over unix socket")
appArgs.add_bool("--eos-vm-oc-enable", "Use OC for rodeos")

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"}, applicationSpecificArgs=appArgs)

testSuccessful=False
TestHelper.printSystemInfo("BEGIN")

with RodeosCluster(args.dump_error_details,
        args.keep_logs,
        args.leave_running,
        args.clean_run, args.unix_socket,
        'test.filter', './tests/test_filter.wasm',
        args.eos_vm_oc_enable,
        args.num_rodeos) as cluster:

    assert cluster.waitRodeosReady(), "Rodeos failed to stand up"

    timeBetweeCommandsSeconds = [2, 1, 0.5]

    class NodeT(IntEnum):
        PROD = 1
        SHIP = 2
        RODEOS = 3

    class CmdT(IntEnum):
        STOP = 1
        RESTART = 2


    # the command schedule, which consist of a list of lists tuples of 2-tuples
    # each 2-tuple is (NodeT, CmdT)
    cmdSched = []

    # for now, just mnauuly produce the schedule
    # ideally this would be  computed, such as with itertools, 
    # to get all possible permutations (ensuring a node is not attempted to be restarted)
    # until it has been stopped
    nReps = len(timeBetweeCommandsSeconds)
    cmdSched.append([(NodeT.RODEOS, CmdT.STOP, {"killsig" : 2}), (NodeT.RODEOS, CmdT.RESTART, {"clean" : False})] * nReps)
    cmdSched.append([(NodeT.SHIP, CmdT.STOP, {"killsig" : 2}), (NodeT.SHIP, CmdT.RESTART, {"clean" : False})] * nReps)
    cmdSched.append([(NodeT.PROD, CmdT.STOP, {"killsig" : 2}), (NodeT.PROD, CmdT.RESTART, {"clean" : False})]  * nReps)

    cmdSched.append([(NodeT.RODEOS, CmdT.STOP, {"killsig" : 9}), (NodeT.RODEOS, CmdT.RESTART, {"clean" : True})] * nReps)
    cmdSched.append([(NodeT.SHIP, CmdT.STOP, {"killsig" : 9}), (NodeT.SHIP, CmdT.RESTART, {"clean" : True})] * nReps)
    cmdSched.append([(NodeT.PROD, CmdT.STOP, {"killsig" : 9}), (NodeT.PROD, CmdT.RESTART, {"clean" : True})] * nReps)

    cmdSched.append([(NodeT.RODEOS, CmdT.STOP, {"killsig" : 15}), (NodeT.RODEOS, CmdT.RESTART, {"clean" : True})] * nReps)
    cmdSched.append([(NodeT.SHIP, CmdT.STOP, {"killsig" : 15}), (NodeT.SHIP, CmdT.RESTART, {"clean" : True})] * nReps)
    cmdSched.append([(NodeT.PROD, CmdT.STOP, {"killsig" : 15}), (NodeT.PROD, CmdT.RESTART, {"clean" : True})] * nReps)

    # print('cmdSched=', cmdSched)
    # 2 combinations 
    # cmdSched.append([(NodeT.PROD, CmdT.STOP, {"killsig" : 2}), (NodeT.SHIP, CmdT.STOP, {"killsig" : 2}), 
    #                  (NodeT.PROD, CmdT.RESTART, {"clean" : False}), (NodeT.SHIP, CmdT.RESTART, {"clean" : False} )])

    # cmdSched.append([(NodeT.PROD, CmdT.STOP, {"killsig" : 2}), (NodeT.RODEOS, CmdT.STOP, {"killsig" : 2}), 
    #                  (NodeT.PROD, CmdT.RESTART, {"clean" : False}), (NodeT.RODEOS, CmdT.RESTART, {"clean" : False} )])                     
    
    # cmdSched.append([(NodeT.SHIP, CmdT.STOP, {"killsig" : 2}), (NodeT.RODEOS, CmdT.STOP, {"killsig" : 2}), 
    #                  (NodeT.SHIP, CmdT.RESTART, {"clean" : False}), (NodeT.RODEOS, CmdT.RESTART, {"clean" : False} )])                                          
    
    i = 0
    for cmdList in cmdSched:
        print('cmdList=', cmdList)
        for cmd in cmdList:
            sleepTime = timeBetweeCommandsSeconds[i]
            
            print(f"Waiting {sleepTime} seconds ..")
            time.sleep(sleepTime)
            nd = cmd[0]
            nd_cmd = cmd[1]
            opts = cmd[2]

            if nd_cmd == CmdT.RESTART:
                i += 1 
                i = i % len(timeBetweeCommandsSeconds)

            if nd == NodeT.PROD:
                if nd_cmd == CmdT.STOP:
                    print("Stopping producer")
                    cluster.stopProducer(opts['killsig'])
                else:
                    print("Restarting producer")
                    cluster.restartProducer(clean=opts['clean'])
            elif nd == NodeT.SHIP:
                if nd_cmd == CmdT.STOP:
                    print("Stopping SHIP")
                    cluster.stopShip(opts['killsig'])
                else:
                    print("Restarting SHIP")
                    cluster.restartShip(clean=opts['clean'])
            elif nd == NodeT.RODEOS:
                if nd_cmd == CmdT.STOP:
                    print("Stopping rodeos")
                    cluster.stopRodeos(opts['killsig'])
                else:
                    print("Restarting rodeos")
                    cluster.restartRodeos(clean=opts['clean'])

    # Big enough to have new blocks produced
    numBlocks=120
    assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks".format(numBlocks)
    assert cluster.allBlocksReceived(numBlocks), "Rodeos did not receive {} blocks".format(numBlocks)

    testSuccessful=True
    cluster.setTestSuccessful(testSuccessful)
    
errorCode = 0 if testSuccessful else 1
exit(errorCode)
