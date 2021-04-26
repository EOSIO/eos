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
# privacy_simple_network
#
# Implements Privacy Test Case #2 (and other misc scenarios). It creates a simple network of mesh connected
# producers and non-producer nodes. It adds the producers to the security group and verifies they are in
# sync and the non-producers are not.  Then, one by one it adds the non-producing nodes to the security
# group, and verifies that the correct nodes are in sync and the others are not. It also repeatedly changes
# the security group, not letting it finalize, to verify Test Case #2.
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit
cmdError=Utils.cmdError
from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--port","-p","-n","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"
                              ,"--wallet-port"})
port=args.port
pnodes=args.p
apiNodes=0        # minimum number of apiNodes that will be used in this test
# bios is also treated as an API node, but doesn't count against totalnodes count
minTotalNodes=pnodes+apiNodes
totalNodes=args.n if args.n >= minTotalNodes else minTotalNodes
if totalNodes > minTotalNodes:
    apiNodes += totalNodes - minTotalNodes

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
timeout = .5 * 12 * pnodes + 60 # time for finalization with 1 producer + 60 seconds padding
Utils.setIrreversibleTimeout(timeout)

try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.setWalletMgr(walletMgr)
    Print("SERVER: {}".format(TestHelper.LOCAL_HOST))
    Print("PORT: {}".format(port))

    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    Print("Stand up cluster")

    # adjust prodCount to ensure that lib trails more than 1 block behind head
    prodCount = 1 if pnodes > 1 else 2

    if cluster.launch(pnodes=pnodes, totalNodes=totalNodes, prodCount=prodCount, onlyBios=False, configSecurityGroup=True) is False:
        cmdError("launcher")
        errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    Utils.Print("\n\n\n\n\nNext Round of Info:")
    cluster.reportInfo()

    producers = [cluster.getNode(x) for x in range(pnodes) ]
    apiNodes = [cluster.getNode(x) for x in range(pnodes, totalNodes)]
    apiNodes.append(cluster.biosNode)
    Utils.Print("producer participants: [{}]".format(", ".join([x.getParticipant() for x in producers])))
    Utils.Print("api participants: [{}]".format(", ".join([x.getParticipant() for x in apiNodes])))

    securityGroup = cluster.getSecurityGroup()
    cluster.reportInfo()

    Utils.Print("Add all producers to security group")
    securityGroup.editSecurityGroup([cluster.getNodes()[x] for x in range(pnodes)])
    securityGroup.verifySecurityGroup()

    cluster.reportInfo()

    Utils.Print("One by one, add each API Node to the security group")
    # one by one add each nonParticipant to the security group
    while len(securityGroup.nonParticipants) > 0:
        securityGroup.moveToSecurityGroup()
        securityGroup.verifySecurityGroup()
        cluster.reportInfo()


    removeTrans = None
    Utils.Print("One by one, remove each API Node from the security group")
    # one by one remove each (original) nonParticipant from the security group
    while len(securityGroup.participants) > pnodes:
        removeTrans = securityGroup.removeFromSecurityGroup()
        securityGroup.verifySecurityGroup()
        cluster.reportInfo()


    # if we have more than 1 api node, we will add and remove all those nodes in bulk, if not it is just a repeat of the above test
    if len(apiNodes) > 1:
        Utils.Print("Add all api nodes to security group at the same time")
        securityGroup.editSecurityGroup(addNodes=securityGroup.nonParticipants)
        securityGroup.verifySecurityGroup()

        cluster.reportInfo()

        # waiting for a block to change (2 blocks since transaction indication could be 1 behind) to prevent duplicate remove transactions
        removeBlockNum = Node.getTransBlockNum(removeTrans)
        securityGroup.defaultNode.waitForBlock(removeBlockNum + 2)

        # alternate adding/removing participants to ensure the security group doesn't change
        initialBlockNum = None
        blockNum = None
        def is_done():
            # want to ensure that we can identify the range of libs the security group was changed in
            return blockNum - initialBlockNum > 12

        done = False
        # keep adding and removing nodes till we are done
        while not done:
            preInfo = securityGroup.defaultNode.getInfo()
            # waiting for a block change to prevent duplicate publish transactions
            securityGroup.defaultNode.waitForNextBlock()
            postInfo = securityGroup.defaultNode.getInfo()
            Utils.Print("Pre-info: \n{}\n\nPost-info: \n{}\n".format(json.dumps(preInfo, indent=4, sort_keys=True), json.dumps(postInfo, indent=4, sort_keys=True)))

            while not done and len(securityGroup.participants) > pnodes:
                publishTrans = securityGroup.removeFromSecurityGroup()
                blockNum = Node.getTransBlockNum(publishTrans)
                if initialBlockNum is None:
                    initialBlockNum = blockNum
                done = is_done()

            while not done and len(securityGroup.nonParticipants) > 0:
                publishTrans = securityGroup.moveToSecurityGroup()
                blockNum = Node.getTransBlockNum(publishTrans)
                done = is_done()

        Utils.Print("First adjustment to security group was in block num: {}, verifying no changes till block num: {} is finalized".format(initialBlockNum, blockNum))
        securityGroup.verifySecurityGroup()

        cluster.reportInfo()

        if len(securityGroup.nonParticipants) > 0:
            Utils.Print("Add all remaining non-participants to security group at the same time, so all api nodes can be removed as one group")
            securityGroup.editSecurityGroup(addNodes=securityGroup.nonParticipants)
            securityGroup.verifySecurityGroup()

            cluster.reportInfo()

        Utils.Print("Remove all api nodes from security group at the same time")
        securityGroup.editSecurityGroup(removeNodes=apiNodes)
        securityGroup.verifySecurityGroup()

        cluster.reportInfo()

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
