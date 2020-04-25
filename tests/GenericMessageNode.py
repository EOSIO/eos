import json
from Node import Node
from testUtils import ReturnType
from testUtils import Utils

class GenericMessageNode:
    @staticmethod
    def resource_name():
        return "test_generic_message"

    @staticmethod
    def getRegisteredTypes(node, ignoreNoSupport=True, silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(ignoreNoSupport, bool))

        payload="{ \"ignore_no_support\": %s }" % (Utils.getJsonBoolString(ignoreNoSupport))
        return node.processCurlCmd(GenericMessageNode.resource_name(), "registered_types", payload, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=exitMsg, returnType=returnType)

    @staticmethod
    def getReceivedData(node, endpoints=[], silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(endpoints, list))

        payload="{ %s }" % (", ".join(endpoints))
        return node.processCurlCmd(GenericMessageNode.resource_name(), "received_data", payload, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=exitMsg, returnType=returnType)

    @staticmethod
    def sendType1(node, f1, f2, f3, endpoints=[], silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(f1, bool))
        assert(isinstance(f2, str))
        assert(isinstance(f3, int))

        data="{ \"f1\": %s, \"f2\": \"%s\", \"f3\": %d  }" % (Utils.getJsonBoolString(f1), f2, f3)
        endpointsStr="[]" if len(endpoints) == 0 else "[ \"%s\" ]" % ("\", \"".join(endpoints))
        payload="{ \"endpoints\": %s, \"data\": %s }" % (endpointsStr, data)
        return node.processCurlCmd(GenericMessageNode.resource_name(), "send_type1", payload, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=exitMsg, returnType=returnType)

    @staticmethod
    def sendType2(node, f1, f2, f3, f4, endpoints=[], silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(f1, int))
        assert(isinstance(f2, bool))
        assert(isinstance(f3, str))
        assert(isinstance(f4, str))

        data="{ \"f1\": %d , \"f2\": %s, \"f3\": \"%s\", \"f4\": \"%s\" }" % (f1, Utils.getJsonBoolString(f2), f3, f4)
        endpointsStr="[]" if len(endpoints) == 0 else "[ \"%s\" ]" % ("\", \"".join(endpoints))
        payload="{ \"endpoints\": %s, \"data\": %s }" % (endpointsStr, data)
        return node.processCurlCmd(GenericMessageNode.resource_name(), "send_type2", payload, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=exitMsg, returnType=returnType)

    @staticmethod
    def sendType3(node, f1, endpoints=[], silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(f1, str))

        data="{ \"f1\": \"%s\"  }" % (f1)
        endpointsStr="[]" if len(endpoints) == 0 else "[ \"%s\" ]" % ("\", \"".join(endpoints))
        payload="{ \"endpoints\": %s, \"data\": %s }" % (endpointsStr, data)
        return node.processCurlCmd(GenericMessageNode.resource_name(), "send_type3", payload, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=exitMsg, returnType=returnType)
