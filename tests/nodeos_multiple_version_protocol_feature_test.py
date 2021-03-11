#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster, PFSetupPolicy
from TestHelper import TestHelper
from WalletMgr import WalletMgr
from Node import Node

import signal
import json
import time
import os
from os.path import join, exists
from datetime import datetime

###############################################################
# nodeos_multiple_version_protocol_feature_test
#
# Test for verifying that older versions of nodeos can work with newer versions of nodeos.
#
###############################################################

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

def restartNode(node: Node, chainArg=None, addSwapFlags=None, nodeosPath=None):
    if not node.killed:
        node.kill(signal.SIGTERM)
    isRelaunchSuccess = node.relaunch(chainArg, addSwapFlags=addSwapFlags,
                                      timeout=5, cachePopen=True, nodeosPath=nodeosPath)
    assert isRelaunchSuccess, "Fail to relaunch"

def shouldNodeContainPreactivateFeature(node):
    preactivateFeatureDigest = node.getSupportedProtocolFeatureDict()["PREACTIVATE_FEATURE"]["feature_digest"]
    assert preactivateFeatureDigest, "preactivateFeatureDigest should not be empty"
    blockHeaderState = node.getLatestBlockHeaderState()
    assert blockHeaderState, "blockHeaderState should not be empty"
    activatedProtocolFeatures = blockHeaderState["activated_protocol_features"]["protocol_features"]
    return preactivateFeatureDigest in activatedProtocolFeatures

beginningOfProdTurnHead = 0
def waitUntilBeginningOfProdTurn(node, producerName, timeout=30, sleepTime=0.4):
    def isDesiredProdTurn():
        beginningOfProdTurnHead = node.getHeadBlockNum()
        res =  node.getBlock(beginningOfProdTurnHead)["producer"] == producerName and \
               node.getBlock(beginningOfProdTurnHead-1)["producer"] != producerName
        return res
    ret = Utils.waitForTruth(isDesiredProdTurn, timeout, sleepTime)
    assert ret != None, "Expected producer to arrive within 19 seconds (with 3 other producers)"

def waitForOneRound():
    time.sleep(24) # We have 4 producers for this test

def setValidityOfActTimeSubjRestriction(node, codename, valid):
    invalidActTimeSubjRestriction = {
        "earliest_allowed_activation_time": "2030-01-01T00:00:00.000",
    }
    validActTimeSubjRestriction = {
        "earliest_allowed_activation_time": "1970-01-01T00:00:00.000",
    }
    actTimeSubjRestriction = validActTimeSubjRestriction if valid else invalidActTimeSubjRestriction
    node.modifyBuiltinPFSubjRestrictions(codename, actTimeSubjRestriction)
    restartNode(node)

