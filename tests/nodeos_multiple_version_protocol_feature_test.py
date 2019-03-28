#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster, PFSetupPolicy
from TestHelper import TestHelper
from WalletMgr import WalletMgr
from Node import Node

import signal
import json
import time
from os.path import join, exists
from datetime import datetime

# Parse command line arguments
args = TestHelper.parse_args({"-v","--clean-run","--dump-error-details","--leave-running",
                              "--keep-logs", "--alternate-version-labels-file"})
Utils.Debug=args.v
killAll=args.clean_run
dumpErrorDetails=args.dump_error_details
dontKill=args.leave_running
killEosInstances=not dontKill
killWallet=not dontKill
keepLogs=args.keep_logs
alternateVersionLabelsFile=args.alternate_version_labels_file

walletMgr=WalletMgr(True)
cluster=Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

def restartNode(node: Node, nodeId, chainArg=None, addOrSwapFlags=None, nodeosPath=None):
    if not node.killed:
        node.kill(signal.SIGTERM)
    isRelaunchSuccess = node.relaunch(nodeId, chainArg, addOrSwapFlags=addOrSwapFlags,
                                      timeout=5, cachePopen=True, nodeosPath=nodeosPath)
    assert isRelaunchSuccess, "Fail to relaunch"

def shouldNodeContainPreactivateFeature(node):
    preactivateFeatureDigest = node.getSupportedProtocolFeatureDict()["PREACTIVATE_FEATURE"]["feature_digest"]
    assert preactivateFeatureDigest
    blockHeaderState = node.getLatestBlockHeaderState()
    activatedProtocolFeatures = blockHeaderState["activated_protocol_features"]["protocol_features"]
    return preactivateFeatureDigest in activatedProtocolFeatures

def waitUntilBeginningOfProdTurn(node, producerName, timeout=30, sleepTime=0.4):
    def isDesiredProdTurn():
        headBlockNum = node.getHeadBlockNum()
        res =  node.getBlock(headBlockNum)["producer"] == producerName and \
               node.getBlock(headBlockNum-1)["producer"] != producerName
        return res
    Utils.waitForBool(isDesiredProdTurn, timeout, sleepTime)

def waitForOneRound():
    time.sleep(24) # We have 4 producers for this test

def setValidityOfActTimeSubjRestriction(node, nodeId, codename, valid):
    invalidActTimeSubjRestriction = {
        "earliest_allowed_activation_time": "2030-01-01T00:00:00.000",
    }
    validActTimeSubjRestriction = {
        "earliest_allowed_activation_time": "1970-01-01T00:00:00.000",
    }
    actTimeSubjRestriction = validActTimeSubjRestriction if valid else invalidActTimeSubjRestriction
    node.modifyBuiltinPFSubjRestrictions(nodeId, codename, actTimeSubjRestriction)
    restartNode(node, nodeId)

