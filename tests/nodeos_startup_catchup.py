#!/usr/bin/env python3

from testUtils import Utils
import testUtils
import time
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import BlockType
from Node import Node
import signal
from TestHelper import AppArgs
from TestHelper import TestHelper

import decimal
import math
import re

###############################################################
# nodeos_startup_catchup
#
#  Test configures a producing node and <--txn-plugins count> non-producing nodes with the
#  txn_test_gen_plugin.  Each non-producing node starts generating transactions and sending them
#  to the producing node.
#  1) After 10 seconds a new node is started.
#  2) the node is allowed to catch up to the producing node
#  3) that node is killed
#  4) restart the node
#  5) the node is allowed to catch up to the producing node
#  3) Repeat steps 2-5, <--catchup-count - 1> more times
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit

from core_symbol import CORE_SYMBOL

appArgs=AppArgs()
extraArgs = appArgs.add(flag="--catchup-count", type=int, help="How many catchup-nodes to launch", default=10)
extraArgs = appArgs.add(flag="--txn-gen-nodes", type=int, help="How many transaction generator nodes", default=2)
args = TestHelper.parse_args({"--prod-count","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run",
                              "-p","--wallet-port"}, applicationSpecificArgs=appArgs)
Utils.Debug=args.v
pnodes=args.p if args.p > 0 else 1
startedNonProdNodes = args.txn_gen_nodes if args.txn_gen_nodes >= 2 else 2
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
prodCount=args.prod_count if args.prod_count > 1 else 2
killAll=args.clean_run
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
    if cluster.launch(prodCount=prodCount, onlyBios=False, pnodes=pnodes, totalNodes=totalNodes, totalProducers=pnodes*prodCount,
                      useBiosBootFile=False, specificExtraNodeosArgs=specificExtraNodeosArgs, unstartedNodes=catchupCount, loadSystemContract=False) is False:
        Utils.errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    Print("Create txn generate nodes")
    txnGenNodes=[]
    for nodeNum in range(txnGenNodeNum, txnGenNodeNum+startedNonProdNodes):
        txnGenNodes.append(cluster.getNode(nodeNum))

    Print("Create accounts for generated txns")
    txnGenNodes[0].txnGenCreateTestAccounts(cluster.eosioAccount.name, cluster.eosioAccount.activePrivateKey)

    def lib(node):
        return node.getBlockNum(BlockType.lib)

    def head(node):
        return node.getBlockNum(BlockType.head)

    def waitForBlock(node, blockNum, blockType=BlockType.head, timeout=None, reportInterval=20):
        if not node.waitForBlock(blockNum, timeout=timeout, blockType=blockType, reportInterval=reportInterval):
            info=node.getInfo()
            headBlockNum=info["head_block_num"]
            libBlockNum=info["last_irreversible_block_num"]
            Utils.errorExit("Failed to get to %s block number %d. Last had head block number %d and lib %d" % (blockType, blockNum, headBlockNum, libBlockNum))

    def waitForNodeStarted(node):
        sleepTime=0
        while sleepTime < 10 and node.getInfo(silentErrors=True) is None:
            time.sleep(1)
            sleepTime+=1

    node0=cluster.getNode(0)

    Print("Wait for account creation to be irreversible")
    blockNum=head(node0)
    waitForBlock(node0, blockNum, blockType=BlockType.lib)

    Print("Startup txn generation")
    period=1500
    transPerPeriod=150
    for genNum in range(0, len(txnGenNodes)):
        salt="%d" % genNum
        txnGenNodes[genNum].txnGenStart(salt, period, transPerPeriod)
        time.sleep(1)

    blockNum=head(node0)
    timePerBlock=500
    blocksPerPeriod=period/timePerBlock
    transactionsPerBlock=transPerPeriod/blocksPerPeriod
    steadyStateWait=20
    startBlockNum=blockNum+steadyStateWait
    numBlocks=20
    endBlockNum=startBlockNum+numBlocks
    waitForBlock(node0, endBlockNum)
    transactions=0
    avg=0
    for blockNum in range(startBlockNum, endBlockNum):
        block=node0.getBlock(blockNum)
        transactions+=len(block["transactions"])

    avg=transactions / (blockNum - startBlockNum + 1)

    Print("Validate transactions are generating")
    minRequiredTransactions=transactionsPerBlock
    assert avg>minRequiredTransactions, "Expected to at least receive %s transactions per block, but only getting %s" % (minRequiredTransactions, avg)

    Print("Cycle through catchup scenarios")
    twoRounds=21*2*12
    twoRoundsTimeout=(twoRounds/2 + 10)  #2 rounds in seconds + some leeway
    for catchup_num in range(0, catchupCount):
        Print("Start catchup node")
        cluster.launchUnstarted(cachePopen=True)
        lastLibNum=lib(node0)
        # verify producer lib is still advancing
        waitForBlock(node0, lastLibNum+1, timeout=twoRoundsTimeout, blockType=BlockType.lib)

        catchupNode=cluster.getNodes()[-1]
        catchupNodeNum=cluster.getNodes().index(catchupNode)
        waitForNodeStarted(catchupNode)
        lastCatchupLibNum=lib(catchupNode)

        Print("Verify catchup node %s's LIB is advancing" % (catchupNodeNum))
        # verify lib is advancing (before we wait for it to have to catchup with producer)
        waitForBlock(catchupNode, lastCatchupLibNum+1, timeout=twoRoundsTimeout, blockType=BlockType.lib)

        Print("Verify catchup node is advancing to producer")
        numBlocksToCatchup=(lastLibNum-lastCatchupLibNum-1)+twoRounds
        waitForBlock(catchupNode, lastLibNum, timeout=twoRoundsTimeout, blockType=BlockType.lib)

        Print("Shutdown catchup node and validate exit code")
        catchupNode.interruptAndVerifyExitStatus(60)

        Print("Restart catchup node")
        catchupNode.relaunch(catchupNodeNum, cachePopen=True)
        waitForNodeStarted(catchupNode)
        lastCatchupLibNum=lib(catchupNode)

        Print("Verify catchup node is advancing")
        # verify catchup node is advancing to producer
        waitForBlock(catchupNode, lastCatchupLibNum+1, timeout=twoRoundsTimeout, blockType=BlockType.lib)

        Print("Verify producer is still advancing LIB")
        lastLibNum=lib(node0)
        # verify producer lib is still advancing
        node0.waitForBlock(lastLibNum+1, timeout=twoRoundsTimeout, blockType=BlockType.lib)

        Print("Verify catchup node is advancing to producer")
        # verify catchup node is advancing to producer
        waitForBlock(catchupNode, lastLibNum, timeout=(numBlocksToCatchup/2 + 60), blockType=BlockType.lib)
        catchupNode.interruptAndVerifyExitStatus(60)
        catchupNode.popenProc=None

    testSuccessful=True

finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exit(0)
