#!/usr/bin/env python3

from testUtils import Utils
import testUtils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from TestHelper import TestHelper

import decimal
import math
import re

###############################################################
# nodeos_voting_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################
class ProducerToNode:
    map={}

    @staticmethod
    def populate(node, num):
        for prod in node.producers:
            ProducerToNode.map[prod]=num
            Utils.Print("Producer=%s for nodeNum=%s" % (prod,num))

def vote(node, account, producers):
    Print("Votes for %s" % (account.name))
    trans=node.vote(account, producers, waitForTransBlock=False)
    if trans is None:
        Utils.cmdError("voting with %s" % (account.name))
        errorExit("Failed to vote with account %s" % (account.name))
    return trans

def getBlockProducer(node, blockNum):
    node.waitForBlock(blockNum)
    block=node.getBlock(blockNum)
    if block is None:
        Utils.cmdError("could not get block number %s" % (blockNum))
        errorExit("Failed to get block")
    blockProducer=block["producer"]
    if blockProducer is None:
        Utils.cmdError("could not get producer for block number %s" % (blockNum))
        errorExit("Failed to get block's producer")
    return blockProducer

def getNodeNum(cluster, node):
    for i in range(0, 4):
        if node == cluster.getNode(i):
            return i
    return -1

def isValidBlockProducer(prodsActive, blockNum, node):
    blockProducer=getBlockProducer(node, blockNum)
    if blockProducer not in prodsActive:
        return False
    return prodsActive[blockProducer]

def validBlockProducer(prodsActive, prodsSeen, blockNum, node):
    blockProducer=getBlockProducer(node, blockNum)
    if blockProducer not in prodsActive:
        Utils.cmdError("unexpected block producer %s at blockNum=%s" % (blockProducer,blockNum))
        errorExit("Failed because of invalid block producer")
    if not prodsActive[blockProducer]:
        Utils.cmdError("block producer %s for blockNum=%s not elected, belongs to node %s" % (blockProducer, blockNum, ProducerToNode.map[blockProducer]))
        errorExit("Failed because of incorrect block producer")
    prodsSeen[blockProducer]=True

def getNextCleanProductionCycle(trans, node):
    transId=Node.getTransId(trans)
    rounds=21*12*2  # max time to ensure that at least 2/3+1 of producers x blocks per producer x at least 2 times
    node.waitForTransFinalization(transId, timeout=rounds/2)
    irreversibleBlockNum=node.getIrreversibleBlockNum()

    # The voted schedule should be promoted now, then need to wait for that to become irreversible
    votingTallyWindow=120  #could be up to 120 blocks before the votes were tallied
    promotedBlockNum=node.getHeadBlockNum()+votingTallyWindow
    node.waitForIrreversibleBlock(promotedBlockNum, timeout=rounds/2)

    ibnSchedActive=node.getIrreversibleBlockNum()

    blockNum=node.getHeadBlockNum()
    Utils.Print("Searching for clean production cycle blockNum=%s ibn=%s  transId=%s  promoted bn=%s  ibn for schedule active=%s" % (blockNum,irreversibleBlockNum,transId,promotedBlockNum,ibnSchedActive))
    blockProducer=getBlockProducer(node, blockNum)
    blockNum+=1
    Utils.Print("Advance until the next block producer is retrieved")
    while blockProducer == getBlockProducer(node, blockNum):
        blockNum+=1

    blockProducer=getBlockProducer(node, blockNum)
    return blockNum

def setActiveProducers(prodsActive, activeProducers):
    for prod in prodsActive:
        prodsActive[prod]=prod in activeProducers

