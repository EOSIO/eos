#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from testUtils import ReturnType
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper

import random

###############################################################
# read_only_query_tests
#
# Loads read-only query test contract and validates
# new API disallows write operations.
# Also validates retrieval from multiple KV tables in a single query.
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit

args=TestHelper.parse_args({"-p","-n","-d","-s","--nodes-file","--seed"
                           ,"--dump-error-details","-v","--leave-running"
                           ,"--clean-run","--keep-logs"})

pnodes=args.p
topo=args.s
delay=args.d
total_nodes = pnodes if args.n < pnodes else args.n
debug=args.v
nodesFile=args.nodes_file
dontLaunch=nodesFile is not None
seed=args.seed
dontKill=args.leave_running
dumpErrorDetails=args.dump_error_details
killAll=args.clean_run
keepLogs=args.keep_logs

killWallet=not dontKill
killEosInstances=not dontKill
if nodesFile is not None:
    killEosInstances=False

Utils.Debug=debug
testSuccessful=False

random.seed(seed) # Use a fixed seed for repeatability.
cluster=Cluster(walletd=True)

walletMgr=WalletMgr(True)
EOSIO_ACCT_PRIVATE_DEFAULT_KEY = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
EOSIO_ACCT_PUBLIC_DEFAULT_KEY = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
contractDir='unittests/contracts'
kvWasmFile='kv_bios.wasm'
kvAbiFile='kv_bios.abi'
wasmFile='read_only_query_tests.wasm'
abiFile='read_only_query_tests.abi'

try:
    if dontLaunch: # run test against remote cluster
        jsonStr=None
        with open(nodesFile, "r") as f:
            jsonStr=f.read()
        if not cluster.initializeNodesFromJson(jsonStr):
            errorExit("Failed to initilize nodes from Json string.")
        total_nodes=len(cluster.getNodes())

        walletMgr.killall(allInstances=killAll)
        walletMgr.cleanup()
        print("Stand up walletd")
        if walletMgr.launch() is False:
            errorExit("Failed to stand up keosd.")
    else:
        cluster.killall(allInstances=killAll)
        cluster.cleanup()

        Print ("producing nodes: %s, non-producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d" %
               (pnodes, total_nodes-pnodes, topo, delay))

        Print("Stand up cluster")
        if cluster.launch(pnodes=pnodes, totalNodes=total_nodes, topo=topo, delay=delay) is False:
            errorExit("Failed to stand up eos cluster.")

        Print ("Wait for Cluster stabilization")
        # wait for cluster to start producing blocks
        if not cluster.waitOnClusterBlockNumSync(3):
            errorExit("Cluster never stabilized")

    Print("Creating kv settings account")
    kvsettingsaccount = Account('kvsettings11')
    kvsettingsaccount.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    kvsettingsaccount.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(kvsettingsaccount, cluster.eosioAccount, buyRAM=700000)
    Print("Creating read-only query account")
    readtestaccount = Account('readtest1111')
    readtestaccount.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    readtestaccount.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(readtestaccount, cluster.eosioAccount, buyRAM=700000)

    Print("Validating accounts after bootstrap")
    cluster.validateAccounts([kvsettingsaccount, readtestaccount])

    node = cluster.getNode()

    Print("Setting KV settings account privileged")
    node.pushMessage(cluster.eosioAccount.name, 'setpriv', '["kvsettings11", 1]', '-p eosio@active')
    Print("Loading KV bios")
    node.publishContract(kvsettingsaccount, contractDir, kvWasmFile, kvAbiFile, waitForTransBlock=True)
    node.pushMessage(kvsettingsaccount.name, 'ramkvlimits', '[1024, 1024, 1024]', '-p kvsettings11@active')
    node.publishContract(readtestaccount, contractDir, wasmFile, abiFile, waitForTransBlock=True)

    Print("Running setup as read-only query")
    trx = {
           "actions": [{"account": readtestaccount.name, "name": "setup",
                        "authorization": [{"actor": readtestaccount.name, "permission": "active"}],
                        "data": ""}]
          }
    success, transaction = node.pushTransaction(trx, opts='--read-only', permissions=readtestaccount.name)
    assert success, 'Executing setup as read-only query failed'
    transId = node.getTransId(transaction)
    # Default wait time from WaitSpec is 60 seconds.  Default transaction expiration time from cleos is 30 seconds.
    result = node.waitForTransInBlock(transId, exitOnError=False)
    assert False == result, 'Executing setup as read-only was erroneously incorporated into the chain'

    Print("Verifying kv tables not written")
    cmd="get kv_table %s roqm id" % readtestaccount.name
    transaction = node.processCleosCmd(cmd, cmd, False, returnType=ReturnType.raw)
    expected = '''{
  "rows": [],
  "more": false,
  "next_key": "",
  "next_key_bytes": ""
}
'''
    assert expected == transaction, 'kv table roqm should be empty'

    Print("Setting up read-only tables")
    success, transaction = node.pushMessage(readtestaccount.name, 'setup', "{}", '-p readtest1111@active')
    assert success, 'Creating KV tables failed\n' + str(transaction)
    transId = node.getTransId(transaction)
    node.waitForTransInBlock(transId)

    Print("Querying combined kv tables")
    trx = {
           "actions": [{"account": readtestaccount.name, "name": "get",
                        "authorization": [{"actor": readtestaccount.name, "permission": "active"}],
                        "data": ""}]
          }
    success, transaction = node.pushTransaction(trx, opts='--read-only --return-failure-trace', permissions=readtestaccount.name)

    assert success
    assert 8 == len(transaction['result']['action_traces'][0]['return_value_data']), 'Combined kv tables roqm and roqf should contain 8 rows\n' + str(transaction)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
