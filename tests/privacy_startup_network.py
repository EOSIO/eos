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
if totalNodes >= minTotalNodes:
    apiNodes += totalNodes - minTotalNodes
else:
    Utils.Print("Requested {} total nodes, but since the minumum number of API nodes is {}, there will be {} total nodes".format(args.n, apiNodes, totalNodes))

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
        # p2p connection between producer and relay
        topo[producerNum] = [pairedRelayNodeNum]
        # p2p connections between relays
        topo[pairedRelayNodeNum] = [x + producerNum for x in range(pnodes) if x != producerNum]
        # p2p connections between relay and all api nodes
        topo[pairedRelayNodeNum].extend(apiNodeNums)
    Utils.Print("topo: {}".format(json.dumps(topo, indent=4, sort_keys=True)))

    # adjust prodCount to ensure that lib trails more than 1 block behind head
    prodCount = 1 if pnodes > 1 else 2

    if cluster.launch(pnodes=pnodes, totalNodes=totalNodes, prodCount=prodCount, onlyBios=False, dontBootstrap=dontBootstrap, configSecurityGroup=True, topo=topo) is False:
        cmdError("launcher")
        errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    cluster.biosNode.kill(signal.SIGTERM)

    producers = [cluster.getNode(x) for x in range(pnodes) ]
    relays = [cluster.getNode(pnodes + x) for x in range(pnodes) ]
    apiNodes = [cluster.getNode(x) for x in apiNodeNums]

    securityGroup = cluster.getSecurityGroup()
    cluster.reportInfo()

    Utils.Print("Add all producers and relay nodes to security group")
    prodsAndRelays = copy.copy(producers)
    prodsAndRelays.extend(relays)
    securityGroup.editSecurityGroup(prodsAndRelays)
    securityGroup.verifySecurityGroup()

    allButLastApiNodes = apiNodes[:-1]
    lastApiNode = [apiNodes[-1]]

    Utils.Print("Add all but last API node and verify they receive blocks and the last API node does not")
    securityGroup.editSecurityGroup(addNodes=allButLastApiNodes)
    securityGroup.verifySecurityGroup()

    Utils.Print("Add the last API node and verify it receives blocks")
    securityGroup.editSecurityGroup(addNodes=lastApiNode)
    securityGroup.verifySecurityGroup()

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
