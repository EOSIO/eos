#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster, PFSetupPolicy
from TestHelper import TestHelper
from WalletMgr import WalletMgr
from Node import Node

import signal
import json
import time
from os.path import join
from datetime import datetime
from urllib.error import HTTPError

# Parse command line arguments
args = TestHelper.parse_args({"-v","--clean-run","--dump-error-details","--leave-running","--keep-logs"})
Utils.Debug = args.v
killAll=args.clean_run
dumpErrorDetails=args.dump_error_details
dontKill=args.leave_running
killEosInstances=not dontKill
killWallet=not dontKill
keepLogs=args.keep_logs

def modifyPFSubjectiveRestriction(nodeId, jsonName, subjectiveRestrictionKey, newValue):
    jsonPath = join(Cluster.getConfigDir(nodeId), "protocol_features", jsonName)
    protocolFeatureJson = []
    with open(jsonPath) as f:
        protocolFeatureJson = json.load(f)
    protocolFeatureJson["subjective_restrictions"][subjectiveRestrictionKey] = newValue
    with open(jsonPath, "w") as f:
        json.dump(protocolFeatureJson, f, indent=2)

def modifyPreactFeatEarliestActivationTime(nodeId, newTime:datetime):
    newTimeAsPosix = newTime.strftime('%Y-%m-%dT%H:%M:%S.%f')[:-3]
    modifyPFSubjectiveRestriction(nodeId, "BUILTIN-PREACTIVATE_FEATURE.json", "earliest_allowed_activation_time", newTimeAsPosix)

def enableOnlyLinkToExistingPermissionPFSpec(nodeId, enable=True):
    modifyPFSubjectiveRestriction(nodeId, "BUILTIN-ONLY_LINK_TO_EXISTING_PERMISSION.json", "enabled", enable)

def relaunchNode(node: Node, nodeId, chainArg="", addOrSwapFlags=None, relaunchAssertMessage="Fail to relaunch"):
    relaunchTimeout=5
    isRelaunchSuccess = node.relaunch(nodeId, chainArg=chainArg, addOrSwapFlags=addOrSwapFlags, timeout=relaunchTimeout, cachePopen=True)
    time.sleep(1) # Give a second to replay or resync if needed
    assert isRelaunchSuccess, relaunchAssertMessage
    return isRelaunchSuccess

def restartNode(node: Node, nodeId, chainArg="", addOrSwapFlags=None):
    if not node.killed:
        node.kill(signal.SIGTERM)
    relaunchNode(node, nodeId, chainArg, addOrSwapFlags)

def isFeatureActivated(node, featureDigest):
    latestBlockHeaderState = node.getLatestBlockHeaderState()
    activatedProcotolFeatures = latestBlockHeaderState["activated_protocol_features"]["protocol_features"]
    return featureDigest in activatedProcotolFeatures

# Setup cluster and it's wallet manager
walletMgr=WalletMgr(True)
cluster=Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

