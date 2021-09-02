import copy
import json

from Node import BlockType
from Node import Node
from Node import ReturnType
from testUtils import Utils

# pylint: disable=too-many-public-methods
class SecurityGroup(object):

    # pylint: disable=too-many-instance-attributes
    # pylint: disable=too-many-arguments
    def __init__(self, nonParticipants, contractAccount, defaultNode=None, minAddRemEntriesToPublish=100, activateAndPublish=True):
        self.participants = []
        self.contractAccount = contractAccount
        assert len(nonParticipants) > 0
        # copy over all the running processes
        self.nonParticipants = copy.copy(nonParticipants)
        if Utils.Debug: Utils.Print("Creating SecurityGroup with the following nonParticipants: []".format(SecurityGroup.createAction(self.nonParticipants)))
        def findDefault(nodes):
            for node in nodes:
                if node.pid:
                    return node

            Utils.errorExit("SecurityGroup is being constructed with no running nodes, there needs to be at least one running node")

        self.defaultNode = defaultNode if defaultNode else findDefault(self.nonParticipants)
        self.publishProcessNum = minAddRemEntriesToPublish
        if activateAndPublish:
            SecurityGroup.activateFeature(self.defaultNode)

            self.contractTrans = SecurityGroup.publishContract(self.defaultNode, self.contractAccount)
        else:
            self.contractTrans = None

        self.publishTrans = None

    # activate the SECURITY_GROUP feature
    @staticmethod
    def activateFeature(node):
        feature = "SECURITY_GROUP"
        Utils.Print("Activating {} Feature".format(feature))
        node.activateAndVerifyFeatures({feature})

        featureDict = node.getSupportedProtocolFeatureDict()
        Utils.Print("feature dict: {}".format(json.dumps(featureDict, indent=4, sort_keys=True)))

        Utils.Print("{} Feature activated".format(feature))

    # publish the eosio.secgrp contract
    @staticmethod
    def publishContract(node, account):
        contract = "eosio.secgrp"
        Utils.Print("Publish {} contract".format(contract))
        return node.publishContract(account, "unittests/test-contracts/security_group_test/", "{}.wasm".format(contract), "{}.abi".format(contract), waitForTransBlock=True)

    # move the provided nodes from the nonParticipants list to the participants list
    def __addParticipants(self, nodes):
        if len(nodes) == 0:
            return

        if Utils.Debug: Utils.Print("Moving the following: {}, from the nonParticipants list: {}, to the participants list: {}".format(SecurityGroup.createAction(nodes), SecurityGroup.createAction(self.nonParticipants), SecurityGroup.createAction(self.participants)))
        for node in nodes:
            assert node in self.nonParticipants, "Cannot remove {} from nonParticipants list: {}".format(node, SecurityGroup.createAction(self.nonParticipants))
            self.nonParticipants.remove(node)

        self.participants.extend(nodes)

    # move the provided nodes from the participants list to the nonParticipants list
    def __remParticipants(self, nodes):
        if len(nodes) == 0:
            return

        if Utils.Debug: Utils.Print("Moving the following: {}, from the participants list: {}, to the non-participants list: {}".format(SecurityGroup.createAction(nodes), SecurityGroup.createAction(self.participants), SecurityGroup.createAction(self.nonParticipants)))
        for node in nodes:
            self.participants.remove(node)

        self.nonParticipants.extend(nodes)

    @staticmethod
    def toString(nodes):
        return "[[{}]]".format(','.join(['"{}"'.format(node.getParticipant()) for node in nodes]))

    # create the action payload for an add or remove action
    @staticmethod
    def createAction(nodes):
        return None if len(nodes) == 0 else SecurityGroup.toString(nodes)

    # sends actions to add/remove the provided nodes to/from the network's security group
    def editSecurityGroup(self, addNodes=[], removeNodes=[], node=None):
        if node is None:
            node = self.defaultNode

        def copyIfNeeded(nodes):
            # doing deep copy in case the passed in list IS one of our lists, which will be adjusted
            if nodes is self.participants or nodes is self.nonParticipants:
                return copy.copy(nodes)
            return nodes

        addAction = SecurityGroup.createAction(addNodes)
        self.__addParticipants(copyIfNeeded(addNodes))

        removeAction = SecurityGroup.createAction(removeNodes)
        self.__remParticipants(copyIfNeeded(removeNodes))

        assert addAction or removeAction, "Called editSecurityGroup and there were neither nodes to add nore remove"
        if addAction:
            Utils.Print("adding {} to the security group".format(addAction))
            succeeded, trans = node.pushMessage(self.contractAccount.name, "add", addAction, "--permission eosio@active")
            assert succeeded
            Utils.Print("add trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))

        if removeAction:
            Utils.Print("removing {} from the security group".format(removeAction))
            succeeded, trans = node.pushMessage(self.contractAccount.name, "remove", removeAction, "--permission eosio@active")
            assert succeeded
            Utils.Print("remove trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))
        
        # this is to prevent scenario when 1st transaction block was dropped and transaction appeared in later block
        # 100% to avoid flackiness there should be LIB wait but for speed we use here just 3 blocks.
        # if you'll find this part flacky, increase number of blocks
        node.waitForBlock(int(trans["processed"]["block_num"]) + 3)

        self.publishProcessNum += 1
        succeeded, self.publishTrans = node.pushMessage(self.contractAccount.name, "publish", "[{}]".format(self.publishProcessNum), "--permission eosio@active")
        Utils.Print("publish action trans: {}".format(json.dumps(self.publishTrans, indent=4, sort_keys=True)))
        assert succeeded
        return self.publishTrans

    # verify that the transaction ID is found, and finalized, in every node in the participants list
    def verifyParticipantsTransactionFinalized(self, transId):
        Utils.Print("Verify participants are in sync")
        assert transId
        headAtTransFinalized = None
        for part in self.participants:
            if part.pid is None:
                continue
            Utils.Print("Checking node {}".format(part.nodeId))
            if part.waitForTransFinalization(transId) == None:
                Utils.errorExit("Transaction: {}, never finalized".format(transId))
            headAtTransFinalized = part.getBlockNum()
        assert headAtTransFinalized, "None of the participants are currently running, no reason to call verifyParticipantsTransactionFinalized"
        Utils.Print("Head when transaction been finalized: {}".format(headAtTransFinalized))
        return headAtTransFinalized

    # verify that the block for the transaction ID is never finalized in nonParticipants
    def verifyNonParticipants(self, transId, headAtTransFinalization):
        Utils.Print("Verify non-participants don't receive blocks")
        assert transId
        publishBlock = self.defaultNode.getBlockNumByTransId(transId)

        # first ensure that enough time has passed that the nonParticipant is not just trailing behind
        prodLib = self.defaultNode.getBlockNum(blockType=BlockType.lib)
        waitForLib = prodLib + 3 * 12
        if self.defaultNode.waitForBlock(waitForLib, blockType=BlockType.lib) == None:
            Utils.errorExit("Producer did not advance lib the expected amount.  Starting lib: {}, exp lib: {}, actual state: {}".format(prodLib, waitForLib, self.defaultNode.getInfo()))

        # verify each nonParticipant in the list has not advanced its lib to the publish block, since the block that would cause it to become finalized would
        # never have been forwarded to a nonParticipant
        expectedProducers = []
        for x in self.nonParticipants:
            expectedProducers.extend(x.getProducers());

        for nonParticipant in self.nonParticipants:
            if nonParticipant.pid is None:
                continue
            nonParticipantPostLIB = nonParticipant.getBlockNum(blockType=BlockType.lib)
            Utils.Print("node {} lib = {}".format(nonParticipant.nodeId, nonParticipantPostLIB))
            assert nonParticipantPostLIB < publishBlock, "Participants not in security group should not have advanced LIB to {}, but it has advanced to {}".format(publishBlock, nonParticipantPostLIB)
            nonParticipantHead = nonParticipant.getBlockNum()
            Utils.Print("node {} head = {}".format(nonParticipant.nodeId, nonParticipantHead))
            if nonParticipantHead > headAtTransFinalization:
                Utils.Print("non-participant head is beyond transaction head, checking that producer is out of participating group")
                producer = nonParticipant.getBlockProducerByNum(nonParticipantHead)
                assert producer in expectedProducers, \
                       "Participants should not advance head to {} unless they are producing their own blocks. It has advanced to {} with producer {}, \
                        which is not one of the non-participant producers: [{}]".format(headAtTransFinalization, nonParticipantHead, producer, ", ".join(expectedProducers))

    def getLatestPublishTransId(self):
        return Node.getTransId(self.publishTrans)

    # verify that the participants' and nonParticipants' nodes are consistent based on the publish transaction
    def verifySecurityGroup(self, publishTrans = None, numProducersNotInSecurityGroup = 0):
        if publishTrans is None:
            publishTrans = self.publishTrans
        publishTransId = Node.getTransId(publishTrans)
        headAtTransFinalization = self.verifyParticipantsTransactionFinalized(publishTransId)
        activeParticipants = [x for x in self.participants if x.pid]
        Node.verifyInSync(activeParticipants, maxDelayForABlock = (numProducersNotInSecurityGroup + 1) * 6) # give at least one window extra time for block to produce
        self.verifyNonParticipants(publishTransId, headAtTransFinalization)

    def moveToSecurityGroup(self, index = 0):
        assert abs(index) < len(self.nonParticipants)
        trans = self.editSecurityGroup([self.nonParticipants[index]])
        Utils.Print("Take a non-participant and make a participant. Now there are {} participants and {} non-participants".format(len(self.participants), len(self.nonParticipants)))
        return trans

    def removeFromSecurityGroup(self, index = -1):
        assert abs(index) < len(self.participants)
        trans = self.editSecurityGroup(removeNodes=[self.participants[index]])
        Utils.Print("Take a participant and make a non-participant. Now there are {} participants and {} non-participants".format(len(self.participants), len(self.nonParticipants)))
        return trans
