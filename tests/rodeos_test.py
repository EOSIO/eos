#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from Node import BlockType
from TestHelper import TestHelper
from TestHelper import AppArgs
from testUtils import BlockLogAction
import json
import sys
import signal
import time
import subprocess

###############################################################
# eosio_blocklog_prune_test.py
#
# Test eosio-blocklog prune-transaction 
# 
# This test creates a producer node and 2 verification nodes. 
# The verification node is configured with state history plugin.
# A transaction with context free data is pushed to the producer node.
# Afterwards, it uses eosio-blocklog to prune the transaction with context free
# data and check if the context free data in the block log has actually been pruned. 
# Notice that there's no verification on whether the pruning on the 
# state history traces log has actually been pruned. We rely on the 
# unittests/state_history_tests.cpp to perform this test. 
#
###############################################################

# Parse command line arguments
args = TestHelper.parse_args({"-v","--clean-run","--dump-error-details","--leave-running","--keep-logs"})
Utils.Debug = args.v
killAll=args.clean_run
dumpErrorDetails=args.dump_error_details
dontKill=args.leave_running
killEosInstances=not dontKill
killWallet=not dontKill
keepLogs=args.keep_logs

walletMgr=WalletMgr(True)
cluster=Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

testSuccessful = False
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    assert cluster.launch(
        pnodes=1,
        prodCount=1,
        totalProducers=1,
        totalNodes=1,
        useBiosBootFile=False,
        loadSystemContract=False,
        specificExtraNodeosArgs={
            0: "--plugin eosio::state_history_plugin --trace-history --disable-replay-opts --sync-fetch-span 200 --state-history-endpoint 127.0.0.1:8080 --plugin eosio::net_api_plugin --enable-stale-production"})

    rodeos = subprocess.Popen(['./programs/rodeos/rodeos', '--rdb-database', './rocksdb', '--data-dir','./data',  '--clone-connect-to',  '127.0.0.1:8080', '--filter-name', 'voice.filter', '--filter-wasm', './voice_filter.wasm' ],
                     stdout=subprocess.PIPE, 
                     stderr=subprocess.PIPE)

    producerNodeIndex = 0
    producerNode = cluster.getNode(producerNodeIndex)
   
    # Create a transaction to create an account
    Utils.Print("create a new account payloadless from the producer node")
    payloadlessAcc = Account("payloadless")
    payloadlessAcc.ownerPublicKey = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
    payloadlessAcc.activePublicKey = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
    producerNode.createAccount(payloadlessAcc, cluster.eosioAccount)


    contractDir="unittests/test-contracts/payloadless"
    wasmFile="payloadless.wasm"
    abiFile="payloadless.abi"
    Utils.Print("Publish payloadless contract")
    trans = producerNode.publishContract(payloadlessAcc, contractDir, wasmFile, abiFile, waitForTransBlock=True)

    trx = {
        "actions": [{"account": "payloadless", "name": "doit", "authorization": [{
          "actor": "payloadless", "permission": "active"}], "data": ""}],
        "context_free_actions": [{"account": "payloadless", "name": "doit", "data": ""}],
        "context_free_data": ["a1b2c3", "1a2b3c"],
    } 

    cmd = "push transaction '{}' -p payloadless".format(json.dumps(trx))
    trans = producerNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans, "Failed to push transaction with context free data"
    
    cfTrxBlockNum = int(trans["processed"]["block_num"])
    cfTrxId = trans["transaction_id"]

    # Wait until the block where create account is executed to become irreversible 
    Utils.waitForBool(lambda: producerNode.getIrreversibleBlockNum() >= cfTrxBlockNum, timeout=30, sleepTime=0.1)
    
    Utils.Print("verify the account payloadless from producer node")
    cmd = "get account -j payloadless"
    trans = producerNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans["account_name"], "Failed to get the account payloadless"

    Utils.Print("verify the context free transaction from producer node")
    cmd = "get transaction " + cfTrxId

    trans_from_full = producerNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans_from_full, "Failed to get the transaction with context free data from the producer node"


    # response = Utils.runCmdArrReturnJson(['curl', '-H', 'Content-Type: application/json', '-H', 'Accept: application/json', 'http://127.0.0.1:8880/v1/chain/get_info'])
    
    # request_body = { "block_num_or_id": cfTrxBlockNum }
    # response=Utils.runCmdArrReturnJson(['curl', '-X', 'POST',  '-H', 'Content-Type: application/json', '-H', 'Accept: application/json', 'http://127.0.0.1:8880/v1/chain/get_block', '--data', json.dumps(request_body)])

    # print("redoes responded ==> {}".format(response))

    
    testSuccessful = True
finally:
    rodeos.kill()
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
