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

    Utils.Print("\n\n\n\n\nNext Round of Info:")
    cluster.reportInfo()

    producers = [cluster.getNode(x) for x in range(pnodes) ]
    apiNodes = [cluster.getNode(x) for x in range(pnodes, totalNodes)]
    apiNodes.append(cluster.biosNode)

    feature = "SECURITY_GROUP"
    Utils.Print("Activating {} Feature".format(feature))
    producers[0].activateAndVerifyFeatures({feature})
    cluster.verifyInSync()

    featureDict = producers[0].getSupportedProtocolFeatureDict()
    Utils.Print("feature dict: {}".format(json.dumps(featureDict, indent=4, sort_keys=True)))

    Utils.Print("{} Feature activated".format(feature))
    cluster.reportInfo()

    def publishContract(account, file, waitForTransBlock=False):
        Print("Publish contract")
        cluster.reportInfo()
        return producers[0].publishContract(account, "unittests/test-contracts/security_group_test/", "{}.wasm".format(file), "{}.abi".format(file), waitForTransBlock=waitForTransBlock)

    publishContract(cluster.eosioAccount, 'eosio.secgrp', waitForTransBlock=True)

    participants = [x for x in producers]
    nonParticipants = [x for x in apiNodes]

    # this is passed to limit the number of add/remove table entries are processed, but using it here to keep from getting duplicate transactions
    publishProcessNum = 20
    def security_group(addNodeNums=[], removeNodeNums=[]):
        def createAction(nodeNums):
            action = None
            for nodeNum in nodeNums:
                if action is None:
                    action = '[['
                else:
                    action += ','
                action += '"{}"'.format(Node.participantName(nodeNum))
            if action:
                action += ']]'
            return action

        addAction = createAction(addNodeNums)
        removeAction = createAction(removeNodeNums)

        if addAction:
            Utils.Print("adding {} to the security group".format(addAction))
            trans = producers[0].pushMessage(cluster.eosioAccount.name, "add", addAction, "--permission eosio@active")
            Utils.Print("add trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))

        if removeAction:
            Utils.Print("removing {} from the security group".format(removeAction))
            trans = producers[0].pushMessage(cluster.eosioAccount.name, "remove", removeAction, "--permission eosio@active")
            Utils.Print("remove trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))

        global publishProcessNum
        publishProcessNum += 1
        trans = producers[0].pushMessage(cluster.eosioAccount.name, "publish", "[{}]".format(publishProcessNum), "--permission eosio@active")
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

    def verifySecurityGroup(publishTransPair):
        publishTransId = Node.getTransId(publishTransPair[1])
        verifyParticipantsTransactionFinalized(publishTransId)
        verifyNonParticipants(publishTransId)

    def moveToParticipants():
        movedNode = nonParticipants[0]
        participants.append(movedNode)
        del nonParticipants[0]
        return movedNode

    def moveToNonParticipants():
        movedNode = participants[-1]
        # popping off back of participants and need to push on the front of nonParticipants
        nonParticipants.insert(0, movedNode)
        del participants[-1]
        return movedNode

    def addToSg():
        node = moveToParticipants()
        Utils.Print("Take a non-participant and make a participant. Now there are {} participants and {} non-participants".format(len(participants), len(nonParticipants)))
        toAddNum = cluster.getParticipantNum(node)
        return security_group([toAddNum])

    def remFromSg():
        node = moveToNonParticipants()
        Utils.Print("Take a participant and make a non-participant. Now there are {} participants and {} non-participants".format(len(participants), len(nonParticipants)))
        toRemoveNum = cluster.getParticipantNum(node)
        return security_group(removeNodeNums=[toRemoveNum])

    Utils.Print("Add all producers to security group")
    publishTrans = security_group([x for x in range(pnodes)])
    verifySecurityGroup(publishTrans)

    cluster.reportInfo()


    # one by one add each nonParticipant to the security group
    while len(nonParticipants) > 0:
        publishTrans = addToSg()
        verifySecurityGroup(publishTrans)
        cluster.reportInfo()


    # one by one remove each (original) nonParticipant from the security group
    while len(participants) > pnodes:
        publishTrans = remFromSg()
        verifySecurityGroup(publishTrans)
        cluster.reportInfo()


    # if we have more than 1 api node, we will add and remove all those nodes in bulk, if not it is just a repeat of the above test
    if totalNodes > pnodes + 1:
        # add all the api nodes to security group at once
        toAdd = []
        for apiNode in nonParticipants:
            participantNum = cluster.getParticipantNum(apiNode)
            toAdd.append(participantNum)
        participants.extend(nonParticipants)
        nonParticipants = []

        Utils.Print("Add all api nodes to security group")
        publishTrans = security_group(addNodeNums=toAdd)
        verifySecurityGroup(publishTrans)

        cluster.reportInfo()


        # alternate adding/removing participants to ensure the security group doesn't change
        initialBlockNum = None
        blockNum = None
        def is_done():
            # want to ensure that we can identify the range of libs the security group was changed in
            return blockNum - initialBlockNum > 12

        done = False
        # keep adding and removing nodes till we are done
        while not done:
            if blockNum:
                participants[0].waitForNextBlock()

            while not done and len(participants) > pnodes:
                publishTrans = remFromSg()
                Utils.Print("publishTrans: {}".format(json.dumps(publishTrans, indent=2)))
                blockNum = Node.getTransBlockNum(publishTrans[1])
                if initialBlockNum is None:
                    initialBlockNum = blockNum
                    lastBlockNum = blockNum
                done = is_done()

            while not done and len(nonParticipants) > 0:
                publishTrans = addToSg()
                blockNum = Node.getTransBlockNum(publishTrans[1])
                done = is_done()

        Utils.Print("First adjustment to security group was in block num: {}, verifying no changes till block num: {} is finalized".format(initialBlockNum, blockNum))
        verifySecurityGroup(publishTrans)

        cluster.reportInfo()

        # remove all the api nodes from the security group at once
        toRemove = []
        # index pnodes and following are moving to nonParticipants, so participants has everything before that
        nonParticipants = participants[pnodes:]
        participants = participants[:pnodes]
        for apiNode in nonParticipants:
            participantNum = cluster.getParticipantNum(apiNode)
            toRemove.append(participantNum)

        Utils.Print("Remove all api nodes from security group")
        publishTrans = security_group(removeNodeNums=toRemove)
        verifySecurityGroup(publishTrans)

        cluster.reportInfo()

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
