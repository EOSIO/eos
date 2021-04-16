#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import BlockType
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper

import copy
import decimal
import re
import signal
import json
import os
import time

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

    Utils.Print("\n\n\n\n\nCheck after KILL:")
    Utils.Print("\n\n\n\n\nNext Round of Info:")
    cluster.reportInfo()

    producers = [cluster.getNode(x) for x in range(pnodes) ]
    apiNodes = [cluster.getNode(x) for x in range(pnodes, totalNodes)]
    apiNodes.append(cluster.biosNode)

    blockProducer = producers[0].getHeadOrLib()["producer"]

    cluster.verifyInSync()

    featureDict = producers[0].getSupportedProtocolFeatureDict()
    Utils.Print("feature dict: {}".format(json.dumps(featureDict, indent=4, sort_keys=True)))

    cluster.reportInfo()
    Utils.Print("Activating SECURITY_GROUP Feature")

    #Utils.Print("act feature dict: {}".format(json.dumps(producers[0].getActivatedProtocolFeatures(), indent=4, sort_keys=True)))
    timeout = ( pnodes * 12 / 2 ) * 2   # (number of producers * blocks produced / 0.5 blocks per second) * 2 rounds
    for producer in producers:
        producers[0].waitUntilBeginningOfProdTurn(blockProducer, timeout=timeout)
        feature = "SECURITY_GROUP"
        producers[0].activateFeatures([feature])
        if producers[0].containsFeatures([feature]):
            break

    Utils.Print("SECURITY_GROUP Feature activated")
    cluster.reportInfo()

    assert producers[0].containsFeatures([feature]), "{} feature was not activated".format(feature)

    def publishContract(account, file, waitForTransBlock=False):
        Print("Publish contract")
        cluster.reportInfo()
        return producers[0].publishContract(account, "unittests/test-contracts/security_group_test/", "{}.wasm".format(file), "{}.abi".format(file), waitForTransBlock=waitForTransBlock)

    publishContract(cluster.eosioAccount, 'eosio.secgrp', waitForTransBlock=True)

    participants = [x for x in producers]
    nonParticipants = [x for x in apiNodes]

    def security_group(nodeNums):
        action = None
        for nodeNum in nodeNums:
            if action is None:
                action = '[['
            else:
                action += ','
            action += '"{}"'.format(Node.participantName(nodeNum))
        action += ']]'

        Utils.Print("adding {} to the security group".format(action))
        trans = producers[0].pushMessage(cluster.eosioAccount.name, "add", action, "--permission eosio@active")
        Utils.Print("add trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))
        trans = producers[0].pushMessage(cluster.eosioAccount.name, "publish", "[0]", "--permission eosio@active")
        Utils.Print("publish action trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))
        return trans

    def verifyParticipantsTransactionFinalized(transId):
        Utils.Print("Verify participants are in sync")
        for part in participants:
            if part.waitForTransFinalization(transId) == None:
                Utils.errorExit("Transaction: {}, never finalized".format(trans))

    def verifyNonParticipants(transId):
        Utils.Print("Verify non-participants don't receive blocks")
        publishBlock = producers[0].getBlockIdByTransId(transId)
        prodLib = producers[0].getBlockNum(blockType=BlockType.lib)
        waitForLib = prodLib + 3 * 12
        if producers[0].waitForBlock(waitForLib, blockType=BlockType.lib) == None:
            Utils.errorExit("Producer did not advance lib the expected amount.  Starting lib: {}, exp lib: {}, actual state: {}".format(prodLib, waitForLib, producers[0].getInfo()))
        producerHead = producers[0].getBlockNum()

        for nonParticipant in nonParticipants:
            nonParticipantPostLIB = nonParticipant.getBlockNum(blockType=BlockType.lib)
            assert nonParticipantPostLIB < publishBlock, "Participants not in security group should not have advanced LIB to {}, but it has advanced to {}".format(publishBlock, nonParticipantPostLIB)
            nonParticipantHead = nonParticipant.getBlockNum()
            assert nonParticipantHead < producerHead, "Participants (that are not producers themselves) should not advance head to {}, but it has advanced to {}".format(producerHead, nonParticipantHead)

    Utils.Print("Add all producers to security group")
    publishTrans = security_group([x for x in range(pnodes)])
    publishTransId = Node.getTransId(publishTrans[1])
    verifyParticipantsTransactionFinalized(publishTransId)
    verifyNonParticipants(publishTransId)

    while len(nonParticipants) > 0:
        toAdd = nonParticipants[0]
        participants.append(toAdd)
        del nonParticipants[0]
        Utils.Print("Take a non-participant and make a participant. Now there are {} participants and {} non-participants".format(len(participants), len(nonParticipants)))

        toAddNum = None
        num = 0
        for node in cluster.getNodes():
            if node == toAdd:
                toAddNum = num
                break
            num += 1
        if toAddNum is None:
            assert toAdd == cluster.biosNode
            toAddNum = totalNodes
        publishTrans = security_group([toAddNum])
        publishTransId = Node.getTransId(publishTrans[1])
        verifyParticipantsTransactionFinalized(publishTransId)
        verifyNonParticipants(publishTransId)


    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
