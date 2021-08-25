#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper
from testUtils import Account

import re
import os
import time
import signal
import subprocess
import shutil

###############################################################
# nodeos_read_terminate_at_block_test
#
# A few tests centered around read mode of irreversible,
# speculative and head with terminate-at-block set
#
###############################################################

Print = Utils.Print
errorExit = Utils.errorExit
cmdError = Utils.cmdError
relaunchTimeout = 10
numOfProducers = 4
totalNodes = 10
bufferTime = 6

termAtFutureBlockNum = 30

# Parse command line arguments
args = TestHelper.parse_args({
    "-v",
    "--clean-run",
    "--dump-error-details",
    "--leave-running",
    "--keep-logs"
})

Utils.Debug = args.v
killAll = args.clean_run
dumpErrorDetails = args.dump_error_details
dontKill = args.leave_running
killEosInstances = not dontKill
killWallet = not dontKill
keepLogs = args.keep_logs

# Setup cluster and it's wallet manager
walletMgr = WalletMgr(True)
cluster = Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

# Wrapper function to execute test
# This wrapper function will resurrect the node to be tested, and shut
# it down by the end of the test
def executeTest(
        cluster,
        producerNode,
        testNode,
        readMode,
        resultMsgs
    ):
    testResult = False
    resultDesc = None

    try:
        Print(
            "Re-launch node #{} to execute test scenario: {}".format(
                testNode.nodeId,
                readMode
            )
        )

        assert producerNode.relaunch(cachePopen=True)

        # Get current information from the producer.
        producerNode.waitForHeadToAdvance()
        prodInfo = producingNode.getInfo()
        assert prodInfo

        if readMode == "irreversible":
            blockNum = int(prodInfo["last_irreversible_block_num"])
        else:
            blockNum = int(prodInfo["head_block_num"])

        termAtBlockNum = blockNum + termAtFutureBlockNum

        chainArg = " --read-mode {}  --terminate-at-block={}".format(
            readMode,
            termAtBlockNum
        )

        # Wait for the producer to get to at least termAtBlockNum
        # before relaunching the node.
        producerNode.waitForBlock(termAtBlockNum)

        # Relaunch killed node with the terminate-at-blocka option.
        assert testNode.relaunch(
            cachePopen=True,
            chainArg=chainArg,
            termMaxBlock=termAtBlockNum
        )

        Print(" ".join([
            "Test node has begun receiving from the producing node and",
            "is expected to stop at or little bigger than block number",
            str(termAtBlockNum)
        ]))

        # Node should be finished, get the last block from the block log.
        head = cluster.getBlockLog(testNode.nodeId, throwException=True)[-1]

        assert head["block_num"] >= termAtBlockNum, (
            "Head ({}) should no less than termAtBlockNum ({})".format(
                head["block_num"],
                termAtBlockNum
            )
        )

        resultDesc = "!!!TEST CASE #{} ({}) IS SUCCESSFUL".format(
            testNode.nodeId,
            readMode
        )
        testResult = True

    finally:
        Print(resultDesc)
        resultMsgs.append(resultDesc)

        # Kill nodes after use
        if not producerNode.killed:
            assert producerNode.kill(signal.SIGTERM)

        if not testNode.killed:
            assert testNode.kill(signal.SIGTERM)

    return testResult


# List to contain the test result message
testResultMsgs = []
testSuccessful = True
try:
    # Kill any existing instances and launch cluster
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    cluster.launch(
        prodCount=numOfProducers,
        totalProducers=numOfProducers,
        totalNodes=totalNodes,
        pnodes=1,
        useBiosBootFile=False,
        topo="mesh",
        specificExtraNodeosArgs={
            0:"--enable-stale-production",
            4:"--read-mode irreversible",
            6:"--read-mode irreversible",
            9:"--plugin eosio::producer_api_plugin"})

    producingNodeId = 0
    producingNode = cluster.getNode(producingNodeId)

    # Kill all nodes, so we can test all node in isolated environment
    cluster.killSomeEosInstances(totalNodes, Utils.SigTermTag)

    # Start executing test cases here
    Utils.Print("Script Begin .............................")

    irreversibleSuccess = executeTest(
        cluster,
        producingNode,
        cluster.getNode(1),
        "irreversible",
        testResultMsgs
    )

    speculativeSuccess = executeTest(
        cluster,
        producingNode,
        cluster.getNode(2),
        "speculative",
        testResultMsgs
    )

    headSuccess = executeTest(
        cluster,
        producingNode,
        cluster.getNode(3),
        "head",
        testResultMsgs
    )

    testSuccessful = (
        irreversibleSuccess and speculativeSuccess and headSuccess
    )

    Utils.Print("Script End ................................")

finally:
    TestHelper.shutdown(
        cluster,
        walletMgr,
        testSuccessful,
        killEosInstances,
        killWallet,
        keepLogs,
        killAll,
        dumpErrorDetails
    )

    # Print test result
    for msg in testResultMsgs:
        Print(msg)
    if not testSuccessful and len(testResultMsgs) < 3:
        Print("Subsequent tests were not run after failing test scenario.")

exitCode = 0 if testSuccessful else 1
exit(exitCode)
