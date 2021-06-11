#!/usr/bin/env python3

from SecurityGroup import SecurityGroup
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper

import time
import signal
import json
import os
import shutil

###############################################################
# privacy_network_from_snapshot
#
# This test ensures that security group cluster can be restarte from snapshot
#
###############################################################

def cleanNodeData(nodes):
    for node in nodes:
        dataDir = Utils.getNodeDataDir(node.nodeId)
        state = os.path.join(dataDir, "state")
        # removing state dir
        shutil.rmtree(state, ignore_errors=True)

        existingBlocksDir = os.path.join(dataDir, "blocks")
        # removing blocks dir
        shutil.rmtree(existingBlocksDir, ignore_errors=True)

Print=Utils.Print

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v"})

pnodes=2
totalNodes=4
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
Utils.Debug=args.v

testSuccessful=False
cluster=Cluster(host=TestHelper.LOCAL_HOST, port=TestHelper.DEFAULT_PORT, walletd=True, walletMgr=WalletMgr(True))
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=True, cleanup=True)

    # we need stale production enabled for producers that are participants in order to produce blocks after restart from snapshot
    # node 3 have irreversible read mode in order to save snapshot after being disconnected from security group.
    # without that flag node waits till next lib to arrive
    specificExtraNodeosArgs = { 0 : "--enable-stale-production",
                                1 : "--enable-stale-production",
                                3 : "--read-mode irreversible" }
    if not cluster.launch(pnodes=pnodes, 
                          totalNodes=totalNodes,
                          specificExtraNodeosArgs=specificExtraNodeosArgs,
                          configSecurityGroup=True,
                          printInfo=True):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    cluster.biosNode.kill(signal.SIGTERM)

    # security group consist of 2 producers and 1 api node
    secGroupNodes = [cluster.getNode(0), cluster.getNode(1), cluster.getNode(2)]
    nonSecGroupNodes = [cluster.getNode(3)]

    securityGroup = cluster.getSecurityGroup()

    trans = securityGroup.editSecurityGroup(addNodes=secGroupNodes)
    trans_block_num = secGroupNodes[0].getBlockNumByTransId(trans["transaction_id"], blocksAhead=3)
    assert trans_block_num is not None
    Print("security group update {} published on block {}".format(SecurityGroup.toString(secGroupNodes), trans_block_num))

    # this method will wait till transaction become lib on all participants and check that non-participants go out of sync
    securityGroup.verifySecurityGroup(publishTrans=trans)

    # snapshot from participant node
    snapshot1 = secGroupNodes[0].createSnapshot()
    snapshot1_path = snapshot1["snapshot_name"]
    Print("snapshot data: {}".format(json.dumps(snapshot1, indent=4, sort_keys=True)))

    #snapshot from non-participant node with lib < security group transaction update
    snapshot2 = nonSecGroupNodes[0].createSnapshot()
    snapshot2_path = snapshot2["snapshot_name"]
    Print("snapshot data: {}".format(json.dumps(snapshot2, indent=4, sort_keys=True)))

    # kill all
    cluster.killSomeEosInstances(killCount=len(cluster.getNodes()))

    # we have two snapshots: one with blocks past security group update and another with blocks till this update
    chainArg1 = "--snapshot {}".format(snapshot1_path)
    chainArg2 = "--snapshot {}".format(snapshot2_path)
    # clear data for all
    cleanNodeData(cluster.getNodes())
    # relaunch all nodes from corresponding snapshots
    for node in secGroupNodes:
        node.relaunch(chainArg=chainArg1, cachePopen=True)
    for node in nonSecGroupNodes:
        node.relaunch(chainArg=chainArg2, cachePopen=True)
    
    # wait for lib to move
    secGroupNodes[0].waitForLibToAdvance()

    # verify security group nodes are in sync after lib moved
    cluster.verifyInSync(specificNodes=secGroupNodes)

    # verifying non-participants to stay disconnected from security group
    for node in nonSecGroupNodes:
        Print("node {} head block is {}".format(node.nodeId, node.getHeadBlockNum()))
        assert node.getHeadBlockNum() < trans_block_num, "node {} must not advance past {} block as it is not in security group".format(node.nodeId, trans_block_num)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)