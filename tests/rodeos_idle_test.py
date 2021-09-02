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

    timeBetweeCommandsSeconds = 3

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
    cmdSched.append([(NodeT.RODEOS, CmdT.STOP), (NodeT.RODEOS, CmdT.RESTART)])
    cmdSched.append([(NodeT.SHIP, CmdT.STOP), (NodeT.SHIP, CmdT.RESTART)])
    cmdSched.append([(NodeT.PROD, CmdT.STOP), (NodeT.PROD, CmdT.RESTART)])

    # 2 combinations 
    cmdSched.append([(NodeT.PROD, CmdT.STOP), (NodeT.SHIP, CmdT.STOP), 
                     (NodeT.PROD, CmdT.RESTART), (NodeT.SHIP, CmdT.RESTART )])

    cmdSched.append([(NodeT.PROD, CmdT.STOP), (NodeT.RODEOS, CmdT.STOP), 
                     (NodeT.PROD, CmdT.RESTART), (NodeT.RODEOS, CmdT.RESTART )])                     
    
    cmdSched.append([(NodeT.SHIP, CmdT.STOP), (NodeT.RODEOS, CmdT.STOP), 
                     (NodeT.SHIP, CmdT.RESTART), (NodeT.RODEOS, CmdT.RESTART )])                                          
    
    for cmdList in cmdSched:
        for cmd in cmdList:
            print(f"Waiting {timeBetweeCommandsSeconds} seconds ..")
            time.sleep(timeBetweeCommandsSeconds)

            if cmd[0] == NodeT.PROD:
                if cmd[1] == CmdT.STOP:
                    print("Stopping producer")
                    cluster.stopProducer(9)
                else:
                    print("Restarting producer")
                    cluster.restartProducer(clean=False)
            elif cmd[0] == NodeT.SHIP:
                if cmd[1] == CmdT.STOP:
                    print("Stopping SHIP")
                    cluster.stopShip(9)
                else:
                    print("Restarting SHIP")
                    cluster.restartShip(clean=False)
            elif cmd[0] == NodeT.RODEOS:
                if cmd[1] == CmdT.STOP:
                    print("Stopping rodeos")
                    cluster.stopRodeos(9)
                else:
                    print("Restarting rodeos")
                    cluster.restartRodeos(clean=False)

    # Big enough to have new blocks produced
    numBlocks=120
    assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks".format(numBlocks)
    assert cluster.allBlocksReceived(numBlocks), "Rodeos did not receive {} blocks".format(numBlocks)

    testSuccessful=True
    cluster.setTestSuccessful(testSuccessful)
    
errorCode = 0 if testSuccessful else 1
exit(errorCode)
