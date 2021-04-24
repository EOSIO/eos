#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import BlockType
from TestHelper import TestHelper

import json
import signal
import time
import re

###############################################################
# privacy_scenario_3 test
#
# See https://blockone.atlassian.net/wiki/spaces/EOSIOProdEng/pages/1434419366/Private+Chain+Access+Test+Plan
# For more details on test
#
###############################################################


Print=Utils.Print

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v"})

# 2 producers (will reference as prod1 and prod2)
pnodes=2
# 2 producers + 2 non-producers (will reference as node1 and node2)
totalNodes=4
unstartedNodes=1
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
Utils.Debug=args.v

testSuccessful=False
cluster=Cluster(host=TestHelper.LOCAL_HOST, port=TestHelper.DEFAULT_PORT, walletd=True, walletMgr=WalletMgr(True))
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=True, cleanup=True)

    # prod1, prod2, and node1 connected in a mesh
    # node2 is not started and is only connected to node1
    topo = { 0 : [1,2], 
             1 : [0,2], 
             2 : [0,1], 
             3 : [2]}
    Utils.Print("topo: {}".format(json.dumps(topo, indent=4, sort_keys=True)))

    extraNodeosArgs=" --plugin eosio::net_api_plugin "

    if not cluster.launch(pnodes=pnodes, totalNodes=totalNodes, unstartedNodes=unstartedNodes, configSecurityGroup=True, extraNodeosArgs=extraNodeosArgs, topo=topo, printInfo=True):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    cluster.biosNode.kill(signal.SIGTERM)

    prod1 = cluster.getNode(0)
    prod2 = cluster.getNode(1)
    securityGroup = cluster.getSecurityGroup()

    # add prod1 and prod2 to turn on security group, in one action *
    trans = securityGroup.editSecurityGroup(addNodes=[prod1, prod2])
    # ecord block number (rbn1) transaction was added in
    trans_block_num = trans["processed"]["block_num"]
    Print("security group update [[\"node1\", \"node2\"]] published on block {}".format(trans_block_num))

    prod1.waitForIrreversibleBlock(trans_block_num)
    prod2.waitForIrreversibleBlock(trans_block_num)

    block1 = prod1.getBlock(trans_block_num)
    block2 = prod2.getBlock(trans_block_num)

    # checking that there is no fork by comparing block ids
    block1_id = block1["id"]
    block2_id = block2["id"]
    # after recorded block number (rbn1) becomes lib, verify production cycle continues without a fork
    assert block1_id is not None and block1_id == block2_id

    prod1_head    = prod1.getHeadBlockNum()
    # on this point head should be already ahead of API node #1, but let's add idle wait just to ensure
    prod1.waitForBlock(prod1_head + 20)

    apiNode1 = cluster.getNode(2)
    apiNode1_head = apiNode1.getHeadBlockNum()
    Print("Node1 currently at {}".format(apiNode1_head))
    Print("producer 0 after security group transaction block became LIB: {}".format(prod1_head))
    # non-producing nodes do not receive the block that moved lib to or past the recorded block
    Print("Node1 head = {} Prod1 head = {}".format(apiNode1_head, prod1_head))
    assert apiNode1_head <= prod1_head

    trans = securityGroup.editSecurityGroup(addNodes=[apiNode1])
    # record block number (rbn1) transaction was added in
    trans_block_num = trans["processed"]["block_num"]
    Print("security group update [[\"node3\"]] published on block ".format(trans_block_num))
    securityGroup.verifyParticipantsTransactionFinalized(trans["transaction_id"])

    cluster.launchUnstarted(cachePopen=True)

    prod1.waitForBlock(trans_block_num + 50)

    apiNode2 = cluster.getNode(3)
    # after last recorded block number (rbn2) is made irreversible, verify
    # starting node2 results in connection being made, but no blocks or transactions being sent
    Print("Node2 head block = {}".format(apiNode2.getHeadBlockNum()))
    assert apiNode2.getHeadBlockNum() == 1
    
    apiNode1ListenEP = apiNode1.getListenEndpoint()
    apiNode1ListenEP.replace("localhost", "0.0.0.0")
    Print("Node1 p2p-listen-endpoint is {}".format(apiNode1ListenEP))

    foundConnection = False
    for connection in apiNode2.getConnections():
        p2p_address = connection["last_handshake"]["p2p_address"]
        match = re.search("([a-z\.0-9]+:[0-9]{2,5}) ", p2p_address)
        if match and len(match.groups()):
            cur_address = match.groups()[0].replace("localhost", "0.0.0.0")
            Print("current p2p_address = " + cur_address)
            if cur_address == str(apiNode1ListenEP):
                foundConnection = True
                break
    
    assert foundConnection

    # add node2 to security group *
    trans = securityGroup.editSecurityGroup(addNodes=[apiNode2])
    # ecord block number (rbn3) transaction was added in
    trans_block_num = trans["processed"]["block_num"]
    Print("security group update [[\"node4\"]] published on block {}".format(trans_block_num))

    # before last recorded block number (rbn3) is made irreversible, verify
    # node2 still receives no blocks or transactions
    while apiNode1.getIrreversibleBlockNum() < trans_block_num - 12:
        Print("Node2 head block = {}".format(apiNode2.getHeadBlockNum()))
        assert apiNode2.getHeadBlockNum() == 1
        time.sleep(0.5)

    apiNode1.waitForIrreversibleBlock(trans_block_num)

    #######################################################################
    # Workaround.
    # Restart shouldn't be needed here but due to bug in nodeos we have to.
    # this supposed to be fixed in nodeos and removed
    apiNode2.kill(signal.SIGTERM)
    apiNode2.relaunch(cachePopen=True)
    #######################################################################

    # after last recorded block number (rbn3) is made irreversible, verify 
    # node2 connects to node1
    # node2 starts syncing
    apiNode2.waitForBlock(trans_block_num, reportInterval=1)

    # after node2 is in sync with node1, verify
    # incoming blocks are sent back to node1 --> for this purpose we use apiNode2 to edit security group
    trans = securityGroup.editSecurityGroup(removeNodes=[apiNode1], node=apiNode2)
    # ecord block number (rbn4) transaction was added in
    trans_block_num = trans["processed"]["block_num"]
    Print("security group removal of [[\"node3\"]] published on block {}".format(trans_block_num))

    # before last recorded block number (rbn4) is made irreversible, verify
    # node1 and node2 still stay in sync
    apiNode1_head = apiNode1.getHeadBlockNum()
    Print("Node1 head block = {}".format(apiNode1_head))
    apiNode2_head = apiNode2.getHeadBlockNum()
    Print("Node2 head block = {}".format(apiNode2_head))
    apiNode1_InSync = False
    apiNode2_InSync = False
    while prod1.getIrreversibleBlockNum() < trans_block_num:
        temp = apiNode1.getHeadBlockNum()
        if apiNode1_head < temp:
            apiNode1_InSync = True
            apiNode1_head = temp
        temp = apiNode2.getHeadBlockNum()
        if apiNode2_head < temp:
            apiNode2_InSync = True
            apiNode2_head = temp
        time.sleep(0.5)
    
    Print("Node1 head = {}".format(apiNode1_head))
    assert trans_block_num < apiNode1_head
    Print("Node2 head = {}".format(apiNode2_head))
    assert trans_block_num < apiNode2_head
    Print("OK: Node1 didn't advance. head still at {}".format(apiNode1_head))
    assert apiNode1_InSync
    Print("OK: Node2 didn't advance. head still at {}".format(apiNode2_head))
    assert apiNode2_InSync

    # after last recorded block number (rbn4) is made irreversible, verify
    # node1 and node2 do not advance their LIB to rbn4 and do not continue to receive blocks
    waitSec = 6
    Print("Waiting for {} seconds to ensure api nodes LIB doesn't move".format(waitSec))
    time.sleep(waitSec)

    Print("Node1 LIB = {}".format(apiNode1.getIrreversibleBlockNum()))
    assert apiNode1.getIrreversibleBlockNum() < trans_block_num
    Print("Node2 LIB = {}".format(apiNode2.getIrreversibleBlockNum()))
    assert apiNode2.getIrreversibleBlockNum() < trans_block_num

    # add node1 back to security group *
    trans = securityGroup.editSecurityGroup(addNodes=[apiNode1])
    # record block number (rbn5) transaction was added in
    trans_block_num = trans["processed"]["block_num"]

    # before last recorded block number (rbn5) is made irreversible, verify
    # node1 and node2 do not receive blocks
    #comparing here with trans_block_num - 12 to avoid possible flakiness when LIB advances inside while
    while prod1.getIrreversibleBlockNum() <= trans_block_num - 12:
        assert apiNode1.getHeadBlockNum() == apiNode1_head
        assert apiNode2.getHeadBlockNum() == apiNode2_head
        Print("Node1 head = {}".format(apiNode1.getHeadBlockNum()))
        Print("Node2 head = {}".format(apiNode2.getHeadBlockNum()))
        time.sleep(0.5)

    # after last recorded block number (rbn5) is made irreversible, verify
    # node1 and node2 start to receive blocks
    assert apiNode1.waitForLibToAdvance()
    Print("After {} became LIB Node1 head = {} Node2 head = {}".format(trans_block_num, apiNode1.getHeadBlockNum(), apiNode2.getHeadBlockNum()))
    #######################################################################
    # Workaround.
    # Restart shouldn't be needed here but due to bug in net_plugin we have to.
    # this supposed to be fixed in nodeos and removed
    apiNode2.kill(signal.SIGTERM)
    apiNode2.relaunch(cachePopen=True)
    #######################################################################
    apiNode2.waitForBlock(trans_block_num, timeout=240, blockType=BlockType.lib, reportInterval=1)
    Print("apiNode2 LIB block is now {}".format(trans_block_num))
    apiNode2.waitForHeadToAdvance()

    # remove node2 back to security group *
    trans = securityGroup.editSecurityGroup(removeNodes=[apiNode2])
    # record block number (rbn6) transaction was added in
    trans_block_num = trans["processed"]["block_num"]

    # after node1’s LIB advances to rbn5, verify
    # node1 and node2 continue to sync
    apiNode2_head = apiNode2.getHeadBlockNum()
    Print("Node1 head = {} Node2 head = {}".format(apiNode2_head, apiNode1.getHeadBlockNum()))
    apiNode2_Advancing = False
    while apiNode1.getIrreversibleBlockNum() < trans_block_num:
        temp = apiNode2.getHeadBlockNum()
        if apiNode2_head < temp:
            apiNode2_Advancing = True
            apiNode2_head = temp
        time.sleep(0.5)
    
    Print("Node1 head = {} Node2 head = {}".format(apiNode2_head, apiNode1.getHeadBlockNum()))
    assert apiNode2_Advancing
    
    # after last recorded block number (rbn6) is made irreversible, verify
    # node2 receives no blocks
    apiNode1_head = apiNode1.getHeadBlockNum()
    Print("Waiting for {} seconds to ensure Node2 head block doesn't move".format(waitSec))
    time.sleep(waitSec)

    Print("Node2 head = {} Node1 LIB = {}".format(apiNode2.getHeadBlockNum(), apiNode1_head))
    assert apiNode2.getHeadBlockNum() < apiNode1_head

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)