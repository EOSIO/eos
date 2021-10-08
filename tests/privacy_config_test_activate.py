#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from Cluster import PFSetupPolicy
from WalletMgr import WalletMgr
from TestHelper import TestHelper

###############################################################
# privacy_config1_test
#
# This test ensures that node can't activate security groups feature if it
# doesn't have ssl + CA certificate assigned
#
###############################################################


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

    if not cluster.launch(pnodes=pnodes, 
                          totalNodes=totalNodes, 
                          configSecurityGroup=False, 
                          onlyBios=True, 
                          loadSystemContract=False, 
                          pfSetupPolicy=PFSetupPolicy.PREACTIVATE_FEATURE_ONLY,
                          printInfo=True):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    feature = "SECURITY_GROUP"
    Utils.Print("Trying to activate {} feature. Expected to fail".format(feature))
    allBuiltInFeatures = cluster.biosNode.getAllBuiltInFeaturesInfo()
    cluster.biosNode.preactivateProtocolFeatures([allBuiltInFeatures[feature]])
    Print("****************************")
    Print("^^^ ERROR ABOVE IS EXPECTED!")
    Print("****************************")
    assert True == cluster.biosNode.waitForHeadToAdvance(blocksToAdvance=12, timeout=10)
    assert False == cluster.biosNode.containsFeatures({feature})

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)