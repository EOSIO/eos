#!/usr/bin/env python3

from testUtils import Utils
import testUtils
import time
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from TestHelper import AppArgs
from TestHelper import TestHelper

import decimal
import math
import re

###############################################################
# nodeos_voting_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################
Print=Utils.Print

from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--prod-count","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run","--p2p-plugin"})
Utils.Debug=args.v
totalProducerNodes=21
totalNonProducerNodes=1
totalNodes=totalProducerNodes+totalNonProducerNodes
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
prodCount=args.prod_count
killAll=args.clean_run
p2pPlugin=args.p2p_plugin

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
    specificExtraNodeosArgs={}
    # producer nodes will be mapped to 0 through totalProducerNodes-1, so totalProducerNodes will be the non-producing node
    specificExtraNodeosArgs[totalProducerNodes]="--plugin eosio::test_control_api_plugin"
    if cluster.launch(prodCount=prodCount, onlyBios=False, dontKill=dontKill, topo="bridge", pnodes=totalProducerNodes,
                      totalNodes=totalNodes, totalProducers=totalProducerNodes, p2pPlugin=p2pPlugin,
                      specificExtraNodeosArgs=specificExtraNodeosArgs) is False:
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")

    # "bridge" shape connects defprocera through defproducerk to each other and defproducerl through defproduceru and the only
    # connection between those 2 groups is through the bridge node

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    accounts=cluster.createAccountKeys(5)
    if accounts is None:
        Utils.errorExit("FAILURE - create keys")
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
        Utils.errorExit("Failed to stand up eos walletd.")

    testWallet=walletMgr.create(testWalletName, [cluster.eosioAccount,accounts[0],accounts[1],accounts[2],accounts[3],accounts[4]])

    for _, account in cluster.defProducerAccounts.items():
        walletMgr.importKey(account, testWallet, ignoreDupKeyWarning=True)

    Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))

    nonProdNode=None
    prodNodes=[]
    producers={}
    for i in range(0, totalNodes):
        node=cluster.getNode(i)
        node.producers=Cluster.parseProducers(i)
        numProducers=len(node.producers)
        if numProducers==1:
            prod=node.producers[0]
            trans=node.regproducer(cluster.defProducerAccounts[prod], "http::/mysite.com", 0, waitForTransBlock=False, exitOnError=True)
            prodNodes.append(node)
            producers[prod]=node
        elif numProducers==0:
            if nonProdNode is None:
                nonProdNode=node
            else:
                Utils.errorExit("More than one non-producing nodes")
        else:
            Utils.errorExit("Producing node should have 1 producer, it has %d" % (numProducers))

    node=prodNodes[0]
    # create accounts via eosio as otherwise a bid is needed
    for account in accounts:
        Print("Create new account %s via %s" % (account.name, cluster.eosioAccount.name))
        trans=node.createInitializeAccount(account, cluster.eosioAccount, stakedDeposit=0, waitForTransBlock=False, stakeNet=1000, stakeCPU=1000, buyRAM=1000, exitOnError=True)
        transferAmount="100000000.0000 {0}".format(CORE_SYMBOL)
        Print("Transfer funds %s from account %s to %s" % (transferAmount, cluster.eosioAccount.name, account.name))
        node.transferFunds(cluster.eosioAccount, account, transferAmount, "test transfer")
        trans=node.delegatebw(account, 20000000.0000, 20000000.0000, exitOnError=True)

    #verify nodes are in sync and advancing
    cluster.waitOnClusterSync(blockAdvancing=5)    
    index=0
    for account in accounts:
        trans=prodNodes[index].vote(account, producers)
        index+=1

    #verify nodes are in sync and advancing
    cluster.waitOnClusterSync(blockAdvancing=5)    
    blockNum=node.getNextCleanProductionCycle(trans)
    blockProducer=node.getBlockProducer(blockNum)
    Utils.Print("Validating blockNum=%s, producer=%s" % (blockNum, blockProducer))

    lastBlockProducer=blockProducer
    while blockProducer==lastBlockProducer:
        blockNum+=1
        blockProducer=node.getBlockProducer(blockNum)

    productionCycle=[]
    producerToSlot={}
    slot=-1
    expectedCount=12
    while True:
        if blockProducer not in producers:
            Utils.errorExit("Producer %s was not one of the voted on producers" % blockProducer)

        productionCycle.append(blockProducer)
        slot+=1
        if blockProducer in producerToSlot:
            Utils.errorExit("Producer %s was first seen in slot %d, but is repeated in slot %d" % (blockProducer, producerToSlot[blockProducer], slot))

        producerToSlot[blockProducer]={"slot":slot, "count":0}
        lastBlockProducer=blockProducer
        while blockProducer==lastBlockProducer:
            producerToSlot[blockProducer]["count"]+=1
            blockNum+=1
            blockProducer=node.getBlockProducer(blockNum)

        if producerToSlot[lastBlockProducer]["count"]!=expectedCount:
            Utils.errorExit("Producer %s, in slot %d, expected to produce %d blocks but produced %d blocks" % (blockProducer, expectedCount, producerToSlot[lastBlockProducer]["count"]))

        if blockProducer==productionCycle[0]:
            break

    output=None
    for blockProducer in productionCycle:
        if output is None:
            output=""
        else:
            output+=", "
        output+=blockProducer+":"+str(producerToSlot[blockProducer]["count"])
    Utils.Print("ProductionCycle ->> {\n%s\n}" % output)

    for prodNode in prodNodes:
        prodNode.getInfo()

    cluster.reportStatus()

    Utils.Print("Sending command to kill \"bridge\" node to separate the 2 producer groups.")
    nonProdNode.killNodeOnProducer(producer="defproducerl", whereInSequence=0)

    for prodNode in prodNodes:
        prodNode.getInfo()

    cluster.reportStatus()

    time.sleep(60)
    assert(not nonProdNode.verifyAlive())
    for prodNode in prodNodes:
        prodNode.getInfo()

    cluster.reportStatus()

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