def verifyProductionRounds(trans, node, prodsActive, rounds):
    blockNum=getNextCleanProductionCycle(trans, node)
    Utils.Print("Validating blockNum=%s" % (blockNum))

    temp=Utils.Debug
    Utils.Debug=False
    Utils.Print("FIND VALID BLOCK PRODUCER")
    blockProducer=getBlockProducer(node, blockNum)
    lastBlockProducer=blockProducer
    adjust=False
    while not isValidBlockProducer(prodsActive, blockNum, node):
        adjust=True
        blockProducer=getBlockProducer(node, blockNum)
        if lastBlockProducer!=blockProducer:
            Utils.Print("blockProducer=%s for blockNum=%s is for node=%s" % (blockProducer, blockNum, ProducerToNode.map[blockProducer]))
        lastBlockProducer=blockProducer
        blockNum+=1

    Utils.Print("VALID BLOCK PRODUCER")
    saw=0
    sawHigh=0
    startingFrom=blockNum
    doPrint=0
    invalidCount=0
    while adjust:
        invalidCount+=1
        if lastBlockProducer==blockProducer:
            saw+=1;
        else:
            if saw>=12:
                startingFrom=blockNum
                if saw>12:
                    Utils.Print("ERROR!!!!!!!!!!!!!!      saw=%s, blockProducer=%s, blockNum=%s" % (saw,blockProducer,blockNum))
                break
            else:
                if saw > sawHigh:
                    sawHigh = saw
                    Utils.Print("sawHigh=%s" % (sawHigh))
                if doPrint < 5:
                    doPrint+=1
                    Utils.Print("saw=%s, blockProducer=%s, blockNum=%s" % (saw,blockProducer,blockNum))
                lastBlockProducer=blockProducer
                saw=1
        blockProducer=getBlockProducer(node, blockNum)
        blockNum+=1

    if adjust:
        blockNum-=1

    Utils.Print("ADJUSTED %s blocks" % (invalidCount-1))

    prodsSeen=None
    Utils.Print("Verify %s complete rounds of all producers producing" % (rounds))
    for i in range(0, rounds):
        prodsSeen={}
        lastBlockProducer=None
        for j in range(0, 21):
            # each new set of 12 blocks should have a different blockProducer 
            if lastBlockProducer is not None and lastBlockProducer==getBlockProducer(node, blockNum):
                Utils.cmdError("expected blockNum %s to be produced by any of the valid producers except %s" % (blockNum, lastBlockProducer))
                errorExit("Failed because of incorrect block producer order")

            # make sure that the next set of 12 blocks all have the same blockProducer
            lastBlockProducer=getBlockProducer(node, blockNum)
            for k in range(0, 12):
                validBlockProducer(prodsActive, prodsSeen, blockNum, node1)
                blockProducer=getBlockProducer(node, blockNum)
                if lastBlockProducer!=blockProducer:
                    printStr=""
                    newBlockNum=blockNum-18
                    for l in range(0,36):
                        printStr+="%s" % (newBlockNum)
                        printStr+=":"
                        newBlockProducer=getBlockProducer(node, newBlockNum)
                        printStr+="%s" % (newBlockProducer)
                        printStr+="  "
                        newBlockNum+=1
                    Utils.cmdError("expected blockNum %s (started from %s) to be produced by %s, but produded by %s: round=%s, prod slot=%s, prod num=%s - %s" % (blockNum, startingFrom, lastBlockProducer, blockProducer, i, j, k, printStr))
                    errorExit("Failed because of incorrect block producer order")
                blockNum+=1

    # make sure that we have seen all 21 producers
    prodsSeenKeys=prodsSeen.keys()
    if len(prodsSeenKeys)!=21:
        Utils.cmdError("only saw %s producers of expected 21. At blockNum %s only the following producers were seen: %s" % (len(prodsSeenKeys), blockNum, ",".join(prodsSeenKeys)))
        errorExit("Failed because of missing block producers")

    Utils.Debug=temp


Print=Utils.Print
errorExit=Utils.errorExit

from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--prod-count","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"})
Utils.Debug=args.v
totalNodes=4
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
prodCount=args.prod_count
killAll=args.clean_run

walletMgr=WalletMgr(True)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName="keosd"
ClientName="cleos"

