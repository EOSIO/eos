import copy
import json
import Node

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

    def securityGroup(self, addNodes=[], removeNodes=[]):

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
        trans = self.defaultNode.pushMessage(self.contractAccount.name, "publish", "[{}]".format(self.publishProcessNum), "--permission eosio@active")
        Utils.Print("publish action trans: {}".format(json.dumps(trans, indent=4, sort_keys=True)))
        return trans
