#!/usr/bin/env python3

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
import os.path
import threading

###############################################################
# undo_stack_test.py
#
# tests undo_stack functionality
# test starts several chainbase and rocksdb instances
# executes get_table_test contract that stores/modifies/erases some data in multi index
# checks that rocksdb instances created undo_stack files and multi index values persist after restart
#
###############################################################

Print=Utils.Print

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v"})

# 9 producers (6 chainbase, 4 rocksdb)
pnodes=10
totalNodes=pnodes
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
Utils.Debug=args.v

testSuccessful=False
cluster=Cluster(host=TestHelper.LOCAL_HOST, port=TestHelper.DEFAULT_PORT, walletd=True, walletMgr=WalletMgr(True))
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=True, cleanup=True)

    # 8th and 9th nodes are running same producer 
    extraProducerAccounts = Cluster.createAccountKeys(1)
    duplicateProducerAccount = extraProducerAccounts[0]

    prodEntry = {
        'key': duplicateProducerAccount,
        'names': ['dup.producer']
    }
    manualProducerNodeConf = {
        pnodes - 3 : prodEntry,
        pnodes - 2 : prodEntry,
        pnodes - 1 : prodEntry
    }

    # last 3 nodes have rocksdb backing store
    specificExtraNodeosArgs = {}
    specificExtraNodeosArgs[pnodes - 4] = " --backing-store=rocksdb"
    specificExtraNodeosArgs[pnodes - 3] = " --backing-store=rocksdb"
    specificExtraNodeosArgs[pnodes - 2] = " --backing-store=rocksdb"
    specificExtraNodeosArgs[pnodes - 1] = " --backing-store=rocksdb"
    
    if not cluster.launch(pnodes=pnodes, totalNodes=totalNodes, printInfo=True,
                          specificExtraNodeosArgs=specificExtraNodeosArgs,
                          manualProducerNodeConf=manualProducerNodeConf):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    cluster.biosNode.kill(signal.SIGTERM)

    # last 4 are rocksdb instances
    rocksdbNodes = cluster.getNodes()[-4:]

    # reusing get_table_test contract that has multi_index add/delete/modify methods
    contractOwner = cluster.eosioAccount
    contract = "get_table_test"
    Print("Publishing {} contract".format(contract))

    node = cluster.getNode(0)
    publishTrans = node.publishContract(contractOwner, "unittests/test-contracts/get_table_test/", "{}.wasm".format(contract), "{}.abi".format(contract), waitForTransBlock=True)
    assert publishTrans, Print("failed to publish {} contract".format(contract))

    opts="--permission {}@active".format(contractOwner.name)
    
    success, msg = node.pushMessage(contractOwner.name, 'addnumobj', '["2"]', opts)
    assert success, Print("error calling addnumobj. msg: {}".format(msg))
    
    node.pushMessage(contractOwner.name, 'addnumobj', '["5"]', opts)
    assert success, Print("error calling addnumobj. msg: {}".format(msg))
   
    node.pushMessage(contractOwner.name, 'addnumobj', '["7"]', opts)
    assert success, Print("error calling addnumobj. msg: {}".format(msg))


    success, msg = node.pushMessage(contractOwner.name, 'addhashobj', '["firstinput"]', opts)
    assert success, Print("error calling addhashobj. msg: {}".format(msg))

    success, msg = node.pushMessage(contractOwner.name, 'addhashobj', '["secondinput"]', opts)
    assert success, Print("error calling addhashobj. msg: {}".format(msg))

    success, msg = node.pushMessage(contractOwner.name, 'addhashobj', '["thirdinput"]', opts)
    assert success, Print("error calling addhashobj. msg: {}".format(msg))


    #checking integer objects, created above
    rows=node.getTableRows(contractOwner.name, contractOwner.name, 'numobjs')
    Print("numobjs rows: {}".format(json.dumps(rows, indent=4, sort_keys=True)))
    assert len(rows) == 3, Print("rows number doesn't match: {} != 3".format(len(rows)))
    assert rows[0]["sec64"] == 2, Print("row[0] doesn't match. 2 != {}".format(rows[0]["sec64"]))
    assert rows[1]["sec64"] == 5, Print("row[1] doesn't match. 5 != {}".format(rows[1]["sec64"]))
    assert rows[2]["sec64"] == 7, Print("row[2] doesn't match. 7 != {}".format(rows[3]["sec64"]))

    # checking hash objects, created above
    rows=node.getTableRows(contractOwner.name, contractOwner.name, 'hashobjs')
    Print("hashobjs rows: {}".format(json.dumps(rows, indent=4, sort_keys=True)))
    assert len(rows) == 3, Print("rows number doesn't match: {} != 3".format(len(rows)))
    assert rows[0]["hash_input"] == "firstinput", Print("row[0] doesn't match. firstinput != {}".format(rows[0]["hash_input"]))
    assert rows[1]["hash_input"] == "secondinput", Print("row[1] doesn't match. secondinput != {}".format(rows[1]["hash_input"]))
    assert rows[2]["hash_input"] == "thirdinput", Print("row[2] doesn't match. thirdinput != {}".format(rows[3]["hash_input"]))


    # increments value with first index
    success, msg = node.pushMessage(contractOwner.name, 'modifynumobj', '["0"]', opts)
    assert success, Print("error calling modifynumobj. msg: {}".format(msg))
    # erases value with last index
    success, msg = node.pushMessage(contractOwner.name, 'erasenumobj', '["2"]', opts)
    assert success, Print("error calling erasenumobj. msg: {}".format(msg))

    # waiting till transaction will appear in rocksdb nodes
    for rocksdbNode in rocksdbNodes:
        assert rocksdbNode.waitForTransInBlock(msg["transaction_id"]), Print("Transaction didn't reach node {}".format(rocksdbNode.nodeId))
    blockId = node.getBlockNumByTransId(msg["transaction_id"])

    rows=node.getTableRows(contractOwner.name, contractOwner.name, 'numobjs')
    Utils.Print("modified numobjs rows: {}".format(json.dumps(rows, indent=4, sort_keys=True)))
    assert len(rows) == 2, Print("rows number doesn't match: {} != 2".format(len(rows)))
    assert rows[0]["sec64"] == 3, Print("row[0] doesn't match. 3 != {}".format(rows[0]["sec64"]))
    assert rows[1]["sec64"] == 5, Print("row[1] doesn't match. 5 != {}".format(rows[1]["sec64"]))

    assert cluster.killSomeEosInstances(len(cluster.getNodes()), killSignalStr=Utils.SigTermTag, orderReversed=False), Print("failed to kill nodes, see arrors above")

    # after entire cluster was killed checking that rocksdb nodes have undo_stack and fork_db files
    for rocksdbNode in rocksdbNodes:
        dataDir = Utils.getNodeDataDir(rocksdbNode.nodeId)
        stateDir = os.path.join(dataDir, "state")
        assert os.path.isdir(stateDir), Print("State directory is missing: {}".format(stateDir))
        undo_stack_file = os.path.abspath(os.path.join(stateDir, "undo_stack.dat"))
        assert os.path.isfile(undo_stack_file), Print("{} file missing".format(undo_stack_file))

        reversible_dir = os.path.join(dataDir, "blocks/reversible")
        assert os.path.isdir(stateDir), Print("Reversible directory is missing: {}".format(stateDir))
        fork_db_file = os.path.abspath(os.path.join(reversible_dir, "fork_db.dat"))
        assert os.path.isfile(fork_db_file), Print("{} is missing".format(fork_db_file))

    # restarting the cluster
    cluster.setChainStrategy(Utils.SyncNoneTag)
    cluster.relaunchEosInstances(cachePopen=True, nodeArgs="--enable-stale-production")

    # waiting for LIB to move
    start_time = time.time()
    assert node.waitForLibToAdvance(timeout=240), Print("LIB didn't advance in expected time")
    Print("startup synchronization took {}".format(time.time() - start_time))

    # checking multi_index stored values still there
    rows=node.getTableRows(contractOwner.name, contractOwner.name, 'numobjs')
    Utils.Print("modified numobjs rows: {}".format(json.dumps(rows, indent=4, sort_keys=True)))
    assert len(rows) == 2, Print("rows number doesn't match: {} != 2".format(len(rows)))
    assert rows[0]["sec64"] == 3, Print("row[0] doesn't match. 3 != {}".format(rows[0]["sec64"]))
    assert rows[1]["sec64"] == 5, Print("row[1] doesn't match. 5 != {}".format(rows[1]["sec64"]))

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)