try:
    TestHelper.printSystemInfo("BEGIN")

    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    Print("Stand up cluster")
    if cluster.launch(prodCount=prodCount, onlyBios=False, dontKill=dontKill, pnodes=totalNodes, totalNodes=totalNodes, totalProducers=totalNodes*21) is False:
        Utils.cmdError("launcher")
        errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    accounts=cluster.createAccountKeys(5)
    if accounts is None:
        errorExit("FAILURE - create keys")
    accounts[0].name="tester111111"
    accounts[1].name="tester222222"
    accounts[2].name="tester333333"
    accounts[3].name="tester444444"
    accounts[4].name="tester555555"

    testWalletName="test"

    Print("Creating wallet \"%s\"." % (testWalletName))
    walletMgr.killall(allInstances=killAll)
    walletMgr.cleanup()
    if walletMgr.launch() is False:
        Utils.cmdError("%s" % (WalletdName))
        errorExit("Failed to stand up eos walletd.")

    testWallet=walletMgr.create(testWalletName, [cluster.eosioAccount,accounts[0],accounts[1],accounts[2],accounts[3],accounts[4]])
    if testWallet is None:
        Utils.cmdError("eos wallet create")
        errorExit("Failed to create wallet %s." % (testWalletName))

    for _, account in cluster.defProducerAccounts.items():
        walletMgr.importKey(account, testWallet, ignoreDupKeyWarning=True)

    Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))

    for i in range(0, totalNodes):
        node=cluster.getNode(i)
        node.producers=Cluster.parseProducers(i)
        for prod in node.producers:
            trans=node.regproducer(cluster.defProducerAccounts[prod], "http::/mysite.com", 0, waitForTransBlock=False)
            if trans is None:
                Utils.cmdError("registering producer %s" % (prod.name))
                errorExit("Failed registering producer %s" % (prod.name))

    node0=cluster.getNode(0)
    if node0 is None:
        errorExit("Cluster in bad state, received None node")
    node1=cluster.getNode(1)
    if node1 is None:
        errorExit("Cluster in bad state, received None node")
    node2=cluster.getNode(2)
    if node2 is None:
        errorExit("Cluster in bad state, received None node")
    node3=cluster.getNode(3)
    if node3 is None:
        errorExit("Cluster in bad state, received None node")

    node=node0
    # create accounts via eosio as otherwise a bid is needed
    for account in accounts:
        Print("Create new account %s via %s" % (account.name, cluster.eosioAccount.name))
        trans=node.createInitializeAccount(account, cluster.eosioAccount, stakedDeposit=0, waitForTransBlock=False, stakeNet=1000, stakeCPU=1000, buyRAM=1000)
        if trans is None:
            Utils.cmdError("%s create account" % (account.name))
            errorExit("Failed to create account %s" % (account.name))
        transferAmount="100000000.0000 {0}".format(CORE_SYMBOL)
        Print("Transfer funds %s from account %s to %s" % (transferAmount, cluster.eosioAccount.name, account.name))
        if node.transferFunds(cluster.eosioAccount, account, transferAmount, "test transfer") is None:
            errorExit("Failed to transfer funds %d from account %s to %s" % (
                transferAmount, cluster.eosioAccount.name, account.name))
        trans=node.delegatebw(account, 20000000.0000, 20000000.0000) 
        if trans is None:
            Utils.cmdError("delegate bandwidth for %s" % (account.name))
            errorExit("Failed to delegate bandwidth for %s" % (account.name))

    # containers for tracking producers
    prodsActive={}
    for i in range(0, 4):
        node=cluster.getNode(i)
        ProducerToNode.populate(node, i)
        for prod in node.producers:
            prodsActive[prod]=False

    #first account will vote for node0 producers, all others will vote for node1 producers
    node=node0
    for account in accounts:
        trans=vote(node, account, node.producers)
        node=node1

    setActiveProducers(prodsActive, node1.producers)

    verifyProductionRounds(trans, node2, prodsActive, 2)

    # test shifting all 21 away from one node to another
    # first account will vote for node2 producers, all others will vote for node3 producers
    node1
    for account in accounts:
        trans=vote(node, account, node.producers)
        node=node2

    setActiveProducers(prodsActive, node2.producers)

    verifyProductionRounds(trans, node1, prodsActive, 2)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
