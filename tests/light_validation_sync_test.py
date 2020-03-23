#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper
from TestHelper import AppArgs
import json

###############################################################
# nodeos_chainbase_allocation_test
#
# Test snapshot creation and restarting from snapshot
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

    # The following is the list of chainbase objects that need to be verified:
    # - account_object (bootstrap)
    # - code_object (bootstrap)
    # - generated_transaction_object
    # - global_property_object
    # - key_value_object (bootstrap)
    # - protocol_state_object (bootstrap)
    # - permission_object (bootstrap)
    # The bootstrap process has created account_object and code_object (by uploading the bios contract),
    # key_value_object (token creation), protocol_state_object (preactivation feature), and permission_object
    # (automatically taken care by the automatically generated eosio account)
    assert cluster.launch(
        pnodes=1,
        prodCount=1,
        totalProducers=1,
        totalNodes=2,
        useBiosBootFile=False,
        loadSystemContract=False,
        specificExtraNodeosArgs={
            1:"--validation-mode light"})

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
    assert trans
    
    cfTrxBlockNum = int(trans["processed"]["block_num"])
    cfTrxId = trans["transaction_id"]

    # Wait until the block where create account is executed to become irreversible 
    def isBlockNumIrr():
        return validationNode.getIrreversibleBlockNum() >= cfTrxBlockNum
    Utils.waitForBool(isBlockNumIrr, timeout=30, sleepTime=0.1)
    
    Utils.Print("verify the account payloadless from validation node")
    cmd = "get account -j payloadless"
    trans = validationNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans["account_name"]

    Utils.Print("verify the context free transaction from validation node")
    cmd = "get transaction " + cfTrxId
    trans = validationNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
