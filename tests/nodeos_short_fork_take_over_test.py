#!/usr/bin/env python3

from testUtils import Utils
import testUtils
import time
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import BlockType
from Node import Node
from TestHelper import TestHelper

import decimal
import math
import re
import signal

###############################################################
# nodeos_short_fork_take_over_test
#
# Similar scenario to nodeos_forked_chain_test, except that there are only 3 producers and, after the "bridge" node is
# shutdown, the second producer node is also shutdown.  Then the "bridge" node is re-started followed by the producer
# node being started.
#
###############################################################
Print=Utils.Print

from core_symbol import CORE_SYMBOL

def analyzeBPs(bps0, bps1, expectDivergence):
    start=0
    index=None
    length=len(bps0)
    firstDivergence=None
    errorInDivergence=False
    analysysPass=0
    bpsStr=None
    bpsStr0=None
    bpsStr1=None
    while start < length:
        analysysPass+=1
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
                    errorInDivergence=True
                break
            bpsStr+=str(blockNum0)+"->"+prod0

        if index is None:
            if expectDivergence:
                errorInDivergence=True
                break
            return None

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
                if expectDivergence:
                    errorInDivergence=True
                break
            bpsStr0+=str(blockNum0)+numDiff+"->"+prod0+prodDiff
            bpsStr1+=str(blockNum1)+numDiff+"->"+prod1+prodDiff
        if errorInDivergence:
            break

    if errorInDivergence:
        msg="Failed analyzing block producers - "
        if expectDivergence:
            msg+="nodes do not indicate different block producers for the same blocks, but they are expected to diverge at some point."
        else:
            msg+="did not expect nodes to indicate different block producers for the same blocks."
        msg+="\n  Matching Blocks= %s \n  Diverging branch node0= %s \n  Diverging branch node1= %s" % (bpsStr,bpsStr0,bpsStr1)
        Utils.errorExit(msg)

    return firstDivergence

def getMinHeadAndLib(prodNodes):
    info0=prodNodes[0].getInfo(exitOnError=True)
    info1=prodNodes[1].getInfo(exitOnError=True)
    headBlockNum=min(int(info0["head_block_num"]),int(info1["head_block_num"]))
    libNum=min(int(info0["last_irreversible_block_num"]), int(info1["last_irreversible_block_num"]))
    return (headBlockNum, libNum)



args = TestHelper.parse_args({"--prod-count","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run",
                              "--wallet-port"})