def waitUntilBlockBecomeIrr(node, blockNum, timeout=60):
    def hasBlockBecomeIrr():
        return node.getIrreversibleBlockNum() >= blockNum
    return Utils.waitForTruth(hasBlockBecomeIrr, timeout)

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
    Utils.Print("Alternate Version Labels File is {}".format(alternateVersionLabelsFile))
    assert exists(alternateVersionLabelsFile), "Alternate version labels file does not exist"
    # version 1.7 did not provide a default value for "--last-block-time-offset-us" so this is needed to
    # avoid dropping late blocks
    assert cluster.launch(pnodes=4, totalNodes=4, prodCount=1, totalProducers=4,
                          extraNodeosArgs=" --plugin eosio::producer_api_plugin ",
                          useBiosBootFile=False,
                          specificExtraNodeosArgs={
                             0:"--http-max-response-time-ms 990000",
                             1:"--http-max-response-time-ms 990000",
                             2:"--http-max-response-time-ms 990000",
                             3:"--last-block-time-offset-us -200000"},
                          onlySetProds=True,
                          pfSetupPolicy=PFSetupPolicy.NONE,
                          alternateVersionLabelsFile=alternateVersionLabelsFile,
                          associatedNodeLabels=associatedNodeLabels), "Unable to launch cluster"

    newNodeIds = [0, 1, 2]
    oldNodeId = 3
    newNodes = list(map(lambda id: cluster.getNode(id), newNodeIds))
    oldNode = cluster.getNode(oldNodeId)
    allNodes = [*newNodes, oldNode]

    def pauseBlockProductions():
        for node in allNodes:
            if not node.killed: node.processCurlCmd("producer", "pause", "")

    def resumeBlockProductions():
        for node in allNodes:
            if not node.killed: node.processCurlCmd("producer", "resume", "")

    def areNodesInSync(nodes:[Node], pauseAll=True, resumeAll=True):
        # Pause all block production to ensure the head is not moving
        if pauseAll:
            pauseBlockProductions()
            time.sleep(2) # Wait for some time to ensure all blocks are propagated

        # Get current head block number and IDs for each producer
        headBlockNums = []
        headBlockIds = []
        for node in nodes:
            hb = node.getInfo()
            headBlockNums.append(hb["head_block_num"])
            headBlockIds.append(hb["head_block_id"])
            Utils.Print("node {}, head block id: {}, num: {}".format(node.nodeId, hb["head_block_id"], hb["head_block_num"]))
        assert len(set(headBlockNums)) == len(set(headBlockIds)), "Different block IDs have the same block numbers, thus nodes are not in sync"
        # Check if each node has head blocks from other producers
        inSync = True
        if len(set(headBlockNums)) != 1:
            def nodeHasBlocks(node, blockIds, blockNums):
                for blkId, blkNum in zip(blockIds, blockNums):
                    assert node.waitForBlock(blkNum, timeout=3) != None, "Expected to find block {}, but only reached {}".format(blkNum, node.getInfo()["head_block_num"])
                    if node.getBlock(blkId) is None:
                        Utils.Print("node {} does not have block Id: {} (num: {})".format(node.nodeId, blkId, blkNum))
                        return False
                return True
            for node in nodes:
                if not nodeHasBlocks(node, headBlockIds, headBlockNums): 
                    inSync = False
                    break

        if resumeAll:
            resumeBlockProductions()
        return inSync

    Utils.Print("+++ Nodes are in sync before preactivation +++")
    # Before everything starts, all nodes (new version and old version) should be in sync
    assert areNodesInSync(allNodes), "Nodes are not in sync before preactivation"

    # First, we are going to test the case where:
    # - 1st node has valid earliest_allowed_activation_time
    # - While 2nd and 3rd node have invalid earliest_allowed_activation_time
    # Producer in the 1st node is going to activate PREACTIVATE_FEATURE during his turn
    # Immediately, in the next block PREACTIVATE_FEATURE should be active in 1st node, but not on 2nd and 3rd
    # Therefore, 1st node will be out of sync with 2nd, 3rd, and 4th node
    # After a round has passed though, 1st node will realize he's in minority fork and then join the other nodes
    # Hence, the PREACTIVATE_FEATURE that was previously activated will be dropped and all of the nodes should be in sync
    Utils.Print("+++ 1st Node should contain PREACTIVATE FEATURE +++")
    setValidityOfActTimeSubjRestriction(newNodes[1], "PREACTIVATE_FEATURE", False)
    setValidityOfActTimeSubjRestriction(newNodes[2], "PREACTIVATE_FEATURE", False)

    for i in range(3):
        Utils.Print("1st node tries activatePreactivateFeature time(s): {}".format(i+1))
        # 1st node waits for the start of the production turn each time it tries activatePreactivateFeature()
        waitUntilBeginningOfProdTurn(newNodes[0], "defproducera")
        newNodes[0].activatePreactivateFeature()
        if shouldNodeContainPreactivateFeature(newNodes[0]):
            break
        diff = newNodes[0].getInfo()["head_block_num"] - beginningOfProdTurnHead
        assert diff >= 12, "1st node should contain PREACTIVATE FEATURE since we set it during its production window"

    assert shouldNodeContainPreactivateFeature(newNodes[0]), "1st node should contain PREACTIVATE FEATURE"
    assert not (shouldNodeContainPreactivateFeature(newNodes[1]) or shouldNodeContainPreactivateFeature(newNodes[2])), \
        "2nd and 3rd node should not contain PREACTIVATE FEATURE"
    Utils.Print("+++ 2nd, 3rd and 4th node should be in sync, and 1st node should be out of sync +++")
    assert areNodesInSync([newNodes[1], newNodes[2], oldNode], pauseAll=True, resumeAll=False), "2nd, 3rd and 4th node should be in sync"
    assert not areNodesInSync(allNodes, pauseAll=False, resumeAll=True), "+++ 1st node should be out of sync with the rest nodes +++"

    waitForOneRound()

    assert not shouldNodeContainPreactivateFeature(newNodes[0]), "PREACTIVATE_FEATURE should be dropped"
    assert areNodesInSync(allNodes), "All nodes should be in sync"

    # Then we set the earliest_allowed_activation_time of 2nd node and 3rd node with valid value
    # Once the 1st node activate PREACTIVATE_FEATURE, all of them should have PREACTIVATE_FEATURE activated in the next block
    # They will be in sync and their LIB will advance since they control > 2/3 of the producers
    # Also the LIB should be able to advance past the block that contains PREACTIVATE_FEATURE
    # However, the 4th node will be out of sync with them, and its LIB will stuck
    setValidityOfActTimeSubjRestriction(newNodes[1], "PREACTIVATE_FEATURE", True)
    setValidityOfActTimeSubjRestriction(newNodes[2], "PREACTIVATE_FEATURE", True)

    waitUntilBeginningOfProdTurn(newNodes[0], "defproducera")
    libBeforePreactivation = newNodes[0].getIrreversibleBlockNum()
    newNodes[0].activatePreactivateFeature()

    assert areNodesInSync(newNodes, pauseAll=True, resumeAll=False), "New nodes should be in sync"
    assert not areNodesInSync(allNodes, pauseAll=False, resumeAll=True), "Nodes should not be in sync after preactivation"
    for node in newNodes: assert shouldNodeContainPreactivateFeature(node), "New node should contain PREACTIVATE_FEATURE"

    activatedBlockNum = newNodes[0].getHeadBlockNum() # The PREACTIVATE_FEATURE should have been activated before or at this block num
    assert waitUntilBlockBecomeIrr(newNodes[0], activatedBlockNum), \
           "1st node LIB should be able to advance past the block that contains PREACTIVATE_FEATURE"
    assert newNodes[1].getIrreversibleBlockNum() >= activatedBlockNum and \
           newNodes[2].getIrreversibleBlockNum() >= activatedBlockNum, \
           "2nd and 3rd node LIB should also be able to advance past the block that contains PREACTIVATE_FEATURE"
    assert oldNode.getIrreversibleBlockNum() <= libBeforePreactivation, \
           "4th node LIB should be stuck on LIB before PREACTIVATE_FEATURE is activated"

    # Restart old node with newest version
    # Before we are migrating to new version, use --export-reversible-blocks as the old version
    # and --import-reversible-blocks with the new version to ensure the compatibility of the reversible blocks
    # Finally, when we restart the 4th node with the version of nodeos that supports protocol feature,
    # all nodes should be in sync, and the 4th node will also contain PREACTIVATE_FEATURE
    portableRevBlkPath = os.path.join(Utils.getNodeDataDir(oldNodeId), "rev_blk_portable_format")
    oldNode.kill(signal.SIGTERM)
    # Note, for the following relaunch, these will fail to relaunch immediately (expected behavior of export/import), so the chainArg will not replace the old cmd
    oldNode.relaunch(chainArg="--export-reversible-blocks {}".format(portableRevBlkPath), timeout=1)
    oldNode.relaunch(chainArg="--import-reversible-blocks {}".format(portableRevBlkPath), timeout=1, nodeosPath="programs/nodeos/nodeos")
    os.remove(portableRevBlkPath)

    restartNode(oldNode, chainArg="--replay", nodeosPath="programs/nodeos/nodeos")
    time.sleep(2) # Give some time to replay

    assert areNodesInSync(allNodes), "All nodes should be in sync"
    assert shouldNodeContainPreactivateFeature(oldNode), "4th node should contain PREACTIVATE_FEATURE"

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