# List to contain the test result message
testSuccessful = False
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    # Create a cluster of 4 nodes, each node has 1 producer. The first 3 nodes use the latest vesion,
    # While the 4th node use the version that doesn't support protocol feature activation (i.e. 1.7.0)
    associatedNodeLabels = {
        "3": "170"
    }
    assert exists(alternateVersionLabelsFile), "Alternate version labels file does not exist"
    cluster.launch(pnodes=4, totalNodes=4, prodCount=1, totalProducers=4,
                   extraNodeosArgs=" --plugin eosio::producer_api_plugin ",
                   useBiosBootFile=False,
                   onlySetProds=True,
                   pfSetupPolicy=PFSetupPolicy.NONE,
                   alternateVersionLabelsFile=alternateVersionLabelsFile,
                   associatedNodeLabels=associatedNodeLabels)

    def pauseBlockProduction():
        for node in cluster.nodes:
            node.sendRpcApi("v1/producer/pause")

    def resumeBlockProduction():
        for node in cluster.nodes:
            node.sendRpcApi("v1/producer/resume")

    def shouldNodesBeInSync(nodes:[Node]):
        # Pause all block production to ensure the head is not moving
        pauseBlockProduction()
        time.sleep(1) # Wait for some time to ensure all blocks are propagated
        headBlockIds = []
        for node in nodes:
            headBlockId = node.getInfo()["head_block_id"]
            headBlockIds.append(headBlockId)
        resumeBlockProduction()
        return len(set(headBlockIds)) == 1

    newNodeIds = [0, 1, 2]
    oldNodeId = 3
    newNodes = list(map(lambda id: cluster.getNode(id), newNodeIds))
    oldNode = cluster.getNode(oldNodeId)
    allNodes = [*newNodes, oldNode]

    # Before everything starts, all nodes (new version and old version) should be in sync
    assert shouldNodesBeInSync(allNodes), "Nodes are not in sync before preactivation"

    # First, we are going to test the case where:
    # - 1st node has valid earliest_allowed_activation_time
    # - While 2nd and 3rd node have invalid earliest_allowed_activation_time
    # Producer in the 1st node is going to activate PREACTIVATE_FEATURE during his turn
    # Immediately, in the next block PREACTIVATE_FEATURE should be active in 1st node, but not on 2nd and 3rd
    # Therefore, 1st node will be out of sync with 2nd, 3rd, and 4th node
    # After a round has passed though, 1st node will realize he's in minority fork and then join the other nodes
    # Hence, the PREACTIVATE_FEATURE that was previously activated will be dropped and all of the nodes should be in sync
    setValidityOfActTimeSubjRestriction(newNodes[1], newNodeIds[1], "PREACTIVATE_FEATURE", False)
    setValidityOfActTimeSubjRestriction(newNodes[2], newNodeIds[2], "PREACTIVATE_FEATURE", False)

    waitUntilBeginningOfProdTurn(newNodes[0], "defproducera")
    newNodes[0].activatePreactivateFeature()
    assert shouldNodeContainPreactivateFeature(newNodes[0]), "1st node should contain PREACTIVATE FEATURE"
    assert not (shouldNodeContainPreactivateFeature(newNodes[1]) or shouldNodeContainPreactivateFeature(newNodes[2])), \
           "2nd and 3rd node should not contain PREACTIVATE FEATURE"
    assert shouldNodesBeInSync([newNodes[1], newNodes[2], oldNode]), "2nd, 3rd and 4th node should be in sync"
    assert not shouldNodesBeInSync(allNodes), "1st node should be out of sync with the rest nodes"

    waitForOneRound()

    assert not shouldNodeContainPreactivateFeature(newNodes[0]), "PREACTIVATE_FEATURE should be dropped"
    assert shouldNodesBeInSync(allNodes), "All nodes should be in sync"

    # Then we set the earliest_allowed_activation_time of 2nd node and 3rd node with valid value
    # Once the 1st node activate PREACTIVATE_FEATURE, all of them should have PREACTIVATE_FEATURE activated in the next block
    # They will be in sync and their LIB will advance since they control > 2/3 of the producers
    # However, the 4th node will be out of sync with them, and his LIB will never advance
    setValidityOfActTimeSubjRestriction(newNodes[1], newNodeIds[1], "PREACTIVATE_FEATURE", True)
    setValidityOfActTimeSubjRestriction(newNodes[2], newNodeIds[2], "PREACTIVATE_FEATURE", True)

    waitUntilBeginningOfProdTurn(newNodes[0], "defproducera")
    newNodes[0].activatePreactivateFeature()

    assert shouldNodesBeInSync(newNodes), "New nodes should be in sync"
    assert not shouldNodesBeInSync(allNodes), "Nodes should not be in sync after preactivation"
    for node in newNodes: assert shouldNodeContainPreactivateFeature(node), "New node should contain PREACTIVATE_FEATURE"
    assert newNodes[0].waitForLibToAdvance(), "1st node LIB should advance"
    newNode0Lib = newNodes[0].getIrreversibleBlockNum()
    assert newNodes[1].getIrreversibleBlockNum() >= newNode0Lib and \
           newNodes[2].getIrreversibleBlockNum() >= newNode0Lib, "2nd and 3rd node LIB should also advance"
    assert not oldNode.waitForLibToAdvance(), "4th node LIB should not advance"

    # Restart old node with newest version
    # Finally, when we restart the 4th node with the version of nodeos that supports protocol feature,
    # all nodes should be in sync, and the 4th node will also contain PREACTIVATE_FEATURE
    restartNode(oldNode, oldNodeId, chainArg=" --replay ", nodeosPath="programs/nodeos/nodeos")
    time.sleep(2) # Give some time to replay

    assert shouldNodesBeInSync(allNodes), "All nodes should be in sync"
    assert shouldNodeContainPreactivateFeature(oldNode), "4th node should contain PREACTIVATE_FEATURE"

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
