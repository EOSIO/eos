#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from GenericMessageNode import GenericMessageNode
from WalletMgr import WalletMgr
from Node import BlockType
import json
import signal
import time
from TestHelper import AppArgs
from TestHelper import TestHelper

###############################################################
# generic_message_test
#  Test uses test_generic_message_plugin to verify that generic_messages can be
#  sent between instances of nodeos
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit

def verifyBlockLog(expected_block_num, trimmedBlockLog):
    firstBlockNum = expected_block_num
    for block in trimmedBlockLog:
        assert 'block_num' in block, print("ERROR: eosio-blocklog didn't return block output")
        block_num = block['block_num']
        assert block_num == expected_block_num
        expected_block_num += 1
    Print("Block_log contiguous from block number %d to %d" % (firstBlockNum, expected_block_num - 1))


appArgs=AppArgs()
args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"})
Utils.Debug=args.v
pnodes=5
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
prodCount=1
killAll=args.clean_run
walletPort=TestHelper.DEFAULT_WALLET_PORT
totalNodes=pnodes

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
    generic_plugin = "--plugin eosio::test_generic_message::test_generic_message_api_plugin "
    specificExtraNodeosArgs={}
    pluginNodeNums=[]
    registeredTypes=[]
    typeSubstr="type"
    typeLen=len(typeSubstr)+1
    registeredTypes.append([ "type1", "type2" ])
    registeredTypes.append([ "type2", "type3" ])
    registeredTypes.append([ "type1", "type2", "type3" ])
    registeredTypes.append([ ])
    for nodeNum in range(0,len(registeredTypes)):
        types=[ " --register-generic-type " + type for type in registeredTypes[nodeNum] ]
        specificExtraNodeosArgs[nodeNum] = generic_plugin + "".join(types)

    Print("Stand up cluster")
    if cluster.launch(prodCount=prodCount, onlyBios=False, pnodes=pnodes, totalNodes=totalNodes, totalProducers=pnodes*prodCount,
                      specificExtraNodeosArgs=specificExtraNodeosArgs) is False:
        Utils.errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    pluginNodes=[ cluster.getNode(num) for num in range(0,len(registeredTypes)) ]

    time.sleep(20)

    node0P2pPort=9876
    nodeEndP2pPort=node0P2pPort + len(registeredTypes)

    def getEndpoints(nodeNum, ignoreNoSupport):
        node=cluster.getNode(nodeNum)
        ret=GenericMessageNode.getRegisteredTypes(node, ignoreNoSupport)
        assert ret != None and "nodes" in ret
        p2pEndpoints=ret["nodes"]
        Utils.Print("nodeNum: %d with %d endpoints" % (nodeNum, len(p2pEndpoints)))
        p2pEndpointDict={}
        for endpointTypes in p2pEndpoints:
            endpoint=endpointTypes["endpoint"]
            p2pRegisteredTypes=endpointTypes["types"]
            assert endpoint not in p2pEndpointDict, \
                Utils.errorExit("endpoint: %s repeated in nodeNum: %d enpoints: %s" % (endpoint, nodeNum, json.dumps(p2pEndpoints, indent=2, sort_keys=True)))
            p2pEndpointDict[endpoint]=p2pRegisteredTypes
        return p2pEndpointDict

    lhstr='localhost:'
    def extractPort(endpoint):
        endOfPort=len(lhstr) + 5
        assert endpoint.startswith(lhstr) and len(endpoint) >= endOfPort, \
            Utils.errorExit("endpoint: \"%s\" should start with \"localhost:XXXX\"" % (endpoint))
        portStr=endpoint[len(lhstr):endOfPort]
        Utils.Print(f"{portStr} from \"{endpoint}\"")
        return int(portStr)

    def portToNodeNum(port):
        assert port >= node0P2pPort and port <= nodeEndP2pPort, \
            Utils.errorExit("port: %d not in expected range: [%d, %d]" % (port, node0P2pPort, nodeEndP2pPort))
        return port-node0P2pPort

    def nodeNumToPort(nodeNum):
        start=0
        end=nodeEndP2pPort-node0P2pPort
        assert nodeNum >= start and nodeNum < end, \
            Utils.errorExit("nodeNum: %d not in expected range: [%d, %d)" % (nodeNum, start, end))
        return node0P2pPort+nodeNum

    def checkEndpoints(nodeNum, expectedNumEndpoints):
        p2pEndpoints=getEndpoints(nodeNum, ignoreNoSupport=True)
        Utils.Print("HERE checkEndpoints %d endpoints (%d) for nodeNum: %d" % (len(p2pEndpoints), expectedNumEndpoints, nodeNum))
        for endpoint, p2pRegisteredTypes in p2pEndpoints.items():
            port=extractPort(endpoint)
            p2pNodeNum=portToNodeNum(port)
            expectedRegisteredTypes=registeredTypes[p2pNodeNum].copy()
            for p2pRegisteredType in p2pRegisteredTypes:
                typeIndex=p2pRegisteredType.rfind(typeSubstr)
                assert typeIndex != -1 and typeIndex + typeLen < len(p2pRegisteredType), "type: \"%s\" does not contain the string: \"%s*\"" % (p2pRegisteredType, typeSubstr)
                reportedType=p2pRegisteredType[typeIndex:typeIndex+typeLen]
                assert reportedType in expectedRegisteredTypes, Utils.errorExit("type of: %s (%s) is not an expected type" % (reportedType, p2pRegisteredType))
                expectedRegisteredTypes.remove(reportedType)
            Utils.Print("endpoint: %s, types: %s" % (endpoint, json.dumps(p2pRegisteredTypes, indent=2, sort_keys=True)))
            assert len(expectedRegisteredTypes) == 0, Utils.errorExit("Endpoint: %s not reporting these expected types: [ %s ]" % (endpoint, ", ".join(expectedRegisteredTypes)))
        assert len(p2pEndpoints) == expectedNumEndpoints, Utils.errorExit("Nodenum: %d should only report %d endpoints with registered types" % (nodeNum))
        return p2pEndpoints

    def checkAllEndpoints(nodeNum, expectedNumEndpoints):
        nonEmptyEndpoints=checkEndpoints(nodeNum, expectedNumEndpoints)
        allEndpoints=getEndpoints(nodeNum, ignoreNoSupport=False)

        # all of the endpoints reported when ignoring no support endpoints, should report the same results for both
        for endpoint, p2pRegisteredTypes in nonEmptyEndpoints.items():
            assert endpoint in allEndpoints, \
                Utils.errorExit("endpoint: %s listed for nodeNum: %d with types: [ %s ], but not listed at all when reporting all nodes: %s"
                                % (endpoint, nodeNum, ", ".join(p2pRegisteredTypes), json.dumps(allEndpoints, indent=2, sort_keys=True)))
            allP2pRegisteredTypes=allEndpoints[endpoint]
            for type in p2pRegisteredTypes:
                assert type in allP2pRegisteredTypes
                allP2pRegisteredTypes.remove(type)
            assert len(allP2pRegisteredTypes) == 0
            allEndpoints.pop(endpoint)
        # and all of the remaining endpoints should have no types
        for endpoint, allP2pRegisteredTypes in allEndpoints.items():
            assert len(allP2pRegisteredTypes) == 0

    checkAllEndpoints(nodeNum=0, expectedNumEndpoints=2)
    checkAllEndpoints(nodeNum=1, expectedNumEndpoints=2)
    checkAllEndpoints(nodeNum=2, expectedNumEndpoints=2)
    checkAllEndpoints(nodeNum=3, expectedNumEndpoints=3)

    def sendType1(nodeNum, data, endpoints=[]):
        ret=GenericMessageNode.sendType1(cluster.getNode(nodeNum), f1=data["f1"], f2=data["f2"], f3=data["f3"], endpoints=endpoints, silentErrors=False)
        assert ret is not None
        return ret

    def sendType2(nodeNum, data, endpoints=[]):
        ret=GenericMessageNode.sendType2(cluster.getNode(nodeNum), f1=data["f1"], f2=data["f2"], f3=data["f3"], f4=data["f4"], endpoints=endpoints, silentErrors=False)
        assert ret is not None
        return ret

    def sendType3(nodeNum, data, endpoints=[]):
        ret=GenericMessageNode.sendType3(cluster.getNode(nodeNum), f1=data["f1"], endpoints=endpoints, silentErrors=False)
        assert ret is not None
        return ret

    def endpointDesc(nodeNum):
        return "%s%d" % (lhstr, nodeNumToPort(nodeNum))

    nodeNum=0
    desc=endpointDesc(nodeNum)
    node0Type1All1= { "f1": True, "f2": desc, "f3": 0xfedcba9876543210 }
    sendType1(nodeNum, node0Type1All1)
    node0Type2All1= { "f1": 0xfedcba9876543211, "f2": False, "f3": desc, "f4": "extra data" }
    sendType2(nodeNum, node0Type2All1)
    node0Type3All1= { "f1": desc }
    sendType3(nodeNum, node0Type3All1)

    nodeNum=1
    desc=endpointDesc(nodeNum)
    node1Type1All1= { "f1": False, "f2": desc, "f3": 0xfedcba9876543212 }
    sendType1(nodeNum, node1Type1All1)
    node1Type2All1= { "f1": 0xfedcba9876543213, "f2": True, "f3": desc, "f4": "extra data2" }
    sendType2(nodeNum, node1Type2All1)
    node1Type3All1= { "f1": desc }
    sendType3(nodeNum, node1Type3All1)

    nodeNum=2
    desc=endpointDesc(nodeNum)
    node2Type1All1= { "f1": True, "f2": desc, "f3": 0xfedcba9876543213 }
    sendType1(nodeNum, node2Type1All1)
    node2Type2All1= { "f1": 0xfedcba9876543214, "f2": False, "f3": desc, "f4": "extra data3" }
    sendType2(nodeNum, node2Type2All1)
    node2Type3All1= { "f1": desc }
    sendType3(nodeNum, node2Type3All1)

    nodeNum=3
    desc=endpointDesc(nodeNum)
    node3Type1All1= { "f1": False, "f2": desc, "f3": 0xfedcba9876543214 }
    sendType1(nodeNum, node3Type1All1)
    node3Type2All1= { "f1": 0xfedcba9876543215, "f2": True, "f3": desc, "f4": "extra data4" }
    sendType2(nodeNum, node3Type2All1)
    node3Type3All1= { "f1": desc }
    sendType3(nodeNum, node3Type3All1)

    def returnKey(d, key, valueNotNone=True):
        assert d is not None and key in d, \
            Utils.errorExit("dict should contain key: %s but it does not. dict: %s" % (key, json.dumps(d, indent=2, sort_keys=True)))
        val=d[key]
        assert not valueNotNone or val is not None, \
            Utils.errorExit("dict should contain a non-None value for key: %s but it does not. dict: %s" % (key, json.dumps(d, indent=2, sort_keys=True)))
        return val

    def popNextMsg(msgs):
        if len(msgs) == 0:
            Utils.errorExit("No more to pop.")
        oldMsgs=msgs.copy()
        msg=msgs.pop(0)
        endpoint=returnKey(msg,"endpoint")
        data=returnKey(msg,"data")
        return (endpoint, data)

    def verifyEndpoint(endpoint, nodeNum):
        desc=endpointDesc(nodeNum)
        assert endpoint.startswith(desc), \
            Utils.errorExit("Expected to have received data from node: %d, but did not. " % (nodeNum) +
                            "Expected endpoint: %s, received endpoint: %s" % (desc, endpoint))

    def verifyListContent(l1, l2, context):
        minSize=min(len(l1), len(l2))
        for index in range(0, minSize):
            ret=verifyContent(l1[index],l2[index],context+"[%d]" % (index))
            if ret is not None:
                return ret
        if len(l1) != len(l2):
            return "at %s lists are different sizes. lhs: [ %s ], rhs: [ %s ]" % (context, ", ".join(l1), ", ".join(l2))
        return None

    def verifyDictionaryContent(d1, d2, context):
        d1Keys=list(d1)
        d2Keys=list(d2)
        d1Keys.sort()
        d2Keys.sort()
        minSize=min(len(d1Keys), len(d2Keys))
        for index in range(0,minSize):
            key=d1Keys[index]
            if key not in d2Keys:
                return "at %s lhs has key: %s and rhs does not. lhs: [ %s ], rhs: [ %s ]" % (context, key, json.dumps(d1, indent=2, sort_keys=True), json.dumps(d2, indent=2, sort_keys=True))
            ret=verifyContent(d1[key], d2[key], context+"[%s]" % (key))
            if ret is not None:
                return ret
        if len(d1Keys) != len(d2Keys):
            return "at %s dictionaries have different keys. lhs: %s, rhs: %s" % (context, json.dumps(d1, indent=2, sort_keys=True), json.dumps(d2, indent=2, sort_keys=True))
        return None

    def massageTypes(lhs, rhs):
        typeLhs=type(lhs)
        typeRhs=type(rhs)
        def asInt(s):
            try:
                return int(s)
            except ValueError:
                return None

        if typeLhs is str and typeRhs is int:
            lhsAsInt=asInt(lhs)
            if lhsAsInt is not None:
                return (lhsAsInt, rhs)
        elif typeLhs is int and typeRhs is str:
            rhsAsInt=asInt(rhs)
            if rhsAsInt is not None:
                return (lhs, rhsAsInt)
        return (lhs, rhs)

    def verifyContent(lhsOrig, rhsOrig, context):
        (lhs, rhs)=massageTypes(lhsOrig, rhsOrig)
        if type(lhs) != type(rhs):
            return "at %s values are of different types. lhs: %s (%s), rhs: %s (%s)" % (context, lhs, type(lhs), rhs, type(rhs))
        if isinstance(lhs, list):
            return verifyListContent(lhs, rhs, context)
        if isinstance(lhs, dict):
            return verifyDictionaryContent(lhs, rhs, context)
        if lhs != rhs:
            return "at %s lhs and rhs differ. lhs: %s, rhs: %s." % (context, lhs, rhs, context)
        return None

    def verifyData(lhs, rhs):
        ret=verifyContent(lhs, rhs, "")
        if ret is not None:
            Utils.errorExit("lhs and rhs don't match: %s\nlhs: %s\n\nrhs: %s" %
                            (ret, json.dumps(lhs, indent=2, sort_keys=True),
                             json.dumps(rhs, indent=2, sort_keys=True)))

    def verifyMsg(msgs, nodeNum, expectedData):
        (endpoint,data)=popNextMsg(msgs)
        Utils.Print("endpoint: %s, data: %s" % (endpoint, json.dumps(data, indent=2, sort_keys=True)))
        verifyEndpoint(endpoint, nodeNum)
        verifyData(data, expectedData)

    receivedData=GenericMessageNode.getReceivedData(cluster.getNode(0))
    msgs=returnKey(receivedData,"messages")
    verifyMsg(msgs, 1, node1Type1All1)
    verifyMsg(msgs, 1, node1Type2All1)
    verifyMsg(msgs, 2, node2Type1All1)
    verifyMsg(msgs, 2, node2Type2All1)
    verifyMsg(msgs, 3, node3Type1All1)
    verifyMsg(msgs, 3, node3Type2All1)

    receivedData=GenericMessageNode.getReceivedData(cluster.getNode(1))
    msgs=returnKey(receivedData,"messages")
    verifyMsg(msgs, 0, node0Type2All1)
    verifyMsg(msgs, 0, node0Type3All1)
    verifyMsg(msgs, 2, node2Type2All1)
    verifyMsg(msgs, 2, node2Type3All1)
    verifyMsg(msgs, 3, node3Type2All1)
    verifyMsg(msgs, 3, node3Type3All1)

    receivedData=GenericMessageNode.getReceivedData(cluster.getNode(2))
    msgs=returnKey(receivedData,"messages")
    verifyMsg(msgs, 0, node0Type1All1)
    verifyMsg(msgs, 0, node0Type2All1)
    verifyMsg(msgs, 0, node0Type3All1)
    verifyMsg(msgs, 1, node1Type1All1)
    verifyMsg(msgs, 1, node1Type2All1)
    verifyMsg(msgs, 1, node1Type3All1)
    verifyMsg(msgs, 3, node3Type1All1)
    verifyMsg(msgs, 3, node3Type2All1)
    verifyMsg(msgs, 3, node3Type3All1)

    receivedData=GenericMessageNode.getReceivedData(cluster.getNode(3))
    msgs=returnKey(receivedData,"messages")
    if len(msgs) > 0:
        Utils.errorExit("node3 should not have returned any messages: %s" % (json.dumps(msgs, indent=2, sort_keys=True)))

    nodes=cluster.getNodes()
    for node in nodes:
        node.getInfo()
        node.kill(signal.SIGTERM)
    cluster.reportStatus()
    testSuccessful=True

finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exit(0)
