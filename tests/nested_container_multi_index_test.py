#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from testUtils import ReturnType
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper
import json

###############################################################
# Nested_container_multi_index_test
#
# Load nested container contracts for multi_index table
# Verifies nested container for vector<optional>, set<optional>, vector<optional<mystruct>>
# and pair<int, vector<optional>>
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

cluster=Cluster(walletd=True)

walletMgr=WalletMgr(True)
EOSIO_ACCT_PRIVATE_DEFAULT_KEY = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
EOSIO_ACCT_PUBLIC_DEFAULT_KEY = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
contractDir='unittests/test-contracts/nested_container_multi_index'
wasmFile='nested_container_multi_index.wasm'
abiFile='nested_container_multi_index.abi'

def create_action(action, data, contract_account, usr):
    cmdArr= [Utils.EosClientPath, '-v', 'push', 'action', contract_account, action, data, '-p', usr+'@active']
    clargs = node.eosClientArgs().split()
    for x in clargs[::-1]:
        cmdArr.insert(1, x)
    result = Utils.checkOutput(cmdArr, ignoreError=False)
    Print("result= ", result)

try:
    cluster.killall(allInstances=False)
    cluster.cleanup()

    Print ("producing nodes: %s, non-producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d" %
            (pnodes, total_nodes-pnodes, topo, delay))

    Print("Stand up cluster")
    if cluster.launch(pnodes=1, totalNodes=1) is False:
        errorExit("Failed to stand up eos cluster.")

    Print ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not cluster.waitOnClusterBlockNumSync(3):
        errorExit("Cluster never stabilized")

    Print("Creating multi_index account")
    MIacct = Account('nestcontnmi')
    MIacct.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    MIacct.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(MIacct, cluster.eosioAccount, buyRAM=700000)
    Print("Creating user account")
    useracct = Account('alice')
    useracct.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    useracct.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(useracct, cluster.eosioAccount, buyRAM=700000)

    Print("Validating accounts")
    cluster.validateAccounts([MIacct, useracct])

    node = cluster.getNode()

    Print("Setting account privilege")
    node.pushMessage(cluster.eosioAccount.name, 'setpriv', '["nestcontnmi", 1]', '-p eosio@active')

    Print("Loading nested container contract")
    node.publishContract(MIacct, contractDir, wasmFile, abiFile, waitForTransBlock=True)

    Print("Test action for vector<optional<uint16_t>>")
    create_action('setvo', '["alice",[100, null, 500]]', 'nestcontnmi', 'alice')

    Print("Test action for set<optional<uint16_t>>")
    create_action('setsto', '["alice",[null, null, 500]]', 'nestcontnmi', 'alice')

    Print("Test action for vector<optional<mystruct>>")
    create_action('setvos', '["alice", [{"_count": 18, "_strID": "dumstr"}, null, {"_count": 19, "_strID": "dumstr"}]]', 'nestcontnmi', 'alice')

    Print("Test action for pair<uint16_t, vector<optional<uint16_t>>>")
    create_action('setpvo', '["alice",{"first": 183, "second":[100, null, 200]}]', 'nestcontnmi', 'alice')

    cmd="get table %s %s people2" % (MIacct.name, MIacct.name)   

    transaction = node.processCleosCmd(cmd, cmd, False, returnType=ReturnType.raw)
    transaction_json = json.loads(transaction)

    assert "[100, None, 500]" == str(transaction_json['rows'][0]['vo']), 'Multi-index table vector< optional<T> > must be [100, None, 500]'
    assert "[None, 500]" == str(transaction_json['rows'][0]['sto']), 'Multi-index table set< optional<T> > must be [None, 500]'

    assert "[{'_count': 18, '_strID': 'dumstr'}, None, {'_count': 19, '_strID': 'dumstr'}]" == str(transaction_json['rows'][0]['vos']), \
         "multi-index table vector<optional<mystruct>> must be [{'_count': 18, '_strID': 'dumstr'}, None, {'_count': 19, '_strID': 'dumstr'}]"
    
    assert "{'first': 183, 'second': [100, None, 200]}" == str(transaction_json['rows'][0]['pvo']), \
         "multi-index table pair<uint16_t, vector<optional<uint16_t>>> must be {'first': 183, 'second': [100, None, 200]}"
        
    testSuccessful=True            
    assert testSuccessful
    
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet)

exit(0)