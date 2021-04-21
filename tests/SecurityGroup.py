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
    def __init__(self, nonParticipants, contractAccount, defaultNode=None, minAddRemEntriesToPublish=100):
        self.participants = []
        self.contractAccount = contractAccount
        assert len(nonParticipants) > 0
        self.nonParticipants = copy.deepcopy(nonParticipants)
        if Utils.Debug: Utils.Print("Creating SecurityGroup with the following nonParticipants: []".format(SecurityGroup.createAction(self.nonParticipants)))
        self.defaultNode = defaultNode if defaultNode else nonParticipants[0]
        self.publishProcessNum = minAddRemEntriesToPublish
        Utils.Print("Publish contract")
        self.contractTrans = self.defaultNode.publishContract(self.contractAccount, "unittests/test-contracts/security_group_test/", "eosio.secgrp.wasm", "eosio.secgrp.abi", waitForTransBlock=True)
        self.publishTrans = None

    @staticmethod
    def __lessThan(lhsNode, rhsNode):
        assert lhsNode != rhsNode
        return lhsNode.getParticipant() < rhsNode.getParticipant()

    def __consistentStorage(self):
        intersect = list(set([x.getParticipant() for x in self.participants]) & set([x.getParticipant() for x in self.nonParticipants]))
        if len(intersect) != 0:
            Utils.exitError("Values: {}, are in the nonParticipants list: {}, and the participants list: {}".format(", ".join(intersect), SecurityGroup.createAction(self.nonParticipants), SecurityGroup.createAction(self.participants)))

    def __addParticipants(self, nodes):
        if Utils.Debug: Utils.Print("Moving the following: {}, from the nonParticipants list: {}, to the participants list: {}".format(SecurityGroup.createAction(nodes), SecurityGroup.createAction(self.nonParticipants), SecurityGroup.createAction(self.participants)))
        for node in nodes:
            if node not in self.nonParticipants:
                for nonParticipant in self.nonParticipants:
                    match = "" if node == nonParticipant else " not"
                    Utils.Print("node: '{}' does{} match '{}'".format(node, match, nonParticipant))
            assert node in self.nonParticipants, "Cannot remove {} from nonParticipants list: {}".format(node, SecurityGroup.createAction(self.nonParticipants))
            self.nonParticipants.remove(node)

        self.participants.extend(nodes)
        if Utils.Debug: Utils.Print("nonParticipants list: {}, to the participants list: {}".format(SecurityGroup.createAction(self.nonParticipants), SecurityGroup.createAction(self.participants)))
        self.__consistentStorage()

    def __remParticipants(self, nodes):
        if Utils.Debug: Utils.Print("Moving the following: {}, from the participants list: {}, to the non-participants list: {}".format(SecurityGroup.createAction(nodes), SecurityGroup.createAction(self.participants), SecurityGroup.createAction(self.nonParticipants)))
        for node in nodes:
            self.participants.remove(node)

        self.nonParticipants.extend(nodes)
        if Utils.Debug: Utils.Print("participants list: {}, to the non-participants list: {}".format(SecurityGroup.createAction(self.participants), SecurityGroup.createAction(self.nonParticipants)))
        self.__consistentStorage()

    @staticmethod
    def createAction(nodes):
        return None if len(nodes) == 0 else \
            "[[{}]]".format(','.join(['"{}"'.format(node.getParticipant()) for node in nodes]))

    def editSecurityGroup(self, addNodes=[], removeNodes=[]):

        addAction = SecurityGroup.createAction(addNodes)
        # doing deep copy in case the passed in list IS participants or nonParticipants lists, which will be adjusted
        self.__addParticipants(copy.deepcopy(addNodes))
        removeAction = SecurityGroup.createAction(removeNodes)
        self.__remParticipants(copy.deepcopy(removeNodes))

        if addAction:
            Utils.Print("adding {} to the security group".format(addAction))
            trans = self.defaultNode.pushMessage(self.contractAccount.name, "add", addAction, "--permission eosio@active")
            Utils.Print("add trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))

        if removeAction:
            Utils.Print("removing {} from the security group".format(removeAction))
            trans = self.defaultNode.pushMessage(self.contractAccount.name, "remove", removeAction, "--permission eosio@active")
            Utils.Print("remove trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))

        self.publishProcessNum += 1
        self.publishTrans = self.defaultNode.pushMessage(self.contractAccount.name, "publish", "[{}]".format(self.publishProcessNum), "--permission eosio@active")[1]
        Utils.Print("publish action trans: {}".format(json.dumps(self.publishTrans, indent=4, sort_keys=True)))
        return self.publishTrans

    def verifyParticipantsTransactionFinalized(self, transId = None):
        Utils.Print("Verify participants are in sync")
        if transId is None:
            transId = Node.getTransId(self.publishTrans)
        for part in self.participants:
            if part.waitForTransFinalization(transId) == None:
                Utils.errorExit("Transaction: {}, never finalized".format(trans))

    def verifyNonParticipants(self, transId = None):
        Utils.Print("Verify non-participants don't receive blocks")
        if transId is None:
            transId = Node.getTransId(self.publishTrans)
        publishBlock = self.defaultNode.getBlockIdByTransId(transId)
        prodLib = self.defaultNode.getBlockNum(blockType=BlockType.lib)
        waitForLib = prodLib + 3 * 12
        if self.defaultNode.waitForBlock(waitForLib, blockType=BlockType.lib) == None:
            Utils.errorExit("Producer did not advance lib the expected amount.  Starting lib: {}, exp lib: {}, actual state: {}".format(prodLib, waitForLib, self.defaultNode.getInfo()))
        producerHead = self.defaultNode.getBlockNum()

        for nonParticipant in self.nonParticipants:
            nonParticipantPostLIB = nonParticipant.getBlockNum(blockType=BlockType.lib)
            assert nonParticipantPostLIB < publishBlock, "Participants not in security group should not have advanced LIB to {}, but it has advanced to {}".format(publishBlock, nonParticipantPostLIB)
            nonParticipantHead = nonParticipant.getBlockNum()
            assert nonParticipantHead < producerHead, "Participants (that are not producers themselves) should not advance head to {}, but it has advanced to {}".format(producerHead, nonParticipantHead)

    def verifySecurityGroup(self, publishTrans = None):
        if publishTrans is None:
            publishTrans = self.publishTrans
        publishTransId = Node.getTransId(publishTrans)
        self.verifyParticipantsTransactionFinalized(publishTransId)
        self.verifyNonParticipants(publishTransId)

    def addToSecurityGroup(self):
        trans = self.editSecurityGroup([self.nonParticipants[0]])
        Utils.Print("Take a non-participant and make a participant. Now there are {} participants and {} non-participants".format(len(self.participants), len(self.nonParticipants)))
        return trans

    def remFromSecurityGroup(self):
        trans = self.editSecurityGroup(removeNodes=[self.participants[-1]])
        Utils.Print("Take a participant and make a non-participant. Now there are {} participants and {} non-participants".format(len(self.participants), len(self.nonParticipants)))
        return trans
