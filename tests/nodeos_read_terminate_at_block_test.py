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
numOfProducers = 1
totalNodes = 4

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
def executeTest(producerNode, testNode, readMode, checkResult, resultMsgs):
    testResult = False
    resultDesc = "!!!BUG IS CONFIRMED ON TEST CASE #{} ({})".format(
        testNode.nodeId,
        readMode
    )

    try:
        Print(
            "Re-launch node #{} to execute test scenario: {}".format(
                testNode.nodeId,
                readMode
            )
        )

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

        # Relaunch killed node with the terminate-at-block option.
        assert testNode.relaunch(
            cachePopen=True,
            chainArg=chainArg,
            waitForTerm=False
        )

        Print(" ".join([
            "Test node has begun receiving from the producing node and",
            "is expected to stop at or little bigger than block number",
            str(termAtBlockNum)
        ]))

        # Read block information from the test node as it runs.
        head, lib = getBlockNumInfo(testNode)
        Print("Test node head = {}, lib = {}.".format(head, lib))

        checkResult(head, lib)

        resultDesc = "!!!TEST CASE #{} ({}) IS SUCCESSFUL".format(
            testNode.nodeId,
            readMode
        )
        testResult = True

    finally:
        Print(resultDesc)
        resultMsgs.append(resultDesc)

        # Kill node after use.
        if not testNode.killed:
            assert testNode.kill(signal.SIGTERM)

    return testResult


def getBlockNumInfo(testNode):
    head = None
    lib = None

    while True:
        info = testNode.getInfo()

        if not info:
            break

        try:
            head = info["head_block_num"]
            lib = info["last_irreversible_block_num"]

        except KeyError:
            pass

    assert head and lib, "Could not retrieve head and lib with getInfo()"
    return head, lib


def checkIrreversible(head, lib):
    assert head == lib, (
        "Head ({}) should be equal to lib ({})".format(head, lib)
    )


def checkHeadOrSpeculative(head, lib):
    assert head > lib, (
        "Head ({}) should be greater than lib ({})".format(head, lib)
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
            0 : "--enable-stale-production",
        }
    )

    producingNodeId = 0
    producingNode = cluster.getNode(producingNodeId)

    # Kill all nodes, so we can test all node in isolated environment
    cluster.killSomeEosInstances(totalNodes, Utils.SigTermTag)
    # Restart the producer for the tests.
    assert producingNode.relaunch(cachePopen=True)

    # Start executing test cases here
    Utils.Print("Script Begin .............................")

    irreversibleSuccess = executeTest(
        producingNode,
        cluster.getNode(1),
        "irreversible",
        checkIrreversible,
        testResultMsgs
    )

    speculativeSuccess = executeTest(
        producingNode,
        cluster.getNode(2),
        "speculative",
        checkHeadOrSpeculative,
        testResultMsgs
    )

    headSuccess = executeTest(
        producingNode,
        cluster.getNode(3),
        "head",
        checkHeadOrSpeculative,
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
