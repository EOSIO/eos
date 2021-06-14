#!/usr/bin/env python3

from testUtils import Account
from testUtils import WaitSpec
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
        totalNodes=3,
        useBiosBootFile=False,
        loadSystemContract=False,
        specificExtraNodeosArgs={
            0: "--plugin eosio::state_history_plugin --trace-history --disable-replay-opts --sync-fetch-span 200 --state-history-endpoint 127.0.0.1:8080 --plugin eosio::net_api_plugin --enable-stale-production",
            2: "--validation-mode light --p2p-reject-incomplete-blocks 0"})

    producerNodeIndex = 0
    producerNode = cluster.getNode(producerNodeIndex)
    fullValidationNodeIndex = 1
    fullValidationNode = cluster.getNode(fullValidationNodeIndex)
    lightValidationNodeIndex = 2
    lightValidationNode = cluster.getNode(lightValidationNodeIndex)

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

    fullValidationNode.kill(signal.SIGTERM)
    lightValidationNode.kill(signal.SIGTERM)

    trx = {
        "actions": [{"account": "payloadless", "name": "doit", "authorization": [{
          "actor": "payloadless", "permission": "active"}], "data": ""}],
        "context_free_actions": [{"account": "payloadless", "name": "doit", "data": ""}],
        "context_free_data": ["a1b2c3", "1a2b3c"],
    } 

    (success, trans) = producerNode.pushTransaction(trx, permissions="payloadless", opts=None)
    assert success and trans, "Failed to push transaction with context free data"
    
    cfTrxBlockNum = int(trans["processed"]["block_num"])
    cfTrxId = trans["transaction_id"]

    # Wait until the block where create account is executed to become irreversible
    producerNode.waitForBlock(cfTrxBlockNum, blockType=BlockType.lib, timeout=WaitSpec.calculate(), errorContext="producerNode LIB did not advance")

    Utils.Print("verify the account payloadless from producer node")
    trans = producerNode.getEosAccount("payloadless")
    assert trans["account_name"], "Failed to get the account payloadless"

    Utils.Print("verify the context free transaction from producer node")
    trans_from_full = producerNode.getTransaction(cfTrxId)
    assert trans_from_full, "Failed to get the transaction with context free data from the producer node"

    producerNode.kill(signal.SIGTERM)

    # prune the transaction with block-num=trans["block_num"], id=cfTrxId
    cluster.getBlockLog(producerNodeIndex, blockLogAction=BlockLogAction.prune_transactions, extraArgs=" --block-num {} --transaction {}".format(trans_from_full["block_num"], cfTrxId), exitOnError=True)

    # try to prune the transaction where it doesn't belong
    try:
        cluster.getBlockLog(producerNodeIndex, blockLogAction=BlockLogAction.prune_transactions, extraArgs=" --block-num {} --transaction {}".format(cfTrxBlockNum - 1, cfTrxId), throwException=True, silentErrors=True)
    except:
        ex = sys.exc_info()[0]
        msg=ex.output.decode("utf-8")
        assert "does not contain the following transactions: " + cfTrxId in msg, "The transaction id is not displayed in the console when it cannot be found"

    #
    #  restart the producer node with pruned cfd
    #
    isRelaunchSuccess = producerNode.relaunch()
    assert isRelaunchSuccess, "Fail to relaunch full producer node"

    #
    #  check if the cfd has been pruned from producer node
    #
    trans = producerNode.getTransaction(cfTrxId)
    assert trans, "Failed to get the transaction with context free data from the producer node"
    # check whether the transaction has been pruned based on the tag of prunable_data, if the tag is 1, then it's a prunable_data_t::none
    assert trans["trx"]["receipt"]["trx"][1]["prunable_data"]["prunable_data"][0] == 1, "the the transaction with context free data has not been pruned"

    producerNode.waitForBlock(cfTrxBlockNum, blockType=BlockType.lib, timeout=WaitSpec.calculate(), errorContext="producerNode LIB did not advance")
    assert producerNode.waitForHeadToAdvance(), "the producer node stops producing"

    # kill the producer with un-pruned cfd
    cluster.biosNode.kill(signal.SIGTERM)

    #
    #  restart both full and light validation node
    #
    isRelaunchSuccess = fullValidationNode.relaunch()
    assert isRelaunchSuccess, "Fail to relaunch full verification node"

    isRelaunchSuccess = lightValidationNode.relaunch()
    assert isRelaunchSuccess, "Fail to relaunch light verification node"

    #
    # Check lightValidationNode keeping sync while the full fullValidationNode stop syncing after the cfd block
    #
    timeForNodesToWorkOutReconnect=30 + 10    # 30 is reconnect cycle since all nodes are restarting and giving time for a little churn
    lightValidationNode.waitForBlock(cfTrxBlockNum, blockType=BlockType.lib, timeout=WaitSpec.calculate(leeway=timeForNodesToWorkOutReconnect), errorContext="lightValidationNode did not advance")

    pvnPreInfo = producerNode.getInfo(exitOnError=True)
    fvnPreInfo = fullValidationNode.getInfo(exitOnError=True)
    lvnPreInfo = lightValidationNode.getInfo(exitOnError=True)

    Utils.Print("Ensure light validation node continues to sync")

    headAdvanced = lightValidationNode.waitForHeadToAdvance()
    pvnPostInfo = producerNode.getInfo(exitOnError=True)
    fvnPostInfo = fullValidationNode.getInfo(exitOnError=True)
    lvnPostInfo = lightValidationNode.getInfo(exitOnError=True)
    if not headAdvanced:
        Utils.Print("Pre waiting info for each node:\nproducer: {}\nfull: {},\nlight: {}".format(json.dumps(pvnPreInfo, indent=1), json.dumps(fvnPreInfo, indent=1), json.dumps(lvnPreInfo, indent=1)))
        Utils.Print("Post light validation waiting info for each node:\nproducer: {}\nfull: {},\nlight: {}".format(json.dumps(pvnPostInfo, indent=1), json.dumps(fvnPostInfo, indent=1), json.dumps(lvnPostInfo, indent=1)))

    assert headAdvanced or lvnPostInfo["head_block_num"] >= cfTrxBlockNum, "the light validation node stops syncing"

    fullValidationNode.waitForBlock(cfTrxBlockNum-1, blockType=BlockType.lib, timeout=WaitSpec.calculate(), errorContext="fullValidationNode LIB did not advance")
    Utils.Print("Ensure full validation node stops syncing")
    headAdvanced = fullValidationNode.waitForHeadToAdvance()
    if headAdvanced:
        pvnPost2Info = producerNode.getInfo(exitOnError=True)
        fvnPost2Info = fullValidationNode.getInfo(exitOnError=True)
        lvnPost2Info = lightValidationNode.getInfo(exitOnError=True)
        Utils.Print("Pre waiting info for each node:\nproducer: {}\nfull: {},\nlight: {}".format(json.dumps(pvnPreInfo, indent=1), json.dumps(fvnPreInfo, indent=1), json.dumps(lvnPreInfo, indent=1)))
        Utils.Print("Post light validation waiting info for each node:\nproducer: {}\nfull: {},\nlight: {}".format(json.dumps(pvnPostInfo, indent=1), json.dumps(fvnPostInfo, indent=1), json.dumps(lvnPostInfo, indent=1)))
        Utils.Print("Post full validation waiting info for each node:\nproducer: {}\nfull: {},\nlight: {}".format(json.dumps(pvnPost2Info, indent=1), json.dumps(fvnPost2Info, indent=1), json.dumps(lvnPost2Info, indent=1)))

    assert (not headAdvanced) or (fvnPost2Info["head_block_num"] < cfTrxBlockNum), "the full validation node is still syncing"

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
