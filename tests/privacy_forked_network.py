#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import BlockType
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper

import decimal
import re
import signal
import json
import os
import time

###############################################################
# privacy_forked_network
#
# Implements Privacy Test Case #4. It starts up 4+ producers in a mesh network. It adds 2/3+1 of
# the producers (all but 1) to the security group and verifies they are in sync and verifies the
# the remaining producers are not in sync with the other producers, but are in sync with each
# other. It then adds all the producers to the security group and then verifies that they are
# eventually in sync.
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit
cmdError=Utils.cmdError
from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--port","-p","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"
                              ,"--wallet-port"})
port=args.port
minTotalPnodes=4
pnodes=args.p if args.p > minTotalPnodes else minTotalPnodes
totalNodes=pnodes

Utils.Debug=args.v
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
onlyBios=False
killAll=args.clean_run
walletPort=args.wallet_port

cluster=Cluster(host=TestHelper.LOCAL_HOST, port=port, walletd=True)
walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName=Utils.EosWalletName
ClientName="cleos"
timeout = .5 * 12 * pnodes + 60 # time for finalization with x producer + 60 seconds padding
Utils.setIrreversibleTimeout(timeout)

try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.setWalletMgr(walletMgr)
    Print("SERVER: {}".format(TestHelper.LOCAL_HOST))
    Print("PORT: {}".format(port))

    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    Print("Stand up cluster")

    if cluster.launch(pnodes=pnodes, totalNodes=totalNodes, prodCount=1, onlyBios=False, configSecurityGroup=True) is False:
        cmdError("launcher")
        errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    Utils.Print("\n\n\n\n\nNext Round of Info:")
    cluster.reportInfo()

    cluster.biosNode.kill(signal.SIGTERM)

    producers = [cluster.getNode(x) for x in range(pnodes) ]

    securityGroup = cluster.getSecurityGroup()
    cluster.reportInfo()


    producerOrder = []
    node = producers[0]
    head = node.getBlockNum()
    start = head - int(12 * pnodes / 2)
    lastProd = node.getBlockProducerByNum(start)
    nextProd = None
    blockNum = start
    while True:
        prod = node.getBlockProducerByNum(blockNum)
        if prod != lastProd:
            Utils.Print("Started looking for producers at block {}, stopped at {}. Will skip producer: {} and send for producer: {}".format(start, blockNum, lastProd, prod))
            nextProd = prod
            # reset to use to determine
            start = blockNum
            break
        assert blockNum != head, "Should have found a new producer between blocks: {} and {}".format(start, head)
        blockNum += 1

    selectedProducers = None
    for producer in producers:
        if lastProd in producer.getProducers():
            selectedProducers = [x for x in producers if x != producer]
            break

    # wait for a whole cycle + an extra producer (in case we just missed the start)
    assert producers[0].waitForProducer(nextProd, atStart=True, timeout=12*(pnodes+1)/2) != None, ""

    Utils.Print("Add all producers except 1 to security group")
    securityGroup.editSecurityGroup(selectedProducers)
    securityGroup.verifySecurityGroup()

    cluster.reportInfo()

    Utils.Print("Add the last producer to the security group")
    securityGroup.moveToSecurityGroup()
    securityGroup.verifySecurityGroup()

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
