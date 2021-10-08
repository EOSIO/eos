#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper

import signal

###############################################################
# privacy_config_test_restart
#
# This test ensures that node can't restart without ssl configuration
# if privacy feature was activated previously
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

    if not cluster.launch(pnodes=pnodes, totalNodes=totalNodes, configSecurityGroup=True, onlyBios=True, printInfo=True):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    cluster.biosNode.kill(signal.SIGTERM)

    # ensure node restarts successfully
    cluster.biosNode.relaunch(timeout=15, cachePopen=True)
    cluster.biosNode.waitForLibToAdvance()
    cluster.biosNode.kill(signal.SIGTERM)

    deleteFlags={"--p2p-tls-own-certificate-file" : True, 
                 "--p2p-tls-private-key-file" : True, 
                 "--p2p-tls-security-group-ca-file" : True}
    # ensure node fails to restart if tls flags removed
    if False == cluster.biosNode.relaunch(deleteFlags=deleteFlags, timeout=15, cachePopen=True):
        Print("****************************")
        Print("^^^ ERROR ABOVE IS EXPECTED!")
        Print("****************************")
        Print("SUCCESS: cluster failed to restart without ssl command line parameters")
    else:
        assert False, "FAIL: Cluster was not supposed to restart succesfully"

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)