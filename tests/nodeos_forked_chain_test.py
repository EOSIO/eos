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
import signal

###############################################################
# nodeos_voting_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################
Print=Utils.Print

from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--prod-count","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run","--p2p-plugin"})
Utils.Debug=args.v
totalProducerNodes=2
totalNonProducerNodes=1
totalNodes=totalProducerNodes+totalNonProducerNodes
maxActiveProducers=21
totalProducers=maxActiveProducers
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
                      totalNodes=totalNodes, totalProducers=totalProducers, p2pPlugin=p2pPlugin,
                      useBiosBootFile=False, specificExtraNodeosArgs=specificExtraNodeosArgs) is False:
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
    producers=[]
    for i in range(0, totalNodes):
        node=cluster.getNode(i)
        node.producers=Cluster.parseProducers(i)
        numProducers=len(node.producers)
        Print("node has producers=%s" % (node.producers))
        if numProducers==0:
            if nonProdNode is None:
                nonProdNode=node
                nonProdNode.nodeNum=i
            else:
                Utils.errorExit("More than one non-producing nodes")
        else:
            for prod in node.producers:
                trans=node.regproducer(cluster.defProducerAccounts[prod], "http::/mysite.com", 0, waitForTransBlock=False, exitOnError=True)

            prodNodes.append(node)
            producers.extend(node.producers)

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
        Print("Vote for producers=%s" % (producers))
        trans=prodNodes[index % len(prodNodes)].vote(account, producers)
        index+=1

    #verify nodes are in sync and advancing
    cluster.waitOnClusterSync(blockAdvancing=5)
    blockNum=node.getNextCleanProductionCycle(trans)
    blockProducer=node.getBlockProducerByNum(blockNum)
    Print("Validating blockNum=%s, producer=%s" % (blockNum, blockProducer))
    cluster.biosNode.kill(signal.SIGTERM)

    lastBlockProducer=blockProducer
    while blockProducer==lastBlockProducer:
        blockNum+=1
        blockProducer=node.getBlockProducerByNum(blockNum)

    productionCycle=[]
    producerToSlot={}
    slot=-1
    inRowCountPerProducer=12
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
            blockProducer=node.getBlockProducerByNum(blockNum)

        if producerToSlot[lastBlockProducer]["count"]!=inRowCountPerProducer:
            Utils.errorExit("Producer %s, in slot %d, expected to produce %d blocks but produced %d blocks" % (blockProducer, inRowCountPerProducer, producerToSlot[lastBlockProducer]["count"]))

        if blockProducer==productionCycle[0]:
            break

    output=None
    for blockProducer in productionCycle:
        if output is None:
            output=""
        else:
            output+=", "
        output+=blockProducer+":"+str(producerToSlot[blockProducer]["count"])
    Print("ProductionCycle ->> {\n%s\n}" % output)

    for prodNode in prodNodes:
        prodNode.getInfo()

    cluster.reportStatus()

    Print("Sending command to kill \"bridge\" node to separate the 2 producer groups.")
    # block number to start expecting node killed after
    preKillBlockNum=nonProdNode.getBlockNum()
    preKillBlockProducer=nonProdNode.getBlockProducerByNum(preKillBlockNum)
    # kill at last block before defproducerl, since the block it is killed on will get propagated
    killAtProducer="defproducerk"
    nonProdNode.killNodeOnProducer(producer=killAtProducer, whereInSequence=(inRowCountPerProducer-1))

    # will search full cycle after the current block, since we don't know how many blocks were produced since retrieving
    # block number and issuing kill command
    postKillBlockNum=prodNodes[1].getBlockNum()
    blockProducers0=[]
    blockProducers1=[]
    libs0=[]
    libs1=[]
    lastBlockNum=max([preKillBlockNum,postKillBlockNum])+maxActiveProducers*inRowCountPerProducer
    actualLastBlockNum=None
    prodChanged=False
    nextProdChange=False
    info0=prodNodes[0].getInfo(exitOnError=True)
    info1=prodNodes[1].getInfo(exitOnError=True)
    headBlockNum=min(int(info0["head_block_num"]),int(info1["head_block_num"]))
    libNum=min(int(info0["last_irreversible_block_num"]), int(info1["last_irreversible_block_num"]))
    for blockNum in range(preKillBlockNum,lastBlockNum):
        if blockNum>headBlockNum:
            info0=prodNodes[0].getInfo(exitOnError=True)
            info1=prodNodes[1].getInfo(exitOnError=True)
            headBlockNum=min(int(info0["head_block_num"]),int(info1["head_block_num"]))
            libNum=min(int(info0["last_irreversible_block_num"]), int(info1["last_irreversible_block_num"]))

        blockProducer0=prodNodes[0].getBlockProducerByNum(blockNum)
        blockProducer1=prodNodes[1].getBlockProducerByNum(blockNum)
        blockProducers0.append({"blockNum":blockNum, "prod":blockProducer0})
        blockProducers1.append({"blockNum":blockNum, "prod":blockProducer1})
        # ensure that we wait for the next instance of killAtProducer
        if not prodChanged:
            if preKillBlockProducer!=blockProducer0:
                prodChanged=True
        if not nextProdChange and prodChanged and blockProducer1==killAtProducer:
            nextProdChange=True
        elif nextProdChange and blockProducer1!=killAtProducer:
            actualLastBlockNum=blockNum
            break
        if blockProducer0!=blockProducer1:
            Utils.errorExit("Groups reported different block producers for block number %d. %s != %s." % (blockNum,blockProducer0,blockProducer1))

    def analyzeBPs(bps0, bps1, expectDivergence):
        start=0
        index=None
        length=len(bps0)
        firstDivergence=None
        printInfo=False
        while start < length:
            bpsStr=None
            for i in range(start,length):
                bp0=bps0[i]
                bp1=bps1[i]
                if bpsStr is None:
                    bpsStr=""
                else:
                    bpsStr+=", "
                blockNum0=bp0["blockNum"]
                prod0=bp0["prod"]
                blockNum1=bp1["blockNum"]
                prod1=bp1["prod"]
                numDiff=True if blockNum0!=blockNum1 else False
                prodDiff=True if prod0!=prod1 else False
                if numDiff or prodDiff:
                    index=i
                    if firstDivergence is None:
                        firstDivergence=min(blockNum0, blockNum1)
                    if not expectDivergence:
                        printInfo=True
                    break
                bpsStr+=str(blockNum0)+"->"+prod0

            if index is None:
                return

            bpsStr0=None
            bpsStr2=None
            start=length
            for i in range(index,length):
                if bpsStr0 is None:
                    bpsStr0=""
                    bpsStr1=""
                else:
                    bpsStr0+=", "
                    bpsStr1+=", "
                bp0=bps0[i]
                bp1=bps1[i]
                blockNum0=bp0["blockNum"]
                prod0=bp0["prod"]
                blockNum1=bp1["blockNum"]
                prod1=bp1["prod"]
                numDiff="*" if blockNum0!=blockNum1 else ""
                prodDiff="*" if prod0!=prod1 else ""
                if not numDiff and not prodDiff:
                    start=i
                    index=None
                    break
                bpsStr0+=str(blockNum0)+numDiff+"->"+prod0+prodDiff
                bpsStr1+=str(blockNum1)+numDiff+"->"+prod1+prodDiff
            if printInfo:
                Print("ERROR: Analyzing Block Producers, did not expect nodes to indicate different block producers for the same blocks.")
                Print("Matching Blocks= %s" % (bpsStr))
                Print("Diverging branch node0= %s" % (bpsStr0))
                Print("Diverging branch node1= %s" % (bpsStr1))
        return firstDivergence

    firstDivergence=analyzeBPs(blockProducers0, blockProducers1, expectDivergence=True)
    # Nodes should not have diverged till the last block
    assert(firstDivergence==blockNum)
    blockProducers0=[]
    blockProducers1=[]

    assert(not nonProdNode.verifyAlive())
    for prodNode in prodNodes:
        prodNode.getInfo()

    killBlockNum=blockNum
