#!/usr/bin/env python3

from testUtils import Utils
import testUtils
from datetime import datetime
import time
from Cluster import Cluster
from core_symbol import CORE_SYMBOL
from WalletMgr import WalletMgr
from Node import BlockType
from Node import Node
from TestHelper import TestHelper
from TestHelper import AppArgs

import decimal
import json
import math
import os
import re
import shutil
import signal

###############################################################
# ship_test
# 
# This test sets up <-p> producing node(s) and <-n - -p>
#   non-producing node(s). One of the non-producing nodes
#   is configured with the state_history_plugin.  An instance
#   of node will be started with a client javascript to exercise
#   the SHiP API.
#
###############################################################

Print=Utils.Print

from core_symbol import CORE_SYMBOL

appArgs = AppArgs()
extraArgs = appArgs.add(flag="--num-requests", type=int, help="How many requests that each ship_client requests", default=1)
extraArgs = appArgs.add(flag="--num-clients", type=int, help="How many ship_clients should be started", default=1)
args = TestHelper.parse_args({"-p", "-n","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"}, applicationSpecificArgs=appArgs)

Utils.Debug=args.v
totalProducerNodes=args.p
totalNodes=args.n
if totalNodes<=totalProducerNodes:
    totalNodes=totalProducerNodes+1
totalNonProducerNodes=totalNodes-totalProducerNodes
totalProducers=totalProducerNodes
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
killAll=args.clean_run
walletPort=TestHelper.DEFAULT_WALLET_PORT

walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName=Utils.EosWalletName
ClientName="cleos"
shipTempDir=None

