#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import BlockType
from TestHelper import TestHelper

import signal
import json
import os
import shutil

###############################################################
# privacy_config_test_snapshot
#
# This test ensures that node without ssl configuration can't start 
# from snapshot that contains SECURITY_GROUP feature activated
#
###############################################################

def cleanNodeData(nodeId):
    dataDir = Utils.getNodeDataDir(nodeId)
    state = os.path.join(dataDir, "state")
    # removing state dir
    shutil.rmtree(state, ignore_errors=True)

    existingBlocksDir = os.path.join(dataDir, "blocks")
    # removing blocks dir
    shutil.rmtree(existingBlocksDir, ignore_errors=True)

Print=Utils.Print

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v"})

pnodes=1
totalNodes=1
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
Utils.Debug=args.v

testSuccessful=False
cluster=Cluster(host=TestHelper.LOCAL_HOST, port=TestHelper.DEFAULT_PORT, walletd=True, walletMgr=WalletMgr(True))
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=True, cleanup=True)

    if not cluster.launch(pnodes=pnodes, totalNodes=totalNodes, configSecurityGroup=True, onlyBios=True, printInfo=True):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    cluster.biosNode.waitForLibToAdvance()

    snapshot = cluster.biosNode.createSnapshot()
    snapshot_path = snapshot["snapshot_name"]
    snapshot_head = snapshot["head_block_num"]
    Print("snapshot data: {}".format(json.dumps(snapshot, indent=4, sort_keys=True)))

    cluster.biosNode.kill(signal.SIGTERM)

    # make sure that we are able to restart from snapshot with TLS flags
    chainArg = "--snapshot {}".format(snapshot_path)
    cleanNodeData("bios")
    cluster.biosNode.relaunch(chainArg=chainArg, timeout=15, cachePopen=True)
    cluster.biosNode.waitForLibToAdvance()

    # if we are here that means we restarted from snapshot successfully
    # now let's disable tls flags and restart again

    cluster.biosNode.kill(signal.SIGTERM)

    deleteFlags={"--p2p-tls-own-certificate-file" : True, 
                 "--p2p-tls-private-key-file" : True, 
                 "--p2p-tls-security-group-ca-file" : True}
    cleanNodeData("bios")
    if False == cluster.biosNode.relaunch(deleteFlags=deleteFlags, timeout=15, cachePopen=True):
        Print("****************************")
        Print("^^^ ERROR ABOVE IS EXPECTED!")
        Print("****************************")
        Print("SUCCESS: cluster failed to restart without ssl command line parameters")
    else:
        Utils.errorExit("FAIL: Cluster was not supposed to restart succesfully")

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)