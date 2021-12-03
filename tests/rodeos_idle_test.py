#!/usr/bin/env python3

from testUtils import Utils
import time
from TestHelper import TestHelper
from TestHelper import AppArgs
from rodeos_utils import RodeosCluster

from enum import IntEnum

import re

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

ndHelp = "Order to restart nodes. Should be 'ship', 'prod', 'rodeos'\n"
ndHelp += "Prefixed by a '_' (for stop) or '+' (for restart)\n"
ndHelp += "Suffixed by '-k<#>' where '<#>' kill signal (e.g, 2, 9, 15) for stop and '-c' or '-n' for clean or normal restart.\n"
ndHelp += "Example: '_ship-k2,_rodeos-k9,+ship-n,+rodeos-c\n "
ndHelp += "will stop ship (kill signal 2), stop rodeos (kill signal 9), then restart ship (normal), restart rodeos (clean)"
appArgs.add("--node-order", type=str, help=ndHelp, default="_ship-k2,+ship-n")
appArgs.add("--stop-wait", type=int, help="Wait time after stop is issued", default=1)
appArgs.add("--restart-wait", type=int, help="Wait time after restart is issued", default=1)
appArgs.add("--reps", type=int, help="How many times to run test", default=3)

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run", "--signing-delay"}, applicationSpecificArgs=appArgs)

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
        args.num_rodeos,
        producerExtraArgs="--signing-delay {}".format(args.signing_delay)) as cluster:

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

    lastKillSig = { NodeT.PROD: -1, NodeT.SHIP: -1, NodeT.RODEOS: -1 }
    
    cmdList = []
    for cmdStr in nodeOrder:
        opts = dict()
        r = re.split('-', cmdStr)
        if len(r) != 2:
            Utils.errorExit("Bad command " + cmdStr + " when parsing --node-order '" + nodeOrderStr + "'.  The character '-' must occur exactly once")
        ndCmd = r[0]
        suffix = r[1]

        ndStr = ndCmd[1:]
        if ndStr not in NodeTstrMap.keys():
            Utils.errorExit("Inknown node type " + ndStr + " when parsing --node-order '" + nodeOrderStr + "'")

        ndT = NodeTstrMap[ndStr]
        sym = ndCmd[0]
        if sym == "_":
            cmdT = CmdT.STOP
            if not suffix[0] == 'k':
                Utils.errorExit("On STOP command, expected 'k<#>', got '" + suffix +  "' when parsing '" + nodeOrderStr + "'")
            sigStr = suffix[1:]
            try:
                killSig = int(sigStr)
            except Exception as e:
                Utils.errorExit("Failed to parse kill signal '" + sigStr + "' when parsing --node-order '" + nodeOrderStr + "'")
            opts["killsig"] = killSig
            lastKillSig[ndT] = killSig
        elif sym == "+":
            cmdT = CmdT.RESTART
            if suffix == 'c':
                clean = True
            elif suffix == 'n':
                clean = False
            else:
                Utils.errorExit("Unknown restart flag '" + suffix + "' when parsing --node-order '" + nodeOrderStr + "'")
            opts['clean'] = clean
        else:
            Utils.errorExit("'+' or '_' expected got '" + sym + "' when parsing --node-order '" + nodeOrderStr + "'")

        cmdList.append((ndT, cmdT, opts))
           
    cmdSched.append(cmdList * nReps)
   
    for cmdList in cmdSched:
        print('cmdList=', cmdList)
        for cmd in cmdList:
            
            nd = cmd[0]
            nd_cmd = cmd[1]
            opts = cmd[2]
            if nd_cmd == CmdT.STOP:
                s = "killsig= " + str(opts['killsig'])
            else:
                s = "clean= " + str(opts['clean'])

            if nd == NodeT.PROD:
                if nd_cmd == CmdT.STOP:
                    print("Stopping producer " + s)
                    cluster.stopProducer(opts['killsig'])
                else:
                    print("Restarting producer " + s)
                    cluster.restartProducer(clean=opts['clean'])
            elif nd == NodeT.SHIP:
                if nd_cmd == CmdT.STOP:
                    print("Stopping SHIP " + s)
                    cluster.stopShip(opts['killsig'])
                else:
                    print("Restarting SHIP " + s)
                    cluster.restartShip(clean=opts['clean'])
            elif nd == NodeT.RODEOS:
                if nd_cmd == CmdT.STOP:
                    print("Stopping rodeos " + s)
                    cluster.stopRodeos(opts['killsig'])
                else:
                    print("Restarting rodeos " + s)
                    cluster.restartRodeos(clean=opts['clean'])
                        
            sleepTime = stopWait
            if nd_cmd == CmdT.RESTART:
                sleepTime = restartWait
                
            print("Waiting " + str(sleepTime) + " seconds ...")
            time.sleep(sleepTime)         

    # Big enough to have new blocks produced
    numBlocks=120
    assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks".format(numBlocks)
    assert cluster.allBlocksReceived(numBlocks), "Rodeos did not receive {} blocks".format(numBlocks)

    testSuccessful=True
    cluster.setTestSuccessful(testSuccessful)
    
errorCode = 0 if testSuccessful else 1
exit(errorCode)