try:
    TestHelper.printSystemInfo("BEGIN")

    cluster.setWalletMgr(walletMgr)
    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    Print("Stand up cluster")
    specificExtraNodeosArgs={}
    # non-producing nodes are at the end of the cluster's nodes, so reserving the last one for state_history_plugin
    shipNodeNum = totalNodes - 1
    specificExtraNodeosArgs[shipNodeNum]="--plugin eosio::state_history_plugin --disable-replay-opts --sync-fetch-span 200 --plugin eosio::net_api_plugin "

    if cluster.launch(pnodes=totalProducerNodes,
                      totalNodes=totalNodes, totalProducers=totalProducers,
                      useBiosBootFile=False, specificExtraNodeosArgs=specificExtraNodeosArgs) is False:
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")

    # ***   identify each node (producers and non-producing node)   ***

    shipNode = cluster.getNode(shipNodeNum)
    prodNode = cluster.getNode(0)

    #verify nodes are in sync and advancing
    cluster.waitOnClusterSync(blockAdvancing=5)
    Print("Cluster in Sync")

    javascriptClient = "tests/ship_client.js"
    cmd = "node %s --num-requests %d" % (javascriptClient, args.num_requests)
    if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
    clients = []
    files = []
    shipTempDir = "%s/ship/" % (Utils.DataDir)
    os.makedirs(shipTempDir, exist_ok = True)
    shipClientFilePrefix = "%s/client" % (shipTempDir)

    starts = []
    for i in range(0, args.num_clients):
        start = time.perf_counter()
        outFile = open("%s%d.out" % (shipClientFilePrefix, i), "w")
        errFile = open("%s%d.err" % (shipClientFilePrefix, i), "w")
        Print("Start client %d" % (i))
        popen=Utils.delayedCheckOutput(cmd, stdout=outFile, stderr=errFile)
        starts.append(time.perf_counter())
        clients.append((popen, cmd))
        files.append((outFile, errFile))
        Print("Client %d started, Ship node head is: %s" % (i, shipNode.getBlockNum()))

    Print("Stopping all %d clients" % (args.num_clients))

    index = 0
    for popen, _ in clients:
        popen.wait()
        Print("Stopped client %d.  Ran for %s seconds." % (index, time.perf_counter() - starts[index]))
        filePair = files[index]
        filePair[0].close()
        filePair[1].close()
        index += 1

    Print("Shutdown state_history_plugin nodeos")
    shipNode.kill(signal.SIGTERM)

    files = None

    maxFirstBN = None
    minLastBN = None
    for index in range(0, len(clients)):
        done = False
        shipClientErrorFile = "%s%d.err" % (shipClientFilePrefix, i)
        with open(shipClientErrorFile, "r") as errFile:
            statuses = None
            try:
                statuses = json.load(errFile)
            except json.decoder.JSONDecodeError as er:
                Utils.errorExit("javascript client output was malformed in %s. Exception: %s" % (shipClientErrorFile, er))

            for status in statuses:
                statusDesc = status["status"]
                if statusDesc == "done":
                    done = True
                    firstBlockNum = status["first_block_num"]
                    lastBlockNum = status["last_block_num"]
                    if maxFirstBN is None:
                        # initialize both
                        maxFirstBN = firstBlockNum
                        minLastBN = lastBlockNum
                    else:
                        maxFirstBN = max(maxFirstBN, firstBlockNum)
                        minLastBN = min(minLastBN, lastBlockNum)
                    break
                if statusDesc == "error":
                    Utils.errorExit("javascript client reporting error see: %s." % (shipClientErrorFile))

        assert done, Print("ERROR: Did not find a \"done\" status for client %d" % (i))

    Print("All clients active from block num: %s to block_num: %s." % (maxFirstBN, minLastBN))

    stderrFile=Utils.getNodeDataDir(shipNodeNum, "stderr.txt")
    biggestDeltaSeconds = 0.0
    totalDeltaSeconds = 0.0
    timeCount = 0
    with open(stderrFile, 'r') as f:
        line = f.readline()
        while line:
            match = re.search(r'info\s+([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3})\s.+Received\sblock\s+.+\s#([0-9]+)\s@\s([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3})',
                              line)
            if match:
                rcvTimeStr = match.group(1)
                prodTimeStr = match.group(3)
                blockNum = int(match.group(2))
                if blockNum > maxFirstBN:
                    # ship requests can only affect time after clients started
                    timeFmt = '%Y-%m-%dT%H:%M:%S.%f'
                    rcvTime = datetime.strptime(rcvTimeStr, '%Y-%m-%dT%H:%M:%S.%f')
                    prodTime = datetime.strptime(prodTimeStr, timeFmt)
                    delta = rcvTime - prodTime
                    deltaSeconds = delta.total_seconds()
                    if deltaSeconds > biggestDeltaSeconds:
                        biggestDeltaSeconds = deltaSeconds

                    totalDeltaSeconds += deltaSeconds
                    timeCount += 1
                    assert deltaSeconds < 0.100, Print("ERROR: block_num: %s took more than %.3f seconds to be received." % (blockNum, deltaSeconds))

            line = f.readline()

    avg = totalDeltaSeconds / timeCount if timeCount > 0 else 0.0
    Print("Greatest delay time: %s seconds, average delay time: %s seconds" % (biggestDeltaSeconds, avg))

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)
    if shipTempDir is not None:
        if dumpErrorDetails and not testSuccessful:
            for index in range(0, args.num_clients):
                Print(Utils.FileDivider)
                shipClientErrorFile = "%s%d.err" % (shipClientFilePrefix, i)
                with open(shipClientErrorFile, "r") as f:
                    Print("Contents of %s" % (shipClientErrorFile))
                    line = f.readline()
                    while line:
                        Print(line)
                        line = f.readline()
                Print(Utils.FileDivider)
                lineCount = 0
                shipClientOutputFile = "%s%d.out" % (shipClientFilePrefix, i)
                with open(shipClientOutputFile, "r") as f:
                    Print("Contents of %s" % (shipClientOutputFile))
                    line = f.readline()
                    maxLines = 20000
                    while line and lineCount < maxLines:
                        Print(line)
                        lineCount += 1
                        line = f.readline()
                    if line:
                        Print("...       CONTENT TRUNCATED AT %d lines" % (maxLines))

        if not keepLogs:
            shutil.rmtree(shipTempDir, ignore_errors=True)
    if not testSuccessful and dumpErrorDetails:
        Print(Utils.FileDivider)
        Print("Compare Blocklog")
        cluster.compareBlockLogs()
        Print(Utils.FileDivider)
        Print("Print Blocklog")
        cluster.printBlockLog()
        Print(Utils.FileDivider)

errorCode = 0 if testSuccessful else 1
exit(errorCode)
