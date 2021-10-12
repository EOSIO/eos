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
# Load nested container contracts for multi-index table
# Verifies nested container for table below
# 
###############################################################
#         |  set  |  vector |  optional | map | pair | tuple |
#---------------------------------------------------------------
#set      |   X   |    X    |     X     |  X  |  X   |   X   |
#---------------------------------------------------------------
#vector   |   X   |    X    |     X     |  X  |  X   |   X   |
#---------------------------------------------------------------
#optional |   X   |    X    |     X     |  X  |  X   |   X   |
#---------------------------------------------------------------
#map      |   X   |    X    |     X     |  X  |  X   |   X   |
#---------------------------------------------------------------
#pair     |   X   |    X    |     X     |  X  |  X   |   X   |
#---------------------------------------------------------------
#tuple    |   X   |    X    |     X     |  X  |  X   |   X   |
#---------------------------------------------------------------
################################################################

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
    traceNodeosArgs = " --plugin eosio::trace_api_plugin --trace-no-abis "
    if cluster.launch(pnodes=1, totalNodes=1, extraNodeosArgs=traceNodeosArgs) is False:
        errorExit("Failed to stand up eos cluster.")

    Print ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not cluster.waitOnClusterBlockNumSync(3):
        errorExit("Cluster never stabilized")

    Print("Creating multi_index account")
    MIacct = Account('nestcontnmi')
    MIacct.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    MIacct.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(MIacct, cluster.eosioAccount, buyRAM=7000000)
    Print("Creating user account alice")
    useracct_I = Account('alice')
    useracct_I.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    useracct_I.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(useracct_I, cluster.eosioAccount, buyRAM=7000000)

    Print("Creating user account bob")
    useracct_II = Account('bob')
    useracct_II.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    useracct_II.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(useracct_II, cluster.eosioAccount, buyRAM=7000000)

    Print("Validating accounts")
    cluster.validateAccounts([MIacct, useracct_I, useracct_II])

    node = cluster.getNode()

    Print("Setting account privilege")
    node.pushMessage(cluster.eosioAccount.name, 'setpriv', '["nestcontnmi", 1]', '-p eosio@active')

    Print("Loading nested container contract")
    node.publishContract(MIacct, contractDir, wasmFile, abiFile, waitForTransBlock=True)

    Print("Test action for set< set< uint16_t >>")
    create_action('setstst', '["alice", [[10, 10], [3], [400, 500, 600]]]', 'nestcontnmi', 'alice')

    Print("Test action for set< vector< uint16_t >>")
    create_action('setstv', '["alice", [[16, 26], [36], [36], [46, 506, 606]]]', 'nestcontnmi', 'alice')

    Print("Test action for set< optional< uint16_t >>")
    create_action('setsto', '["alice", [null, null, 500]]', 'nestcontnmi', 'alice')

    Print("Test action for set< map< uint16_t, uint16_t >>")
    create_action('setstm', '["alice", [[{"key":30,"value":300},{"key":30,"value":300}],[{"key":60,"value":600},{"key":60,"value":600}]]]', 'nestcontnmi', 'alice')

    Print("Test action for set< pair< uint16_t, uint16_t >>")
    create_action('setstp', '["alice", [{"key": 69, "value": 129}, {"key": 69, "value": 129}]]', 'nestcontnmi', 'alice')

    Print("Test action for set< tuple< uint16_t, uint16_t >>")
    create_action('setstt', '["alice", [[1,2],[36,46], [56,66]]]', 'nestcontnmi', 'alice')

    
    Print("Test action for vector< set< uint16_t >>")
    create_action('setvst', '["alice", [[10, 10], [3], [400, 500, 600]]]', 'nestcontnmi', 'alice')

    Print("Test action for vector< vector< uint16_t >>")
    create_action('setvv', '["alice", [[16, 26], [36], [36], [46, 506, 606]]]', 'nestcontnmi', 'alice')

    Print("Test action for vector< optional< uint16_t >>")
    create_action('setvo', '["alice",[null, null, 500]]', 'nestcontnmi', 'alice')

    Print("Test action for vector< map< uint16_t, uint16_t >>")
    create_action('setvm', '["alice", [[{"key": 30, "value": 300}, {"key": 30, "value": 300}], [{"key": 60, "value": 600}, {"key": 60, "value": 600}]]]'
    , 'nestcontnmi', 'alice')

    Print("Test action for vector< pair< uint16_t, uint16_t >>")
    create_action('setvp', '["alice", [{"key": 69, "value": 129}, {"key": 69, "value": 129}]]', 'nestcontnmi', 'alice')

    Print("Test action for vector< tuple< uint16_t, uint16_t >>")
    create_action('setvt', '["alice", [[10,20],[30,40], [50,60]]]', 'nestcontnmi', 'alice')


    Print("Test action for optional< set< uint16_t >>")
    create_action('setost', '["alice", [10, 10, 3]]', 'nestcontnmi', 'alice')
    create_action('setost', '["bob", null]', 'nestcontnmi', 'bob')

    Print("Test action for optional< vector< uint16_t >>")
    create_action('setov', '["alice", [46, 506, 606]]', 'nestcontnmi', 'alice')
    create_action('setov', '["bob", null]', 'nestcontnmi', 'bob')

    Print("Test action for optional< optional< uint16_t >>")
    create_action('setoo', '["alice", 500]', 'nestcontnmi', 'alice')
    create_action('setoo', '["bob", null]', 'nestcontnmi', 'bob')

    Print("Test action for optional< map< uint16_t, uint16_t >>")
    create_action('setom', '["alice", [{"key": 10, "value": 1000}, {"key": 11,"value": 1001}] ]', 'nestcontnmi', 'alice')
    create_action('setom', '["bob", null]', 'nestcontnmi', 'bob')

    Print("Test action for optional< pair< uint16_t, uint16_t >>")
    create_action('setop', '["alice", {"key": 60, "value": 61}]', 'nestcontnmi', 'alice')
    create_action('setop', '["bob", null]', 'nestcontnmi', 'bob')

    Print("Test action for optional< tuple< uint16_t, uint16_t >>")
    create_action('setot', '["alice", [1001,2001]]', 'nestcontnmi', 'alice')
    create_action('setot', '["bob", null]', 'nestcontnmi', 'bob')


    Print("Test action for map< set< uint16_t >>")
    create_action('setmst', '["alice", [{"key": 1,"value": [10, 10, 12, 16]},  {"key": 2, "value": [200, 300]} ]]', 'nestcontnmi', 'alice')

    Print("Test action for map< vector< uint16_t >>")
    create_action('setmv', '["alice", [{"key": 1, "value": [10, 10, 12, 16]},  {"key": 2, "value": [200, 300]} ]]', 'nestcontnmi', 'alice')

    Print("Test action for map< optional< uint16_t >>")
    create_action('setmo', '["alice", [{"key": 10, "value": 1000}, {"key": 11, "value": null}]]', 'nestcontnmi', 'alice')

    Print("Test action for map< map< uint16_t, uint16_t >>")
    create_action('setmm', '["alice", [{"key": 10, "value": [{"key": 200, "value": 2000}, \
         {"key": 201, "value": 2001}] }, {"key": 11, "value": [{"key": 300, "value": 3000}, {"key": 301, "value": 3001}] } ]]', 'nestcontnmi', 'alice')

    Print("Test action for map< pair< uint16_t, uint16_t >>")
    create_action('setmp', '["alice", [{"key": 36, "value": {"key": 300, "value": 301}}, {"key": 37, "value": {"key": 600, "value": 601}} ]]', 'nestcontnmi', 'alice')

    Print("Test action for map< tuple< uint16_t, uint16_t >>")
    create_action('setmt', '["alice", [{"key":1,"value":[10,11]},  {"key":2,"value":[200,300]} ]]', 'nestcontnmi', 'alice')


    Print("Test action for pair< set< uint16_t >>")
    create_action('setpst', '["alice", {"key": 20, "value": [200, 200, 202]}]', 'nestcontnmi', 'alice')

    Print("Test action for pair< vector< uint16_t >>")
    create_action('setpv', '["alice", {"key": 10, "value": [100, 100, 102]}]', 'nestcontnmi', 'alice')

    Print("Test action for pair< optional< uint16_t >>")
    create_action('setpo', '["alice", {"key": 70, "value": 71}]', 'nestcontnmi', 'alice')
    create_action('setpo', '["bob", {"key": 70, "value": null}]', 'nestcontnmi', 'bob')

    Print("Test action for pair< map< uint16_t, uint16_t >>")
    create_action('setpm', '["alice", {"key": 6, "value": [{"key": 20, "value": 300}, {"key": 21,"value": 301}] }]', 'nestcontnmi', 'alice')

    Print("Test action for pair< pair< uint16_t, uint16_t >>")
    create_action('setpp', '["alice", {"key": 30, "value": {"key": 301, "value": 302} }]', 'nestcontnmi', 'alice')

    Print("Test action for pair< tuple< uint16_t, uint16_t >>")
    create_action('setpt', '["alice", {"key":10, "value":[100,101]}]', 'nestcontnmi', 'alice')


    Print("Test action for tuple< uint16_t, set< uint16_t >, set< uint16_t >>")
    create_action('settst', '["alice", [10,[21,31], [41,51,61]]]', 'nestcontnmi', 'alice')

    Print("Test action for tuple< uint16_t, vector< uint16_t >, vector< uint16_t >")
    create_action('settv', '["alice", [16,[26,36], [46,506,606]]]', 'nestcontnmi', 'alice')

    Print("Test action for tuple< optional< uint16_t >, optional< uint16_t >, optional< uint16_t > \
    , optional< uint16_t >, optional< uint16_t >>")
    create_action('setto', '["alice", [100, null, 200, null, 300]]', 'nestcontnmi', 'alice')
    create_action('setto', '["bob", [null, null, 10, null, 20]]', 'nestcontnmi', 'bob')

    Print("Test action for tuple< map< uint16_t, uint16_t >, map< uint16_t, uint16_t >>")
    create_action('settm', '["alice", [126, [{"key":10,"value":100},{"key":11,"value":101}], [{"key":80,"value":800},{"key":81,"value":9009}] ]]', 'nestcontnmi', 'alice')

    Print("Test action for tuple< uint16_t, pair< uint16_t, uint16_t >, pair< uint16_t, uint16_t >>")
    create_action('settp', '["alice", [127, {"key":18, "value":28}, {"key":19, "value":29}]]', 'nestcontnmi', 'alice')

    Print("Test action for tuple< tuple< uint16_t, uint16_t >, tuple< uint16_t, uint16_t >, tuple< uint16_t, uint16_t >>")
    create_action('settt', '["alice", [[1,2],[30,40], [50,60]]]', 'nestcontnmi', 'alice')


    Print("Test action for vector<optional<mystruct>>")
    create_action('setvos', '["alice", [{"_count": 18, "_strID": "dumstr"}, null, {"_count": 19, "_strID": "dumstr"}]]', 'nestcontnmi', 'alice')

    Print("Test action for pair<uint16_t, vector<optional<uint16_t>>>")
    create_action('setpvo', '["alice",{"first": 183, "second":[100, null, 200]}]', 'nestcontnmi', 'alice')

    cmd="get table %s %s people2" % (MIacct.name, MIacct.name)   

    transaction = node.processCleosCmd(cmd, cmd, False, returnType=ReturnType.raw)
    transaction_json = json.loads(transaction)

    assert "[[3], [10], [400, 500, 600]]" == str(transaction_json['rows'][0]['stst']), \
        'Content of multi-index table set< set< uint16_t >> is not correct'

    assert "[[16, 26], [36], [46, 506, 606]]" == str(transaction_json['rows'][0]['stv']), \
        'Content of multi-index table set< vector< uint16_t >> is not correct'

    assert "[None, 500]" == str(transaction_json['rows'][0]['sto']), 'Content of multi-index table set< optional< uint16_t >>  is not correct'

    assert "[[{'key': 30, 'value': 300}], [{'key': 60, 'value': 600}]]" \
        == str(transaction_json['rows'][0]['stm']), 'Content of multi-index table set< map< uint16_t >> is not correct'

    assert "[{'key': 69, 'value': 129}]" == str(transaction_json['rows'][0]['stp']), \
        'Content of multi-index table set< pair< uint16_t >> is not correct'

    assert "[{'field_0': 1, 'field_1': 2}, {'field_0': 36, 'field_1': 46}, {'field_0': 56, 'field_1': 66}]" == str(transaction_json['rows'][0]['stt']), \
        'Content of multi-index table set< tuple< uint16_t, uint16_t >> is not correct'



    assert "[[10], [3], [400, 500, 600]]" == str(transaction_json['rows'][0]['vst']), \
        'Content of multi-index table vector< set< uint16_t >> is not correct'

    assert "[[16, 26], [36], [36], [46, 506, 606]]" == str(transaction_json['rows'][0]['vv']), \
        'Content of multi-index table vector< vector< uint16_t >> is not correct'

    assert "[None, None, 500]" == str(transaction_json['rows'][0]['vo']), \
        'Content of multi-index table vector< optional< uint16_t >>  is not correct'

    assert "[[{'key': 30, 'value': 300}], [{'key': 60, 'value': 600}]]" \
        == str(transaction_json['rows'][0]['vm']), 'Content of multi-index table vector< map< uint16_t >> is not correct'

    assert "[{'key': 69, 'value': 129}, {'key': 69, 'value': 129}]" == str(transaction_json['rows'][0]['vp']), \
        'Content of multi-index table vector< pair< uint16_t >> is not correct'

    assert "[{'field_0': 10, 'field_1': 20}, {'field_0': 30, 'field_1': 40}, {'field_0': 50, 'field_1': 60}]" == str(transaction_json['rows'][0]['vt']), \
        'Content of multi-index table vector< tuple< uint16_t, uint16_t >> is not correct'


    assert "[3, 10]" == str(transaction_json['rows'][0]['ost']), 'Content of multi-index table optional< set< uint16_t >> is not correct'
    assert "None" == str(transaction_json['rows'][1]['ost']), 'Content of multi-index table optional< set< uint16_t >> is not correct'

    assert "[46, 506, 606]" == str(transaction_json['rows'][0]['ov']), 'Content of multi-index table optional< vector< uint16_t >> is not correct'
    assert "None" == str(transaction_json['rows'][1]['ov']), 'Content of multi-index table optional< set< uint16_t >> is not correct'

    assert "500" == str(transaction_json['rows'][0]['oo']), 'Content of multi-index table optional< optional< uint16_t >>  is not correct'
    assert "None" == str(transaction_json['rows'][1]['oo']), 'Content of multi-index table optional< set< uint16_t >> is not correct'

    assert "[{'key': 10, 'value': 1000}, {'key': 11, 'value': 1001}]" \
        == str(transaction_json['rows'][0]['om']), 'Content of multi-index table optional< map< uint16_t >> is not correct'
    assert "None" == str(transaction_json['rows'][1]['om']), 'Content of multi-index table optional< set< uint16_t >> is not correct'

    assert "{'key': 60, 'value': 61}" == str(transaction_json['rows'][0]['op']), \
        'Content of multi-index table optional< pair< uint16_t >> is not correct'
    assert "None" == str(transaction_json['rows'][1]['op']), 'Content of multi-index table optional< set< uint16_t >> is not correct'

    assert "{'field_0': 1001, 'field_1': 2001}" == str(transaction_json['rows'][0]['ot']), 'Content of multi-index table optional< tuple< uint16_t, uint16_t >>  is not correct'
    assert "None" == str(transaction_json['rows'][1]['ot']), 'Content of multi-index table optional< tuple< uint16_t, uint16_t >> is not correct'


    assert "[{'key': 1, 'value': [10, 12, 16]}, {'key': 2, 'value': [200, 300]}]" \
        == str(transaction_json['rows'][0]['mst']), 'Content of multi-index table map< set< uint16_t >> is not correct'

    assert "[{'key': 1, 'value': [10, 10, 12, 16]}, {'key': 2, 'value': [200, 300]}]" \
        == str(transaction_json['rows'][0]['mv']), 'Content of multi-index table map< vector< uint16_t >> is not correct'

    assert "[{'key': 10, 'value': 1000}, {'key': 11, 'value': None}]" == \
        str(transaction_json['rows'][0]['mo']), 'Content of multi-index table map< optional< uint16_t >> is not correct'

    assert "[{'key': 10, 'value': [{'key': 200, 'value': 2000}, {'key': 201, 'value': 2001}]}, {'key': 11, 'value': [{'key': 300, 'value': 3000}, {'key': 301, 'value': 3001}]}]" \
        == str(transaction_json['rows'][0]['mm']), 'Content of multi-index table map< map< uint16_t >> is not correct'

    assert "[{'key': 36, 'value': {'key': 300, 'value': 301}}, {'key': 37, 'value': {'key': 600, 'value': 601}}]"\
         == str(transaction_json['rows'][0]['mp']), 'Content of multi-index table map< pair< uint16_t >> is not correct'

    assert "[{'key': 1, 'value': {'field_0': 10, 'field_1': 11}}, {'key': 2, 'value': {'field_0': 200, 'field_1': 300}}]"\
         == str(transaction_json['rows'][0]['mt']), 'Content of multi-index table map< uint16_t, tuple< uint16_t, uint16_t >> is not correct'


    assert "{'key': 20, 'value': [200, 202]}" == str(transaction_json['rows'][0]['pst']), \
        'Content of multi-index table pair< set< uint16_t >> is not correct'

    assert "{'key': 10, 'value': [100, 100, 102]}" == str(transaction_json['rows'][0]['pv']), \
        'Content of multi-index table pair< vector< uint16_t >> is not correct'

    assert "{'key': 70, 'value': 71}" == str(transaction_json['rows'][0]['po']), \
        'Content of multi-index table pair< optional< uint16_t >>  is not correct'

    assert "{'key': 70, 'value': None}" == str(transaction_json['rows'][1]['po']), \
        'Content of multi-index table pair< optional< uint16_t >>  is not correct'

    assert "{'key': 6, 'value': [{'key': 20, 'value': 300}, {'key': 21, 'value': 301}]}" \
        == str(transaction_json['rows'][0]['pm']), 'Content of multi-index table pair< map< uint16_t >> is not correct'
        
    assert "{'key': 30, 'value': {'key': 301, 'value': 302}}" == str(transaction_json['rows'][0]['pp']),\
         'Content of multi-index table pair< pair< uint16_t >> is not correct'

    assert "{'key': 10, 'value': {'field_0': 100, 'field_1': 101}}"\
         == str(transaction_json['rows'][0]['pt']), 'Content of multi-index table pair< uint16_t, tuple< uint16_t, uint16_t >> is not correct'


    assert "{'field_0': 10, 'field_1': [21, 31], 'field_2': [41, 51, 61]}" == str(transaction_json['rows'][0]['tst']), \
        'Content of multi-index table tuple< set< uint16_t >> is not correct'

    assert "{'field_0': 16, 'field_1': [26, 36], 'field_2': [46, 506, 606]}" == str(transaction_json['rows'][0]['tv']), \
        'Content of multi-index table tuple< vector< uint16_t >> is not correct'

    assert "{'field_0': 100, 'field_1': None, 'field_2': 200, 'field_3': None, 'field_4': 300}" == str(transaction_json['rows'][0]['to']), \
        'Content of multi-index table tuple< optional< uint16_t >>  is not correct'

    assert "{'field_0': None, 'field_1': None, 'field_2': 10, 'field_3': None, 'field_4': 20}" == str(transaction_json['rows'][1]['to']), \
        'Content of multi-index table tuple< optional< uint16_t >>  is not correct'

    assert "{'field_0': 126, 'field_1': [{'key': 10, 'value': 100}, {'key': 11, 'value': 101}], 'field_2': [{'key': 80, 'value': 800}, {'key': 81, 'value': 9009}]}" \
        == str(transaction_json['rows'][0]['tm']), 'Content of multi-index table pair< map< uint16_t, uint16_t >> is not correct'
        
    assert "{'field_0': 127, 'field_1': {'key': 18, 'value': 28}, 'field_2': {'key': 19, 'value': 29}}" == str(transaction_json['rows'][0]['tp']),\
         'Content of multi-index table tuple< pair< uint16_t, uint16_t >> is not correct'

    assert "{'field_0': {'field_0': 1, 'field_1': 2}, 'field_1': {'field_0': 30, 'field_1': 40}, 'field_2': {'field_0': 50, 'field_1': 60}}"\
         == str(transaction_json['rows'][0]['tt']), 'Content of multi-index table tuple< tuple< uint16_t, uint16_t >, ...> is not correct'


    assert "[{'_count': 18, '_strID': 'dumstr'}, None, {'_count': 19, '_strID': 'dumstr'}]" == str(transaction_json['rows'][0]['vos']), \
         "Content of multi-index table vector<optional<mystruct>> is not correct"
    
    assert "{'first': 183, 'second': [100, None, 200]}" == str(transaction_json['rows'][0]['pvo']), \
         "Content of multi-index table pair<uint16_t, vector<optional<uint16_t>>> is not correct"
        
    testSuccessful=True            
    assert testSuccessful
    
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet)

exit(0)