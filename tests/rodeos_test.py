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
import os
import shutil

###############################################################
# rodeos_test.py
#
# rodeos integration test 
# 
# This test creates a producer node with state history plugin and a 
# rodeos process with a test filter to connect to the producer. Pushes 
# a few transactions to the producer and query rodeos get_block endpoint
# to see if it sees one of the block containing the pushed transaction. 
# Lastly, it verifies if rodeos get_info endpoint returns a head_block_num. 
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
rodeos = None
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    rodeos_dir = os.path.join(os.getcwd(), 'var/lib/rodeos')
    shutil.rmtree(rodeos_dir, ignore_errors=True)
    os.makedirs(rodeos_dir, exist_ok=True)

    assert cluster.launch(
        pnodes=1,
        prodCount=1,
        totalProducers=1,
        totalNodes=1,
        useBiosBootFile=False,
        loadSystemContract=False,
        specificExtraNodeosArgs={
            0: ("--plugin eosio::state_history_plugin --trace-history --chain-state-history --disable-replay-opts " 
                "--state-history-endpoint 127.0.0.1:8080 --plugin eosio::net_api_plugin --wasm-runtime eos-vm-jit")})

    rodeos_stdout = open(os.path.join(rodeos_dir, "stdout.out"), "w")
    rodeos_stderr = open(os.path.join(rodeos_dir, "stderr.out"), "w")
    rodeos = subprocess.Popen(['./programs/rodeos/rodeos', '--rdb-database', os.path.join(rodeos_dir,'rocksdb'), '--data-dir', os.path.join(rodeos_dir,'data'),
                               '--clone-connect-to',  '127.0.0.1:8080', '--filter-name', 'test.filter', '--filter-wasm', './tests/test_filter.wasm' ],
                     stdout=rodeos_stdout, 
                     stderr=rodeos_stderr)

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
    
    Utils.Print("Verify rodeos get_block endpoint works")
    request_body = { "block_num_or_id": cfTrxBlockNum }
    response=Utils.runCmdArrReturnJson(['curl', '-X', 'POST',  '-H', 'Content-Type: application/json', '-H', 'Accept: application/json', 'http://127.0.0.1:8880/v1/chain/get_block', '--data', json.dumps(request_body)])
    assert 'id' in response, "Redeos response does not contain the block id"

    Utils.Print("Verify rodeos get_info endpoint works")
    response = Utils.runCmdArrReturnJson(['curl', '-H', 'Accept: application/json', 'http://127.0.0.1:8880/v1/chain/get_info'])
    assert 'head_block_num' in response, "Redeos response does not contain head_block_num, response body = {}".format(json.dumps(response))
    
    testSuccessful = True
finally:
    if rodeos is not None:
        rodeos.kill()
    if testSuccessful and not keepLogs:
        shutil.rmtree(rodeos_dir, ignore_errors=True)
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)
    
exitCode = 0 if testSuccessful else 1
exit(exitCode)
