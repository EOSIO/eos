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

###############################################################
# privacy_startup_network
#
# General test for Privacy to verify TLS connections, and slowly adding participants to the security group and verifying
# how blocks and transactions are sent/not sent.
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit
cmdError=Utils.cmdError
from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--port","-p","-n","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"
                              ,"--sanity-test","--wallet-port"})
port=args.port
pnodes=args.p
relayNodes=pnodes # every pnode paired with a relay node
apiNodes=2        # minimum number of apiNodes that will be used in this test
minTotalNodes=pnodes+relayNodes+apiNodes
totalNodes=args.n if args.n >= minTotalNodes else minTotalNodes
if totalNodes > minTotalNodes:
    apiNodes += totalNodes - minTotalNodes

Utils.Debug=args.v
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
onlyBios=False
killAll=args.clean_run
sanityTest=args.sanity_test
walletPort=args.wallet_port

cluster=Cluster(host=TestHelper.LOCAL_HOST, port=port, walletd=True)
walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill
dontBootstrap=sanityTest # intent is to limit the scope of the sanity test to just verifying that nodes can be started

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
    topo = {}
    firstApiNodeNum = pnodes + relayNodes
    apiNodeNums = [x for x in range(firstApiNodeNum, totalNodes)]
    for producerNum in range(pnodes):
        pairedRelayNodeNum = pnodes + producerNum
        topo[producerNum] = [pairedRelayNodeNum]
        topo[pairedRelayNodeNum] = apiNodeNums
    Utils.Print("topo: {}".format(json.dumps(topo, indent=4, sort_keys=True)))

    if cluster.launch(pnodes=pnodes, totalNodes=totalNodes, prodCount=1, onlyBios=False, dontBootstrap=dontBootstrap, configSecurityGroup=True, topo=topo) is False:
        cmdError("launcher")
        errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    cluster.biosNode.kill(signal.SIGTERM)

    producers = [cluster.getNode(x) for x in range(pnodes) ]
    relays = [cluster.getNode(pnodes + x) for x in range(pnodes) ]
    apiNodes = [cluster.getNode(x) for x in apiNodeNums]

    def createAccount(newAcc):
        producers[0].createInitializeAccount(newAcc, cluster.eosioAccount)
        ignWallet = cluster.walletMgr.create("ignition")  # will actually just look up the wallet
        cluster.walletMgr.importKey(newAcc, ignWallet)

    numAccounts = 4
    testAccounts = Cluster.createAccountKeys(numAccounts)
    accountPrefix = "testaccount"
    for i in range(numAccounts):
        testAccount = testAccounts[i]
        testAccount.name  = accountPrefix + str(i + 1)
        createAccount(testAccount)

    blockProducer = None

    def verifyInSync(producerNum):
        Utils.Print("Ensure all nodes are in-sync")
        lib = producers[producerNum].getInfo()["last_irreversible_block_num"]
        headBlockNum = producers[producerNum].getBlockNum()
        headBlock = producers[producerNum].getBlock(headBlockNum)
        global blockProducer
        if blockProducer is None:
            blockProducer = headBlock["producer"]
        Utils.Print("headBlock: {}".format(json.dumps(headBlock, indent=4, sort_keys=True)))
        headBlockId = headBlock["id"]
        for prod in producers:
            if prod == producers[producerNum]:
                continue

            assert prod.waitForBlock(headBlockNum, timeout = 10, reportInterval = 1) != None, "Producer node failed to get block number {}".format(headBlockNum)
            prod.getBlock(headBlockId)  # if it isn't there it will throw an exception
            assert prod.waitForBlock(lib, blockType=BlockType.lib), \
                "Producer node is failing to advance its lib ({}) with producer {} ({})".format(node.getInfo()["last_irreversible_block_num"], producerNum, lib)
        for node in apiNodes:
            assert node.waitForBlock(headBlockNum, timeout = 10, reportInterval = 1) != None, "API node failed to get block number {}".format(headBlockNum)
            node.getBlock(headBlockId)  # if it isn't there it will throw an exception
            assert node.waitForBlock(lib, blockType=BlockType.lib), \
                "API node is failing to advance its lib ({}) with producer {} ({})".format(node.getInfo()["last_irreversible_block_num"], producerNum, lib)

        Utils.Print("Ensure all nodes are in-sync")
        assert node.waitForBlock(lib + 1, blockType=BlockType.lib, reportInterval = 1) != None, "Producer node failed to advance lib ahead one block to: {}".format(lib + 1)

    verifyInSync(producerNum=0)

    featureDict = producers[0].getSupportedProtocolFeatureDict()
    Utils.Print("feature dict: {}".format(json.dumps(featureDict, indent=4, sort_keys=True)))

    Utils.Print("act feature dict: {}".format(json.dumps(producers[0].getActivatedProtocolFeatures(), indent=4, sort_keys=True)))
    timeout = ( pnodes * 12 / 2 ) * 2   # (number of producers * blocks produced / 0.5 blocks per second) * 2 rounds
    producers[0].waitUntilBeginningOfProdTurn(blockProducer, timeout=timeout)
    feature = "SECURITY_GROUP"
    producers[0].activateFeatures([feature])
    assert producers[0].containsFeatures([feature]), "{} feature was not activated".format(feature)

    if sanityTest:
        testSuccessful=True
        exit(0)

    def publishContract(account, wasmFile, waitForTransBlock=False):
        Print("Publish contract")
        return producers[0].publishContract(account, "unittests/test-contracts/security_group_test/", wasmFile, abiFile=None, waitForTransBlock=waitForTransBlock)

    publishContract(testAccounts[0], 'security_group_test.wasm', waitForTransBlock=True)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
