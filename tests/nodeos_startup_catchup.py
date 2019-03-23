#!/usr/bin/env python3

from testUtils import Utils
import testUtils
import time
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import BlockType
from Node import Node
from TestHelper import AppArgs
from TestHelper import TestHelper

import decimal
import math
import re

###############################################################
# nodeos_startup_catchup
#  Test configures a producing node and <--txn-plugins count> non-producing nodes with the
#  txn_test_gen_plugin.  Each non-producing node starts generating transactions and sending them
#  to the producing node.
#  1) After 10 seconds a new node is started.
#  2) 10 seconds later, that node is checked to see if it has caught up to the producing node and
#     that node is killed and a new node is started.
#  3) Repeat step 2, <--catchup-count - 1> more times
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit

from core_symbol import CORE_SYMBOL

appArgs=AppArgs()
extraArgs = appArgs.add(flag="--catchup-count", type=int, help="How many catchup-nodes to launch", default=10)
extraArgs = appArgs.add(flag="--txn-gen-nodes", type=int, help="How many transaction generator nodes", default=2)
args = TestHelper.parse_args({"--prod-count","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run",
                              "-p","--p2p-plugin","--wallet-port"}, applicationSpecificArgs=appArgs)
Utils.Debug=args.v
pnodes=args.p if args.p > 0 else 1
startedNonProdNodes = args.txn_gen_nodes if args.txn_gen_nodes >= 2 else 2
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
prodCount=args.prod_count if args.prod_count > 1 else 2
killAll=args.clean_run
p2pPlugin=args.p2p_plugin
walletPort=args.wallet_port
catchupCount=args.catchup_count if args.catchup_count > 0 else 1
totalNodes=startedNonProdNodes+pnodes+catchupCount

walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName=Utils.EosWalletName
ClientName="cleos"

try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.setWalletMgr(walletMgr)

    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    specificExtraNodeosArgs={}
    txnGenNodeNum=pnodes  # next node after producer nodes
    for nodeNum in range(txnGenNodeNum, txnGenNodeNum+startedNonProdNodes):
        specificExtraNodeosArgs[nodeNum]="--plugin eosio::txn_test_gen_plugin --txn-test-gen-account-prefix txntestacct"
    Print("Stand up cluster")
    if cluster.launch(prodCount=prodCount, onlyBios=False, pnodes=pnodes, totalNodes=totalNodes, totalProducers=pnodes*prodCount, p2pPlugin=p2pPlugin,
                      useBiosBootFile=False, specificExtraNodeosArgs=specificExtraNodeosArgs, unstartedNodes=catchupCount, loadSystemContract=False) is False:
        Utils.errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    txnGenNodes=[]
    for nodeNum in range(txnGenNodeNum, txnGenNodeNum+startedNonProdNodes):
        txnGenNodes.append(cluster.getNode(nodeNum))

    txnGenNodes[0].txnGenCreateTestAccounts(cluster.eosioAccount.name, cluster.eosioAccount.activePrivateKey)
    time.sleep(20)

    for genNum in range(0, len(txnGenNodes)):
        salt="%d" % genNum
        txnGenNodes[genNum].txnGenStart(salt, 1500, 150)
        time.sleep(1)

    node0=cluster.getNode(0)

    def lib(node):
        return node.getBlockNum(BlockType.lib)

    def head(node):
        return node.getBlockNum(BlockType.head)

    time.sleep(10)
    retryCountMax=100
    for catchup_num in range(0, catchupCount):
        lastLibNum=lib(node0)
        lastHeadNum=head(node0)
        lastCatchupLibNum=None

        cluster.launchUnstarted(cachePopen=True)
        retryCount=0
        # verify that production node is advancing (sanity check)
        while lib(node0)<=lastLibNum:
            time.sleep(4)
            retryCount+=1
            # give it some more time if the head is still moving forward
            if retryCount>=20 or head(node0)<=lastHeadNum:
                Utils.errorExit("Node 0 failing to advance lib.  Was %s, now %s." % (lastLibNum, lib(node0)))
            if Utils.Debug: Utils.Print("Node 0 head was %s, now %s.  Waiting for lib to advance" % (lastLibNum, lib(node0)))
            lastHeadNum=head(node0)

        catchupNode=cluster.getNodes()[-1]
        time.sleep(9)
        lastCatchupLibNum=lib(catchupNode)
        lastCatchupHeadNum=head(catchupNode)
        retryCount=0
        while lib(catchupNode)<=lastCatchupLibNum:
            time.sleep(5)
            retryCount+=1
            # give it some more time if the head is still moving forward
            if retryCount>=100 or head(catchupNode)<=lastCatchupHeadNum:
                Utils.errorExit("Catchup Node %s failing to advance lib.  Was %s, now %s." %
                                (cluster.getNodes().index(catchupNode), lastCatchupLibNum, lib(catchupNode)))
            if Utils.Debug: Utils.Print("Catchup Node %s head was %s, now %s.  Waiting for lib to advance" % (cluster.getNodes().index(catchupNode), lastCatchupLibNum, lib(catchupNode)))
            lastCatchupHeadNum=head(catchupNode)

        retryCount=0
        lastLibNum=lib(node0)
        trailingLibNum=lastLibNum-lib(catchupNode)
        lastHeadNum=head(node0)
        libNotMovingCount=0
        while trailingLibNum>0:
            delay=5
            time.sleep(delay)
            libMoving=lib(catchupNode)>lastCatchupLibNum
            if libMoving:
                trailingLibNum=lastLibNum-lib(catchupNode)
                libNotMovingCount=0
            else:
                libNotMovingCount+=1
                if Utils.Debug and libNotMovingCount%10==0:
                    Utils.Print("Catchup node %s lib has not moved for %s seconds, lib is %s" %
                                (cluster.getNodes().index(catchupNode), (delay*libNotMovingCount), lib(catchupNode)))
            retryCount+=1
            # give it some more time if the head is still moving forward
            if retryCount>=retryCountMax or head(catchupNode)<=lastCatchupHeadNum or libNotMovingCount>100:
                Utils.errorExit("Catchup Node %s failing to advance lib along with node 0.  Catchup node lib is %s, node 0 lib is %s." %
                                (cluster.getNodes().index(catchupNode), lib(catchupNode), lastLibNum))
            if Utils.Debug: Utils.Print("Catchup Node %s head is %s, node 0 head is %s.  Waiting for lib to advance from %s to %s" % (cluster.getNodes().index(catchupNode), head(catchupNode), head(node0), lib(catchupNode), lastLibNum))
            lastCatchupHeadNum=head(catchupNode)

        catchupNode.interruptAndVerifyExitStatus(60)
        retryCountMax*=3

    testSuccessful=True

finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exit(0)
