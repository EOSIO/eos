#!/usr/bin/env python3

import signal
import time

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper


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

# Wrapper function to execute test
# This wrapper function will resurrect the node to be tested, and shut
# it down by the end of the test
def executeTest(cluster, producerNode, testNodeId, testNodeArgs, resultMsgs):
    testResult = False
    resultDesc = "!!!BUG IS CONFIRMED ON TEST CASE #{} ({})".format(
        testNodeId,
        testNodeArgs
    )

    try:
        Print(
            "Launch node #{} to execute test scenario: {}".format(
                testNodeId,
                testNodeArgs
            )
        )

        # Get current information from the producer.
        producerNode.waitForHeadToAdvance()

        # Launch the node with the terminate-at-block option specified
        # in testNodeArgs. The option for each node has already
        # been set via specificExtraNodeosArg in the cluster launch.
        cluster.launchUnstarted(cachePopen=True)

        # Wait for node to start up.
        time.sleep(10)

        Print(testNodeArgs)
        Print(" ".join([
            "The test node has begun receiving from the producing node and",
            "is expected to stop at or little bigger than the block number",
            "specified here: ",
            testNodeArgs
        ]))

        # Read block information from the test node as it runs.
        testNode = cluster.getNode(testNodeId)
        head, lib = getBlockNumInfo(testNode)

        Print("Test node head = {}, lib = {}.".format(head, lib))

        if "irreversible" in testNodeArgs:
            checkIrreversible(head, lib)
        else:
            checkHeadOrSpeculative(head, lib)

        resultDesc = "!!!TEST CASE #{} ({}) IS SUCCESSFUL".format(
            testNodeId,
            testNodeArgs
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


# Setup cluster and it's wallet manager
walletMgr = WalletMgr(True)
cluster = Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

# List to contain the test result message
testResultMsgs = []
testSuccessful = False
try:
    specificNodeosArgs = {
        0 : "--enable-stale-production",
        1 : "--read-mode irreversible --terminate-at-block=150",
        2 : "--read-mode speculative --terminate-at-block=200",
        3 : "--read-mode head --terminate-at-block=250",
    }
    traceNodeosArgs = " --plugin eosio::trace_api_plugin --trace-no-abis "

    # Kill any existing instances and launch cluster
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    cluster.launch(
        prodCount=numOfProducers,
        totalProducers=numOfProducers,
        totalNodes=totalNodes,
        unstartedNodes=totalNodes - numOfProducers,
        pnodes=1,
        useBiosBootFile=False,
        topo="mesh",
        specificExtraNodeosArgs=specificNodeosArgs,
        extraNodeosArgs=traceNodeosArgs
    )

    producingNodeId = 0
    producingNode = cluster.getNode(producingNodeId)

    # Start executing test cases here
    Utils.Print("Script Begin .............................")

    for nodeId, nodeArgs in specificNodeosArgs.items():
        # The test only needs to be run on the non-producer nodes.
        if nodeId == producingNodeId:
            continue

        success = executeTest(
            cluster,
            producingNode,
            nodeId,
            nodeArgs,
            testResultMsgs
        )

        if not success:
            break
    else:
        testSuccessful = True

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