Utils.Debug=args.v
totalProducerNodes=2
totalNonProducerNodes=1
totalNodes=totalProducerNodes+totalNonProducerNodes
maxActiveProducers=3
totalProducers=maxActiveProducers
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
killAll=args.clean_run
walletPort=args.wallet_port

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
    Print("Stand up cluster")
    specificExtraNodeosArgs={}
    # producer nodes will be mapped to 0 through totalProducerNodes-1, so the number totalProducerNodes will be the non-producing node
    specificExtraNodeosArgs[totalProducerNodes]="--plugin eosio::test_control_api_plugin"


    # ***   setup topogrophy   ***

    # "bridge" shape connects defprocera through defproducerk (in node0) to each other and defproducerl through defproduceru (in node01)
    # and the only connection between those 2 groups is through the bridge node
    if cluster.launch(prodCount=2, topo="bridge", pnodes=totalProducerNodes,
                      totalNodes=totalNodes, totalProducers=totalProducers,
                      useBiosBootFile=False, specificExtraNodeosArgs=specificExtraNodeosArgs, onlySetProds=True) is False:
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    # ***   identify each node (producers and non-producing node)   ***

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
            prodNodes.append(node)
            producers.extend(node.producers)


    node=prodNodes[0]
    node1=prodNodes[1]

    # ***   Identify a block where production is stable   ***

    #verify nodes are in sync and advancing
    cluster.waitOnClusterSync(blockAdvancing=5)
    cluster.biosNode.kill(signal.SIGTERM)

    Utils.Print("catching defproducera")
    tries = 120
    blockNum = node.getHeadBlockNum()
    blockProducer=node.getBlockProducerByNum(blockNum)
    while blockProducer != "defproducera" and tries > 0:
        blockNum+=1
        blockProducer=node.getBlockProducerByNum(blockNum)
        tries = tries - 1

    if tries == 0:
        Utils.errorExit("failed to catch a block produced by defproducera")

    Utils.Print("catching the start of defproducerb")
    tries = 30
    while blockProducer != "defproducerb" and tries > 0:
        blockNum+=1
        blockProducer=node.getBlockProducerByNum(blockNum)
        tries = tries - 1

    if tries == 0:
        Utils.errorExit("failed to catch a block produced by defproducerb")

    blockNum+=1
    blockProducer=node.getBlockProducerByNum(blockNum)
    blockProducer1=node1.getBlockProducerByNum(blockNum)
    Utils.Print("block number %d is producer by %s in node0" % (blockNum, blockProducer))
    Utils.Print("block number %d is producer by %s in node1" % (blockNum, blockProducer1))

    # ***   Killing the "bridge" node   ***
    Print("Sending command to kill \"bridge\" node to separate the 2 producer groups.")
    # block number to start expecting node killed after
    preKillBlockNum=nonProdNode.getBlockNum()
    preKillBlockProducer=nonProdNode.getBlockProducerByNum(preKillBlockNum)
    # kill at last block before defproducerl, since the block it is killed on will get propagated
    killAtProducer="defproducerb"
    inRowCountPerProducer=12
    nonProdNode.killNodeOnProducer(producer=killAtProducer, whereInSequence=(inRowCountPerProducer-1))


    # ***   Identify a highest block number to check while we are trying to identify where the divergence will occur   ***

    # will search full cycle after the current block, since we don't know how many blocks were produced since retrieving
    # block number and issuing kill command
    postKillBlockNum=prodNodes[1].getBlockNum()
    blockProducers0=[]
    blockProducers1=[]
    libs0=[]
    libs1=[]
    lastBlockNum=max([preKillBlockNum,postKillBlockNum])+2*maxActiveProducers*inRowCountPerProducer
    actualLastBlockNum=None
    prodChanged=False
    nextProdChange=False
    #identify the earliest LIB to start identify the earliest block to check if divergent branches eventually reach concensus
    (headBlockNum, libNumAroundDivergence)=getMinHeadAndLib(prodNodes)
    Print("Tracking block producers from %d till divergence or %d. Head block is %d and lowest LIB is %d" % (preKillBlockNum, lastBlockNum, headBlockNum, libNumAroundDivergence))
    transitionCount=0
    missedTransitionBlock=None
    #range is half closed, but forking can happen at lastBlockNum;
    #that's why lastBlockNum + 1
    for blockNum in range(preKillBlockNum,lastBlockNum + 1):
        #avoiding getting LIB until my current block passes the head from the last time I checked
        if blockNum>headBlockNum:
            (headBlockNum, libNumAroundDivergence)=getMinHeadAndLib(prodNodes)

        # track the block number and producer from each producing node
        blockProducer0=prodNodes[0].getBlockProducerByNum(blockNum)
        blockProducer1=prodNodes[1].getBlockProducerByNum(blockNum)
        blockProducers0.append({"blockNum":blockNum, "prod":blockProducer0})
        blockProducers1.append({"blockNum":blockNum, "prod":blockProducer1})

        #in the case that the preKillBlockNum was also produced by killAtProducer, ensure that we have
        #at least one producer transition before checking for killAtProducer
        if not prodChanged:
            if preKillBlockProducer!=blockProducer0:
                prodChanged=True

        #since it is killing for the last block of killAtProducer, we look for the next producer change
        if not nextProdChange and prodChanged and blockProducer1==killAtProducer:
            nextProdChange=True
        elif nextProdChange and blockProducer1!=killAtProducer:
            nextProdChange=False
            if blockProducer0!=blockProducer1:
                Print("Divergence identified at block %s, node_00 producer: %s, node_01 producer: %s" % (blockNum, blockProducer0, blockProducer1))
                actualLastBlockNum=blockNum
                break
            else:
                missedTransitionBlock=blockNum
                transitionCount+=1
                # allow this to transition twice, in case the script was identifying an earlier transition than the bridge node received the kill command
                if transitionCount>1:
                    Print("At block %d and have passed producer: %s %d times and we have not diverged, stopping looking and letting errors report" % (blockNum, killAtProducer, transitionCount))
                    actualLastBlockNum=blockNum
                    break

        #if we diverge before identifying the actualLastBlockNum, then there is an ERROR
        if blockProducer0!=blockProducer1:
            extra="" if transitionCount==0 else " Diverged after expected killAtProducer transition at block %d." % (missedTransitionBlock)
            Utils.errorExit("Groups reported different block producers for block number %d.%s %s != %s." % (blockNum,extra,blockProducer0,blockProducer1))

    #verify that the non producing node is not alive (and populate the producer nodes with current getInfo data to report if
    #an error occurs)
    if nonProdNode.verifyAlive():
        Utils.errorExit("Expected the non-producing node to have shutdown.")

    Print("Analyzing the producers leading up to the block after killing the non-producing node, expecting divergence at %d" % (blockNum))

    firstDivergence=analyzeBPs(blockProducers0, blockProducers1, expectDivergence=True)
    # Nodes should not have diverged till the last block
    if firstDivergence!=blockNum:
        Utils.errorExit("Expected to diverge at %s, but diverged at %s." % (firstDivergence, blockNum))
    blockProducers0=[]
    blockProducers1=[]

    for prodNode in prodNodes:
        info=prodNode.getInfo()
        Print("node info: %s" % (info))

    killBlockNum=blockNum
    lastBlockNum=killBlockNum+(maxActiveProducers - 1)*inRowCountPerProducer+1  # allow 1st testnet group to produce just 1 more block than the 2nd

    Print("Tracking the blocks from the divergence till there are 10*12 blocks on one chain and 10*12+1 on the other, from block %d to %d" % (killBlockNum, lastBlockNum))

    for blockNum in range(killBlockNum,lastBlockNum):
        blockProducer0=prodNodes[0].getBlockProducerByNum(blockNum)
        blockProducer1=prodNodes[1].getBlockProducerByNum(blockNum)
        blockProducers0.append({"blockNum":blockNum, "prod":blockProducer0})
        blockProducers1.append({"blockNum":blockNum, "prod":blockProducer1})


    Print("Analyzing the producers from the divergence to the lastBlockNum and verify they stay diverged, expecting divergence at block %d" % (killBlockNum))

    firstDivergence=analyzeBPs(blockProducers0, blockProducers1, expectDivergence=True)
    if firstDivergence!=killBlockNum:
        Utils.errorExit("Expected to diverge at %s, but diverged at %s." % (firstDivergence, killBlockNum))
    blockProducers0=[]
    blockProducers1=[]

    for prodNode in prodNodes:
        info=prodNode.getInfo()
        Print("node info: %s" % (info))

    Print("killing node1(defproducerc) so that bridge node will frist connect to node0 (defproducera, defproducerb)")
    node1.kill(killSignal=15)
    time.sleep(2)
    if node1.verifyAlive():
        Utils.errorExit("Expected the node 1 to have shutdown.")

    Print("Relaunching the non-producing bridge node to connect the node 0 (defproducera, defproducerb)")
    if not nonProdNode.relaunch():
        errorExit("Failure - (non-production) node %d should have restarted" % (nonProdNode.nodeNum))

    Print("Relaunch node 1 (defproducerc) and let it connect to brigde node that already synced up with node 0")
    time.sleep(10)
    if not node1.relaunch(chainArg=" --enable-stale-production "):
        errorExit("Failure - (non-production) node 1 should have restarted")

    Print("Waiting to allow forks to resolve")
    time.sleep(3)

    for prodNode in prodNodes:
        info=prodNode.getInfo()
        Print("node info: %s" % (info))

    #ensure that the nodes have enough time to get in concensus, so wait for 3 producers to produce their complete round
    time.sleep(inRowCountPerProducer * 3 / 2)
    remainingChecks=20
    match=False
    checkHead=False
    while remainingChecks>0:
        checkMatchBlock=killBlockNum if not checkHead else prodNodes[0].getBlockNum()
        blockProducer0=prodNodes[0].getBlockProducerByNum(checkMatchBlock)
        blockProducer1=prodNodes[1].getBlockProducerByNum(checkMatchBlock)
        match=blockProducer0==blockProducer1
        if match:
            if checkHead:
                break
            else:
                checkHead=True
                continue
        Print("Fork has not resolved yet, wait a little more. Block %s has producer %s for node_00 and %s for node_01.  Original divergence was at block %s. Wait time remaining: %d" % (checkMatchBlock, blockProducer0, blockProducer1, killBlockNum, remainingChecks))
        time.sleep(1)
        remainingChecks-=1

    for prodNode in prodNodes:
        info=prodNode.getInfo()
        Print("node info: %s" % (info))

    # ensure all blocks from the lib before divergence till the current head are now in consensus
    endBlockNum=max(prodNodes[0].getBlockNum(), prodNodes[1].getBlockNum())

    Print("Identifying the producers from the saved LIB to the current highest head, from block %d to %d" % (libNumAroundDivergence, endBlockNum))

    for blockNum in range(libNumAroundDivergence,endBlockNum):
        blockProducer0=prodNodes[0].getBlockProducerByNum(blockNum)
        blockProducer1=prodNodes[1].getBlockProducerByNum(blockNum)
        blockProducers0.append({"blockNum":blockNum, "prod":blockProducer0})
        blockProducers1.append({"blockNum":blockNum, "prod":blockProducer1})


    Print("Analyzing the producers from the saved LIB to the current highest head and verify they match now")

    analyzeBPs(blockProducers0, blockProducers1, expectDivergence=False)

    resolvedKillBlockProducer=None
    for prod in blockProducers0:
        if prod["blockNum"]==killBlockNum:
            resolvedKillBlockProducer = prod["prod"]
    if resolvedKillBlockProducer is None:
        Utils.errorExit("Did not find find block %s (the original divergent block) in blockProducers0, test setup is wrong.  blockProducers0: %s" % (killBlockNum, ", ".join(blockProducers)))
    Print("Fork resolved and determined producer %s for block %s" % (resolvedKillBlockProducer, killBlockNum))

    blockProducers0=[]
    blockProducers1=[]

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

    if not testSuccessful:
        Print(Utils.FileDivider)
        Print("Compare Blocklog")
        cluster.compareBlockLogs()
        Print(Utils.FileDivider)
        Print("Compare Blocklog")
        cluster.printBlockLog()
        Print(Utils.FileDivider)

exit(0)