# List to contain the test result message
testResultMsgs = []
testSuccessful = False
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    # Start the cluster
    cluster.launch(extraNodeosArgs=" --plugin eosio::producer_api_plugin ",
                   specificExtraNodeosArgs={0:" -e "},
                   totalNodes=2,
                   prodCount=4,
                   useBiosBootFile=False,
                   pfSetupPolicy=PFSetupPolicy.NONE)
    # We don't need bios node anymore, just kill it
    cluster.biosNode.kill(signal.SIGTERM)
    producingNodeId = 0
    nonProducingNodeId = 1
    producingNode = cluster.getNode(producingNodeId)
    nonProducingNode = cluster.getNode(nonProducingNodeId)

    supportedProtocolFeatureDigestDict = producingNode.getSupportedProtocolFeatureDigestDict()

    def pauseBlockProduction():
        producingNode.sendRpcApi("v1/producer/pause")

    def resumeBlockProduction():
        producingNode.sendRpcApi("v1/producer/resume")

    ###############################################################
    # The following test case is testing PREACTIVATE_FEATURE with
    # earliest_allowed_activation_time subjective restriction
    ###############################################################

    preactivateFeatureDigest = supportedProtocolFeatureDigestDict["PREACTIVATE_FEATURE"]

    # Kill the nodes and then modify the JSON of PREACTIVATE FEATURE so it's far in the future
    modifyPreactFeatEarliestActivationTime(producingNodeId, datetime(2120, 1, 1))
    modifyPreactFeatEarliestActivationTime(nonProducingNodeId, datetime(2120, 1, 1))
    restartNode(producingNode, producingNodeId)
    restartNode(nonProducingNode, nonProducingNodeId)

    # Activating PREACTIVATE_FEATURE should fail
    try:
        producingNode.activatePreactivateFeature()
        Utils.errorExit("Preactivate Feature should Fail")
    except Exception as e:
        Utils.Print("Fail to activate PREACTIVATE_FEATURE as expected")

    # Modify back the earliest activation time for producing node
    modifyPreactFeatEarliestActivationTime(producingNodeId, datetime(1970, 1, 1))
    restartNode(producingNode, producingNodeId)

    # This time PREACTIVATE_FEATURE should be successfully activated on the producing node
    Utils.Print("Activating PREACTIVATE_FEATURE")
    producingNode.activatePreactivateFeature()
    producingNode.waitForHeadToAdvance()
    assert isFeatureActivated(producingNode, preactivateFeatureDigest),\
        "PREACTIVATE_FEATURE is not activated in producing node"

    # However, it should fail on the non producing node, because its earliest act time is in the future
    assert not isFeatureActivated(nonProducingNode, preactivateFeatureDigest),\
        "PREACTIVATE_FEATURE is activated on non producing node when it's supposed not to"

    # After modifying the earliest activation time, and it's syncing, it should be activated
    modifyPreactFeatEarliestActivationTime(nonProducingNodeId, datetime(1970, 1, 1))
    restartNode(nonProducingNode, nonProducingNodeId)
    assert isFeatureActivated(nonProducingNode, preactivateFeatureDigest),\
        "PREACTIVATE_FEATURE is not activated on non producing node"

    #####################################################################
    # The following test case is testing ONLY_LINK_TO_EXISTING_PERMISSION
    # with enabled subjective restriction
    #####################################################################

    onlyLinkToExistPermFeatDigest = supportedProtocolFeatureDigestDict["ONLY_LINK_TO_EXISTING_PERMISSION"]

    # First publish eosio contract
    trans = producingNode.publishContract("eosio", "unittests/contracts/eosio.bios",
                                          "eosio.bios.wasm", "eosio.bios.abi", waitForTransBlock=True)
    assert trans is not None, "Fail to publish eosio.bios contract"

    # Test ONLY_LINK_TO_EXISTING_PERMISSION when subjective_restrictions enabled is false
    enableOnlyLinkToExistingPermissionPFSpec(producingNodeId, False)
    restartNode(producingNode, producingNodeId)
    producingNode.preactivateProtocolFeatures([onlyLinkToExistPermFeatDigest])
    producingNode.waitForHeadToAdvance()
    assert not isFeatureActivated(producingNode, onlyLinkToExistPermFeatDigest),\
        "ONLY_LINK_TO_EXISTING_PERMISSION is activated on non producing node when it's supposed not to"

    # Modify the subjective_restriction enabled to be true, now it should be activated on both nodes
    enableOnlyLinkToExistingPermissionPFSpec(producingNodeId, True)
    restartNode(producingNode, producingNodeId)
    producingNode.preactivateProtocolFeatures([onlyLinkToExistPermFeatDigest])
    producingNode.waitForHeadToAdvance()
    assert isFeatureActivated(producingNode, onlyLinkToExistPermFeatDigest),\
        "ONLY_LINK_TO_EXISTING_PERMISSION is not activated on producing node"
    assert isFeatureActivated(nonProducingNode, onlyLinkToExistPermFeatDigest),\
        "ONLY_LINK_TO_EXISTING_PERMISSION is not activated on non producing node"

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)
    # Print test result
    for msg in testResultMsgs: Utils.Print(msg)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
