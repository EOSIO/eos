#!/usr/bin/env python3

from testUtils import Account
from testUtils import WaitSpec
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from Node import BlockType
from TestHelper import TestHelper
from TestHelper import AppArgs
from testUtils import BlockLogAction
import json
import sys
import signal
import time

###############################################################
# nodeos_latency_test.py
#
###############################################################

# Parse command line arguments
args = TestHelper.parse_args({"-v","--dump-error-details","--keep-logs"})
Utils.Debug = args.v
killAll=True
dumpErrorDetails=args.dump_error_details
dontKill=False
killEosInstances=not dontKill
killWallet=not dontKill
keepLogs=args.keep_logs

walletMgr=WalletMgr(True)
cluster=Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

testSuccessful = False
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    topo = { 0 : [1],
             1 : [0,2,3,4,5],
             2 : [1],
             3 : [1],
             4 : [1],
             5 : [1] }

    assert cluster.launch(
        pnodes=1,
        prodCount=1,
        totalProducers=1,
        totalNodes=6,
        useBiosBootFile=False,
        loadSystemContract=False,
        topo=topo)

    Utils.Print("Validating system accounts after bootstrap")
    # cluster.validateAccounts(None)

    cluster.biosNode.kill(signal.SIGTERM)
    
    pnode = cluster.getNode(0)
    Utils.Print("producer node: {}".format(pnode.nodeId))
    snode = cluster.getNode(1)
    Utils.Print("source node: {}".format(snode.nodeId))
    syncNodes = cluster.getNodes()[2:5]
    Utils.Print("Sync nodes: {}".format([node.nodeId for node in syncNodes]))
    backupNode = cluster.getNode(5)
    Utils.Print("backup node: {}".format(backupNode.nodeId))

    for node in cluster.getNodes():
        assert node.waitForLibToAdvance(), Utils.Print("node {} is not syncing".format(node.nodeId))

    for node in syncNodes:
        if not node.kill(signal.SIGTERM):
            Utils.errorExit("failed to kill node {}".format(node.nodeId))

    Utils.Print("Restarting sync nodes")
    for node in syncNodes:
        node.relaunch(cachePopen=True)

    for node in syncNodes:
        assert node.waitForLibToAdvance(), Utils.Print("node {} is not syncing after restart".format(node.nodeId))

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)