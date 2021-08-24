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
        checkReplay,
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

        # Relaunch killed node so it can be used for the test
        assert testNode.relaunch(cachePopen=True)
        assert producerNode.relaunch(cachePopen=True)

        # Kill node and replay
        assert testNode.kill(signal.SIGTERM)

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

        # Run test scenario
        replay = getReplayInfo(cluster, testNode)
        checkReplay(replay["block_num"], termAtBlockNum)

        resultDesc = "!!!TEST CASE #{} ({}) IS SUCCESSFUL".format(
            testNode.nodeId,
            readMode
        )
        testResult = True

    except Exception as e:
        resultDesc = "!!!BUG IS CONFIRMED ON TEST CASE #{} ({}): {}".format(
            testNode.nodeId,
            readMode,
            e
        )

    finally:
        Print(resultDesc)
        resultMsgs.append(resultDesc)

        # Kill nodes after use
        if not producerNode.killed:
            assert producerNode.kill(signal.SIGTERM)

        if not testNode.killed:
            assert testNode.kill(signal.SIGTERM)

    return testResult


def getReplayInfo(cluster, testNode):
    log = cluster.getBlockLog(testNode.nodeId)
    assert log
    return log[-1]


def checkHead(head, termAtBlockNum):
    assert head >= termAtBlockNum, (
        "Head ({}) should no less than termAtBlockNum ({})".format(
            head,
            termAtBlockNum
        )
    )


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

    # Give some time for it to produce, so lib is advancing
    producingNode.waitForHeadToAdvance()

    # Kill all nodes, so we can test all node in isolated environment
    for clusterNode in cluster.nodes:
        clusterNode.kill(signal.SIGTERM)
    cluster.biosNode.kill(signal.SIGTERM)

    # Start executing test cases here
    Utils.Print("Script Begin .............................")

    irreversibleSuccess = executeTest(
        cluster,
        producingNode,
        cluster.getNode(1),
        "irreversible",
        checkHead,
        testResultMsgs
    )

    speculativeSuccess = executeTest(
        cluster,
        producingNode,
        cluster.getNode(2),
        "speculative",
        checkHead,
        testResultMsgs
    )

    headSuccess = executeTest(
        cluster,
        producingNode,
        cluster.getNode(3),
        "head",
        checkHead,
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
