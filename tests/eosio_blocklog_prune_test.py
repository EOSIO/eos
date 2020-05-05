#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper
from TestHelper import AppArgs
from testUtils import BlockLogAction
import json
import sys
import signal

###############################################################
# eosio_blocklog_prune_test.py
#
# Test eosio-blocklog prune-transaction 
# 
# This test creates a producer node and a verification node. 
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
        totalNodes=2,
        useBiosBootFile=False,
        loadSystemContract=False,
        specificExtraNodeosArgs={
            1:"--plugin eosio::state_history_plugin --trace-history --disable-replay-opts --sync-fetch-span 200 --plugin eosio::net_api_plugin "})

    producerNode = cluster.getNode(0)
    validationNode = cluster.getNode(1)

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
    def isBlockNumIrr():
        return validationNode.getIrreversibleBlockNum() >= cfTrxBlockNum
    Utils.waitForBool(isBlockNumIrr, timeout=30, sleepTime=0.1)
    
    Utils.Print("verify the account payloadless from validation node")
    cmd = "get account -j payloadless"
    trans = validationNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans["account_name"], "Failed to get the account payloadless"

    Utils.Print("verify the context free transaction from validation node")
    cmd = "get transaction " + cfTrxId
    trans = validationNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans, "Failed to get the transaction with context free data from the light validation node"

    validationNode.kill(signal.SIGTERM)

    # prune the transaction with block-num=trans["block_num"], id=cfTrxId
    cluster.getBlockLog(1, blockLogAction=BlockLogAction.prune_transactions, extraArgs=" --block-num {} --transaction {}".format(trans["block_num"], cfTrxId), exitOnError=True)

    # try to prune the transaction where it doesn't belong
    try:
        cluster.getBlockLog(1, blockLogAction=BlockLogAction.prune_transactions, extraArgs=" --block-num {} --transaction {}".format(cfTrxBlockNum - 1, cfTrxId), throwException=True, silentErrors=True)
    except:
        ex = sys.exc_info()[0]
        msg=ex.output.decode("utf-8")
        assert "does not contain the following transactions: " + cfTrxId in msg, "The transaction id is not displayed in the console when it cannot be found"

    isRelaunchSuccess = validationNode.relaunch(1)
    assert isRelaunchSuccess, "Fail to relaunch verification node"

    trans = validationNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans, "Failed to get the transaction with context free data from the light validation node"
    # check whether the transaction has been pruned based on the tag of prunable_data, if the tag is 1, then it's a prunable_data_t::none
    assert trans["trx"]["receipt"]["trx"][1]["prunable_data"]["prunable_data"][0] == 1, "the the transaction with context free data has not been pruned"

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