#    lastBlockNum=killBlockNum+(maxActiveProducers - 1)*inRowCountPerProducer+1  # allow 1st testnet group to produce just 1 more block than the 2nd
    lastBlockNum=prodNodes[1].getBlockNum()+(maxActiveProducers - 1)*inRowCountPerProducer+1  # allow 1st testnet group to produce just 1 more block than the 2nd
    for blockNum in range(killBlockNum,lastBlockNum):
        blockProducer0=prodNodes[0].getBlockProducerByNum(blockNum)
        blockProducer1=prodNodes[1].getBlockProducerByNum(blockNum)
        blockProducers0.append({"blockNum":blockNum, "prod":blockProducer0})
        blockProducers1.append({"blockNum":blockNum, "prod":blockProducer1})

    info0=prodNodes[0].getInfo(blockNum)
    info1=prodNodes[1].getInfo(blockNum)
    firstDivergence=analyzeBPs(blockProducers0, blockProducers1, expectDivergence=True)
    assert(firstDivergence==killBlockNum)
    blockProducers0=[]
    blockProducers1=[]

    if not nonProdNode.relaunch(nonProdNode.nodeNum, None):
        errorExit("Failure - (non-production) node %d should have restarted" % (nonProdNode.nodeNum))

    # ensure all blocks from the lib before divergence till the current head are now in consensus
    endBlockNum=max(prodNodes[0].getBlockNum(), prodNodes[1].getBlockNum())

    for blockNum in range(libNum,endBlockNum):
        blockProducer0=prodNodes[0].getBlockProducerByNum(blockNum)
        blockProducer1=prodNodes[1].getBlockProducerByNum(blockNum)
        blockProducers0.append({"blockNum":blockNum, "prod":blockProducer0})
        blockProducers1.append({"blockNum":blockNum, "prod":blockProducer1})


    firstDivergence=analyzeBPs(blockProducers0, blockProducers1, expectDivergence=False)
    assert(firstDivergence==None)
    blockProducers0=[]
    blockProducers1=[]

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
