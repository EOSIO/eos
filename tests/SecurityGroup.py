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
        self.nonParticipants = copy.copy(nonParticipants)
        if Utils.Debug: Utils.Print("Creating SecurityGroup with the following nonParticipants: []".format(SecurityGroup.createAction(self.nonParticipants)))
        self.defaultNode = defaultNode if defaultNode else nonParticipants[0]
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

    # create the action payload for an add or remove action
    @staticmethod
    def createAction(nodes):
        return None if len(nodes) == 0 else \
            "[[{}]]".format(','.join(['"{}"'.format(node.getParticipant()) for node in nodes]))

    # sends actions to add/remove the provided nodes to/from the network's security group
    def editSecurityGroup(self, addNodes=[], removeNodes=[]):

        def copyIfNeeded(nodes):
            # doing deep copy in case the passed in list IS one of our lists, which will be adjusted
            if nodes is self.participants or nodes is self.nonParticipants:
                return copy.copy(nodes)
            return nodes

        addAction = SecurityGroup.createAction(addNodes)
        self.__addParticipants(copyIfNeeded(addNodes))

        removeAction = SecurityGroup.createAction(removeNodes)
        self.__remParticipants(copyIfNeeded(removeNodes))

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

    # verify that the transaction ID is found, and finalized, in every node in the participants list
    def verifyParticipantsTransactionFinalized(self, transId):
        Utils.Print("Verify participants are in sync")
        assert transId
        for part in self.participants:
            if part.waitForTransFinalization(transId) == None:
                Utils.errorExit("Transaction: {}, never finalized".format(trans))

    # verify that the block for the transaction ID is never finalized in nonParticipants
    def verifyNonParticipants(self, transId):
        Utils.Print("Verify non-participants don't receive blocks")
        assert transId
        publishBlock = self.defaultNode.getBlockIdByTransId(transId)

        # first ensure that enough time has passed that the nonParticipant is not just trailing behind
        prodLib = self.defaultNode.getBlockNum(blockType=BlockType.lib)
        waitForLib = prodLib + 3 * 12
        if self.defaultNode.waitForBlock(waitForLib, blockType=BlockType.lib) == None:
            Utils.errorExit("Producer did not advance lib the expected amount.  Starting lib: {}, exp lib: {}, actual state: {}".format(prodLib, waitForLib, self.defaultNode.getInfo()))
        producerHead = self.defaultNode.getBlockNum()

        # verify each nonParticipant in the list has not advanced its lib to the publish block, since the block that would cause it to become finalized would
        # never have been forwarded to a nonParticipant
        for nonParticipant in self.nonParticipants:
            nonParticipantPostLIB = nonParticipant.getBlockNum(blockType=BlockType.lib)
            assert nonParticipantPostLIB < publishBlock, "Participants not in security group should not have advanced LIB to {}, but it has advanced to {}".format(publishBlock, nonParticipantPostLIB)
            nonParticipantHead = nonParticipant.getBlockNum()
            assert nonParticipantHead < producerHead, "Participants (that are not producers themselves) should not advance head to {}, but it has advanced to {}".format(producerHead, nonParticipantHead)

    # verify that the participants' and nonParticipants' nodes are consistent based on the publish transaction
    def verifySecurityGroup(self, publishTrans = None):
        if publishTrans is None:
            publishTrans = self.publishTrans
        publishTransId = Node.getTransId(publishTrans)
        self.verifyParticipantsTransactionFinalized(publishTransId)
        self.verifyNonParticipants(publishTransId)

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
