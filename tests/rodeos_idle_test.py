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

ndHelp = "Order to restart nodes. Should be 'ship', 'prod', 'rodeos' "
ndHelp += "Prefixed by a '-' (for stop) or '+' (for restart) "
ndHelp += "e.g '_ship,_rodeos,+ship,+rodeos "
ndHelp += "will stop ship, stop rodeos, then restart ship, restart rodeos,"
appArgs.add("--node-order", type=str, help=ndHelp, default="_ship,+ship")
appArgs.add("--stop-wait", type=int, help="Wait time after stop is issued", default=1)
appArgs.add("--restart-wait", type=int, help="Wait time after restart is issued", default=1)
appArgs.add("--reps", type=int, help="How many times to run test", default=5)

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"}, applicationSpecificArgs=appArgs)

nodeOrderStr = args.node_order
nodeOrder = re.split(",", nodeOrderStr)
stopWait = args.stop_wait
restartWait = args.restart_wait
nReps = args.reps

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

    class NodeT(IntEnum):
        PROD = 1
        SHIP = 2
        RODEOS = 3
  
    class CmdT(IntEnum):
        STOP = 1
        RESTART = 2

    NodeTstrMap = {"prod" : NodeT.PROD, "ship" : NodeT.SHIP, "rodeos" : NodeT.RODEOS }

    # the command schedule, which consist of a list of lists tuples of 2-tuples
    # each 2-tuple is (NodeT, CmdT)
    cmdSched = []

    lastKillSig = -1
    for killSig in [2, 15, 9]:
       cmdList = []
       for nd in nodeOrder:
           opts = dict()
           ndStr = nd[1:]
           if ndStr not in NodeTstrMap.keys():
               Utils.errorExit(f"unkknown node type {ndStr} when parsing --node-order '{nodeOrderStr}'")

           ndT = NodeTstrMap[ndStr]
           sym = nd[0]
           if sym == "_":
               cmdT = CmdT.STOP
               opts["killsig"] = killSig
               lastKillSig = killSig
           elif sym == "+":
               cmdT = CmdT.RESTART
               opts['clean'] = (lastKillSig == 9)
           else:
               Utils.errorExit(f"'+' or '_' expected got '{sym} when parsing --node-order '{nodeOrderStr}'")

           cmdList.append((ndT, cmdT, opts))
           
       cmdSched.append(cmdList * nReps)
   
    for cmdList in cmdSched:
        print('cmdList=', cmdList)
        for cmd in cmdList:
            
            nd = cmd[0]
            nd_cmd = cmd[1]
            opts = cmd[2]
            if nd_cmd == CmdT.STOP:
                s = f"killsig= {opts['killsig']}"
            else:
                s = f"clean= {opts['clean']}"

            if nd == NodeT.PROD:
                if nd_cmd == CmdT.STOP:
                    print(f"Stopping producer {s}")
                    cluster.stopProducer(opts['killsig'])
                else:
                    print(f"Restarting producer {s}")
                    cluster.restartProducer(clean=opts['clean'])
            elif nd == NodeT.SHIP:
                if nd_cmd == CmdT.STOP:
                    print(f"Stopping SHIP {s}")
                    cluster.stopShip(opts['killsig'])
                else:
                    print(f"Restarting SHIP {s}")
                    cluster.restartShip(clean=opts['clean'])
            elif nd == NodeT.RODEOS:
                if nd_cmd == CmdT.STOP:
                    print(f"Stopping rodeos {s}")
                    cluster.stopRodeos(opts['killsig'])
                else:
                    print(f"Restarting rodeos {s}")
                    cluster.restartRodeos(clean=opts['clean'])
                        
            sleepTime = stopWait
            if nd_cmd == CmdT.RESTART:
                sleepTime = restartWait
                
            print(f"Waiting {sleepTime} seconds ..")
            time.sleep(sleepTime)         

    # Big enough to have new blocks produced
    numBlocks=120
    assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks".format(numBlocks)
    assert cluster.allBlocksReceived(numBlocks, timeoutSeconds=80), "Rodeos did not receive {} blocks".format(numBlocks)

    testSuccessful=True
    cluster.setTestSuccessful(testSuccessful)
    
errorCode = 0 if testSuccessful else 1
exit(errorCode)
