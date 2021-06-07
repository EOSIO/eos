#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from Cluster import PFSetupPolicy
from WalletMgr import WalletMgr
from Node import Node
from TestHelper import TestHelper

import os

###############################################################
# privacy_config_test_no_ca
#
# This test ensures that node with TLS setup but without CA configured
# can't participate security group
#
###############################################################

def makeNoCAArgs(index):
    participantName = Node.participantName(index+1)
    privacyDir=os.path.join(Utils.ConfigDir, "privacy")
    nodeCert = os.path.join(privacyDir, "{}.crt".format(participantName))
    nodeKey = os.path.join(privacyDir, "{}_key.pem".format(participantName))
    return "--p2p-tls-own-certificate-file {} --p2p-tls-private-key-file {}".format(nodeCert, nodeKey)

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

    Cluster.generateCertificates("privacy", totalNodes + 1)
    specificExtraNodeosArgs = {}
    specificExtraNodeosArgs[-1] = Cluster.getPrivacyArguments("privacy", totalNodes)
    specificExtraNodeosArgs[0]  = makeNoCAArgs(0)

    # configSecurityGroup is False because of we need castom setup and provide it via specificExtraNodeosArgs
    if not cluster.launch(pnodes=pnodes, 
                          totalNodes=totalNodes, 
                          configSecurityGroup=False, 
                          pfSetupPolicy=PFSetupPolicy.PREACTIVATE_FEATURE_ONLY, 
                          onlyBios=True,
                          loadSystemContract=False, 
                          specificExtraNodeosArgs=specificExtraNodeosArgs, 
                          printInfo=True):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    cluster.verifyInSync()

    prodNode = cluster.biosNode
    nonProdNode = cluster.getNode(0)

    feature = "SECURITY_GROUP"
    Utils.Print("Activating {} Feature".format(feature))
    allBuiltInFeatures = prodNode.getAllBuiltInFeaturesInfo()
    prodNode.preactivateProtocolFeatures([allBuiltInFeatures[feature]], failOnError=True)

    assert True == prodNode.waitForHeadToAdvance(blocksToAdvance=12, timeout=10)
    assert True == prodNode.containsFeatures({feature})

    # ensure non-prod node went out of sync
    assert False == nonProdNode.waitForHeadToAdvance(timeout=5)
    assert False == nonProdNode.containsFeatures({feature})

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)