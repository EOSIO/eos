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
# Script implements Privacy Test Case #1. It pairs up producers with p2p connections with relay nodes
# and the relay nodes connected to at least 2 or more API nodes.  The producers and relay nodes are
# added to the security Group and then it validates they are in sync and the api nodes do not receive
# blocks.  Then it adds all but one api nodes and verifies they are in sync with producers, then all
# nodes are added and verifies that all nodes are in sync.
#
# NOTE: A relay node is a node that an entity running a producer uses to prevent outside nodes from
# affecting the producing node. An API Node is a node that is setup for the general community to
# connect to and will have more p2p connections. This script doesn't necessarily setup the API nodes
# the way that they are setup in the real world, but it is referencing them this way to explain what
# the test is intending to verify.
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

cluster=Cluster(host=TestHelper.LOCAL_HOST, port=port, walletd=True, walletMgr=WalletMgr(True, port=walletPort))
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
        topo[pairedRelayNodeNum] = [x for x in range(pnodes, pnodes + relayNodes) if x != pairedRelayNodeNum]
        # p2p connections between relay and all api nodes
        topo[pairedRelayNodeNum].extend(apiNodeNums)
    Utils.Print("topo: {}".format(json.dumps(topo, indent=4, sort_keys=True)))

    # adjust prodCount to ensure that lib trails more than 1 block behind head
    prodCount = 1 if pnodes > 1 else 2

    if cluster.launch(pnodes=pnodes, totalNodes=totalNodes, prodCount=prodCount, onlyBios=False, dontBootstrap=dontBootstrap, configSecurityGroup=True, topo=topo, printInfo=True) is False:
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
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
