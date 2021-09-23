#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from testUtils import ReturnType
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper
import json

###############################################################
# Nested_container_kv_test
#
# Load nested container contracts for kv table
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
contractDir='unittests/test-contracts/nested_container_kv'
wasmFile='nested_container_kv.wasm'
abiFile='nested_container_kv.abi'
kvcontractDir='unittests/contracts'
kvWasmFile='kv_bios.wasm'
kvAbiFile='kv_bios.abi'

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

    Print("Creating kv account")
    kvacct = Account('nestcontn2kv')
    kvacct.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    kvacct.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(kvacct, cluster.eosioAccount, buyRAM=7000000)
    Print("Creating user account")
    useracct = Account('alice')
    useracct.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    useracct.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(useracct, cluster.eosioAccount, buyRAM=7000000)

    Print("Validating accounts")
    cluster.validateAccounts([kvacct, useracct])

    node = cluster.getNode()

    Print("Setting kv settings account privileged")
    node.pushMessage(cluster.eosioAccount.name, 'setpriv', '["nestcontn2kv", 1]', '-p eosio@active')
    node.publishContract(cluster.eosioAccount, kvcontractDir, kvWasmFile, kvAbiFile, waitForTransBlock=True)
    node.pushMessage(cluster.eosioAccount.name, 'ramkvlimits', '[2024, 2024, 2024]', '-p eosio@active')

    Print("Loading nested container kv contract")
    node.publishContract(kvacct, contractDir, wasmFile, abiFile, waitForTransBlock=True)

    Print("Test action for set< set< uint16_t >>")
    create_action('setstst', '[1, [[10, 10], [3], [400, 500, 600]]]', 'nestcontn2kv', 'alice')

    Print("Test action for set< vector< uint16_t >>")
    create_action('setstv', '[1, [[16, 26], [36], [36], [46, 506, 606]]]', 'nestcontn2kv', 'alice')

    Print("Test action for set< optional< uint16_t >>")
    create_action('setsto', '[1, [null, null, 500]]', 'nestcontn2kv', 'alice')

    Print("Test action for set< map< uint16_t, uint16_t >>")
    create_action('setstm', '[1, [[{"key":30,"value":300},{"key":30,"value":300}],[{"key":60,"value":600},{"key":60,"value":600}]]]', 'nestcontn2kv', 'alice')

    Print("Test action for set< pair< uint16_t, uint16_t >>")
    create_action('setstp', '[1, [{"key": 69, "value": 129}, {"key": 69, "value": 129}]]', 'nestcontn2kv', 'alice')

    Print("Test action for set< tuple< uint16_t, uint16_t >>")
    create_action('setstt', '[1, [[1,2],[36,46], [56,66]]]', 'nestcontn2kv', 'alice')

    
    Print("Test action for vector< set< uint16_t >>")
    create_action('setvst', '[1, [[10, 10], [3], [400, 500, 600]]]', 'nestcontn2kv', 'alice')

    Print("Test action for vector< vector< uint16_t >>")
    create_action('setvv', '[1, [[16, 26], [36], [36], [46, 506, 606]]]', 'nestcontn2kv', 'alice')

    Print("Test action for vector< optional< uint16_t >>")
    create_action('setvo', '[1,[null, null, 500]]', 'nestcontn2kv', 'alice')

    Print("Test action for vector< map< uint16_t, uint16_t >>")
    create_action('setvm', '[1, [[{"key": 30, "value": 300}, {"key": 30, "value": 300}], [{"key": 60, "value": 600}, {"key": 60, "value": 600}]]]'
    , 'nestcontn2kv', 'alice')

    Print("Test action for vector< pair< uint16_t, uint16_t >>")
    create_action('setvp', '[1, [{"key": 69, "value": 129}, {"key": 69, "value": 129}]]', 'nestcontn2kv', 'alice')

    Print("Test action for vector< tuple< uint16_t, uint16_t >>")
    create_action('setvt', '[1, [[10,20],[30,40], [50,60]]]', 'nestcontn2kv', 'alice')


    Print("Test action for optional< set< uint16_t >>")
    create_action('setost', '[1, [10, 10, 3]]', 'nestcontn2kv', 'alice')
    create_action('setost', '[2, null]', 'nestcontn2kv', 'alice')

    Print("Test action for optional< vector< uint16_t >>")
    create_action('setov', '[1, [46, 506, 606]]', 'nestcontn2kv', 'alice')
    create_action('setov', '[2, null]', 'nestcontn2kv', 'alice')

    Print("Test action for optional< optional< uint16_t >>")
    create_action('setoo', '[1, 500]', 'nestcontn2kv', 'alice')
    create_action('setoo', '[2, null]', 'nestcontn2kv', 'alice')

    Print("Test action for optional< map< uint16_t, uint16_t >>")
    create_action('setom', '[1, [{"key": 10, "value": 1000}, {"key": 11,"value": 1001}] ]', 'nestcontn2kv', 'alice')
    create_action('setom', '[2, null]', 'nestcontn2kv', 'alice')

    Print("Test action for optional< pair< uint16_t, uint16_t >>")
    create_action('setop', '[1, {"key": 60, "value": 61}]', 'nestcontn2kv', 'alice')
    create_action('setop', '[2, null]', 'nestcontn2kv', 'alice')

    Print("Test action for optional< tuple< uint16_t, uint16_t >>")
    create_action('setot', '[1, [1001,2001]]', 'nestcontn2kv', 'alice')
    create_action('setot', '[2, null]', 'nestcontn2kv', 'alice')


    Print("Test action for map< set< uint16_t >>")
    create_action('setmst', '[1, [{"key": 1,"value": [10, 10, 12, 16]},  {"key": 2, "value": [200, 300]} ]]', 'nestcontn2kv', 'alice')

    Print("Test action for map< vector< uint16_t >>")
    create_action('setmv', '[1, [{"key": 1, "value": [10, 10, 12, 16]},  {"key": 2, "value": [200, 300]} ]]', 'nestcontn2kv', 'alice')

    Print("Test action for map< optional< uint16_t >>")
    create_action('setmo', '[1, [{"key": 10, "value": 1000}, {"key": 11, "value": null}]]', 'nestcontn2kv', 'alice')

    Print("Test action for map< map< uint16_t, uint16_t >>")
    create_action('setmm', '[1, [{"key": 10, "value": [{"key": 200, "value": 2000}, \
         {"key": 201, "value": 2001}] }, {"key": 11, "value": [{"key": 300, "value": 3000}, {"key": 301, "value": 3001}] } ]]', 'nestcontn2kv', 'alice')

    Print("Test action for map< pair< uint16_t, uint16_t >>")
    create_action('setmp', '[1, [{"key": 36, "value": {"key": 300, "value": 301}}, {"key": 37, "value": {"key": 600, "value": 601}} ]]', 'nestcontn2kv', 'alice')

    Print("Test action for map< tuple< uint16_t, uint16_t >>")
    create_action('setmt', '[1, [{"key":1,"value":[10,11]},  {"key":2,"value":[200,300]} ]]', 'nestcontn2kv', 'alice')


    Print("Test action for pair< set< uint16_t >>")
    create_action('setpst', '[1, {"key": 20, "value": [200, 200, 202]}]', 'nestcontn2kv', 'alice')

    Print("Test action for pair< vector< uint16_t >>")
    create_action('setpv', '[1, {"key": 10, "value": [100, 100, 102]}]', 'nestcontn2kv', 'alice')

    Print("Test action for pair< optional< uint16_t >>")
    create_action('setpo', '[1, {"key": 70, "value": 71}]', 'nestcontn2kv', 'alice')
    create_action('setpo', '[2, {"key": 70, "value": null}]', 'nestcontn2kv', 'alice')

    Print("Test action for pair< map< uint16_t, uint16_t >>")
    create_action('setpm', '[1, {"key": 6, "value": [{"key": 20, "value": 300}, {"key": 21,"value": 301}] }]', 'nestcontn2kv', 'alice')

    Print("Test action for pair< pair< uint16_t, uint16_t >>")
    create_action('setpp', '[1, {"key": 30, "value": {"key": 301, "value": 302} }]', 'nestcontn2kv', 'alice')

    Print("Test action for pair< tuple< uint16_t, uint16_t >>")
    create_action('setpt', '[1, {"key":10, "value":[100,101]}]', 'nestcontn2kv', 'alice')


    Print("Test action for tuple< uint16_t, set< uint16_t >, set< uint16_t >>")
    create_action('settst', '[1, [10,[21,31], [41,51,61]]]', 'nestcontn2kv', 'alice')

    Print("Test action for tuple< uint16_t, vector< uint16_t >, vector< uint16_t >")
    create_action('settv', '[1, [16,[26,36], [46,506,606]]]', 'nestcontn2kv', 'alice')

    Print("Test action for tuple< optional< uint16_t >, optional< uint16_t >, optional< uint16_t > \
    , optional< uint16_t >, optional< uint16_t >>")
    create_action('setto', '[1, [100, null, 200, null, 300]]', 'nestcontn2kv', 'alice')
    create_action('setto', '[2, [null, null, 10, null, 20]]', 'nestcontn2kv', 'alice')

    Print("Test action for tuple< map< uint16_t, uint16_t >, map< uint16_t, uint16_t >>")
    create_action('settm', '[1, [126, [{"key":10,"value":100},{"key":11,"value":101}], [{"key":80,"value":800},{"key":81,"value":9009}] ]]', 'nestcontn2kv', 'alice')

    Print("Test action for tuple< uint16_t, pair< uint16_t, uint16_t >, pair< uint16_t, uint16_t >>")
    create_action('settp', '[1, [127, {"key":18, "value":28}, {"key":19, "value":29}]]', 'nestcontn2kv', 'alice')

    Print("Test action for tuple< tuple< uint16_t, uint16_t >, tuple< uint16_t, uint16_t >, tuple< uint16_t, uint16_t >>")
    create_action('settt', '[1, [[1,2],[30,40], [50,60]]]', 'nestcontn2kv', 'alice')


    Print("Test action for vector<optional<mystruct>>")
    create_action('setvos', '[1, [{"_count": 18, "_strID": "dumstr"}, null, {"_count": 19, "_strID": "dumstr"}]]', 'nestcontn2kv', 'alice')

    Print("Test action for pair<uint16_t, vector<optional<uint16_t>>>")
    create_action('setpvo', '[1,{"first": 183, "second":[100, null, 200]}]', 'nestcontn2kv', 'alice')

    cmd="get kv_table %s people2kv map.index" % kvacct.name
    transaction = node.processCleosCmd(cmd, cmd, False, returnType=ReturnType.raw)
    transaction_json = json.loads(transaction)

    assert "[[3], [10], [400, 500, 600]]" == str(transaction_json['rows'][0]['stst']), \
        'Content of kv table set< set< uint16_t >> is not correct'

    assert "[[16, 26], [36], [46, 506, 606]]" == str(transaction_json['rows'][0]['stv']), \
        'Content of kv table set< vector< uint16_t >> is not correct'

    assert "[None, 500]" == str(transaction_json['rows'][0]['sto']), 'Content of kv table set< optional< uint16_t >>  is not correct'

    assert "[[{'key': 30, 'value': 300}], [{'key': 60, 'value': 600}]]" \
        == str(transaction_json['rows'][0]['stm']), 'Content of kv table set< map< uint16_t >> is not correct'

    assert "[{'key': 69, 'value': 129}]" == str(transaction_json['rows'][0]['stp']), \
        'Content of kv table set< pair< uint16_t >> is not correct'

    assert "[{'field_0': 1, 'field_1': 2}, {'field_0': 36, 'field_1': 46}, {'field_0': 56, 'field_1': 66}]" == str(transaction_json['rows'][0]['stt']), \
        'Content of kv table set< tuple< uint16_t, uint16_t >> is not correct'



    assert "[[10], [3], [400, 500, 600]]" == str(transaction_json['rows'][0]['vst']), \
        'Content of kv table vector< set< uint16_t >> is not correct'

    assert "[[16, 26], [36], [36], [46, 506, 606]]" == str(transaction_json['rows'][0]['vv']), \
        'Content of kv table vector< vector< uint16_t >> is not correct'

    assert "[None, None, 500]" == str(transaction_json['rows'][0]['vo']), \
        'Content of kv table vector< optional< uint16_t >>  is not correct'

    assert "[[{'key': 30, 'value': 300}], [{'key': 60, 'value': 600}]]" \
        == str(transaction_json['rows'][0]['vm']), 'Content of kv table vector< map< uint16_t >> is not correct'

    assert "[{'key': 69, 'value': 129}, {'key': 69, 'value': 129}]" == str(transaction_json['rows'][0]['vp']), \
        'Content of kv table vector< pair< uint16_t >> is not correct'

    assert "[{'field_0': 10, 'field_1': 20}, {'field_0': 30, 'field_1': 40}, {'field_0': 50, 'field_1': 60}]" == str(transaction_json['rows'][0]['vt']), \
        'Content of kv table vector< tuple< uint16_t, uint16_t >> is not correct'


    assert "[3, 10]" == str(transaction_json['rows'][0]['ost']), 'Content of kv table optional< set< uint16_t >> is not correct'
    assert "None" == str(transaction_json['rows'][1]['ost']), 'Content of kv table optional< set< uint16_t >> is not correct'

    assert "[46, 506, 606]" == str(transaction_json['rows'][0]['ov']), 'Content of kv table optional< vector< uint16_t >> is not correct'
    assert "None" == str(transaction_json['rows'][1]['ov']), 'Content of kv table optional< set< uint16_t >> is not correct'

    assert "500" == str(transaction_json['rows'][0]['oo']), 'Content of kv table optional< optional< uint16_t >>  is not correct'
    assert "None" == str(transaction_json['rows'][1]['oo']), 'Content of kv table optional< set< uint16_t >> is not correct'

    assert "[{'key': 10, 'value': 1000}, {'key': 11, 'value': 1001}]" \
        == str(transaction_json['rows'][0]['om']), 'Content of kv table optional< map< uint16_t >> is not correct'
    assert "None" == str(transaction_json['rows'][1]['om']), 'Content of kv table optional< set< uint16_t >> is not correct'

    assert "{'key': 60, 'value': 61}" == str(transaction_json['rows'][0]['op']), \
        'Content of kv table optional< pair< uint16_t >> is not correct'
    assert "None" == str(transaction_json['rows'][1]['op']), 'Content of kv table optional< set< uint16_t >> is not correct'

    assert "{'field_0': 1001, 'field_1': 2001}" == str(transaction_json['rows'][0]['ot']), 'Content of kv table optional< tuple< uint16_t, uint16_t >>  is not correct'
    assert "None" == str(transaction_json['rows'][1]['ot']), 'Content of kv table optional< tuple< uint16_t, uint16_t >> is not correct'


    assert "[{'key': 1, 'value': [10, 12, 16]}, {'key': 2, 'value': [200, 300]}]" \
        == str(transaction_json['rows'][0]['mst']), 'Content of kv table map< set< uint16_t >> is not correct'

    assert "[{'key': 1, 'value': [10, 10, 12, 16]}, {'key': 2, 'value': [200, 300]}]" \
        == str(transaction_json['rows'][0]['mv']), 'Content of kv table map< vector< uint16_t >> is not correct'

    assert "[{'key': 10, 'value': 1000}, {'key': 11, 'value': None}]" == \
        str(transaction_json['rows'][0]['mo']), 'Content of kv table map< optional< uint16_t >> is not correct'

    assert "[{'key': 10, 'value': [{'key': 200, 'value': 2000}, {'key': 201, 'value': 2001}]}, {'key': 11, 'value': [{'key': 300, 'value': 3000}, {'key': 301, 'value': 3001}]}]" \
        == str(transaction_json['rows'][0]['mm']), 'Content of kv table map< map< uint16_t >> is not correct'

    assert "[{'key': 36, 'value': {'key': 300, 'value': 301}}, {'key': 37, 'value': {'key': 600, 'value': 601}}]"\
         == str(transaction_json['rows'][0]['mp']), 'Content of kv table map< pair< uint16_t >> is not correct'

    assert "[{'key': 1, 'value': {'field_0': 10, 'field_1': 11}}, {'key': 2, 'value': {'field_0': 200, 'field_1': 300}}]"\
         == str(transaction_json['rows'][0]['mt']), 'Content of kv table map< uint16_t, tuple< uint16_t, uint16_t >> is not correct'


    assert "{'key': 20, 'value': [200, 202]}" == str(transaction_json['rows'][0]['pst']), \
        'Content of kv table pair< set< uint16_t >> is not correct'

    assert "{'key': 10, 'value': [100, 100, 102]}" == str(transaction_json['rows'][0]['pv']), \
        'Content of kv table pair< vector< uint16_t >> is not correct'

    assert "{'key': 70, 'value': 71}" == str(transaction_json['rows'][0]['po']), \
        'Content of kv table pair< optional< uint16_t >>  is not correct'

    assert "{'key': 70, 'value': None}" == str(transaction_json['rows'][1]['po']), \
        'Content of kv table pair< optional< uint16_t >>  is not correct'

    assert "{'key': 6, 'value': [{'key': 20, 'value': 300}, {'key': 21, 'value': 301}]}" \
        == str(transaction_json['rows'][0]['pm']), 'Content of kv table pair< map< uint16_t >> is not correct'
        
    assert "{'key': 30, 'value': {'key': 301, 'value': 302}}" == str(transaction_json['rows'][0]['pp']),\
         'Content of kv table pair< pair< uint16_t >> is not correct'

    assert "{'key': 10, 'value': {'field_0': 100, 'field_1': 101}}"\
         == str(transaction_json['rows'][0]['pt']), 'Content of kv table pair< uint16_t, tuple< uint16_t, uint16_t >> is not correct'


    assert "{'field_0': 10, 'field_1': [21, 31], 'field_2': [41, 51, 61]}" == str(transaction_json['rows'][0]['tst']), \
        'Content of kv table tuple< set< uint16_t >> is not correct'

    assert "{'field_0': 16, 'field_1': [26, 36], 'field_2': [46, 506, 606]}" == str(transaction_json['rows'][0]['tv']), \
        'Content of kv table tuple< vector< uint16_t >> is not correct'

    assert "{'field_0': 100, 'field_1': None, 'field_2': 200, 'field_3': None, 'field_4': 300}" == str(transaction_json['rows'][0]['to']), \
        'Content of kv table tuple< optional< uint16_t >>  is not correct'

    assert "{'field_0': None, 'field_1': None, 'field_2': 10, 'field_3': None, 'field_4': 20}" == str(transaction_json['rows'][1]['to']), \
        'Content of kv table tuple< optional< uint16_t >>  is not correct'

    assert "{'field_0': 126, 'field_1': [{'key': 10, 'value': 100}, {'key': 11, 'value': 101}], 'field_2': [{'key': 80, 'value': 800}, {'key': 81, 'value': 9009}]}" \
        == str(transaction_json['rows'][0]['tm']), 'Content of kv table pair< map< uint16_t, uint16_t >> is not correct'
        
    assert "{'field_0': 127, 'field_1': {'key': 18, 'value': 28}, 'field_2': {'key': 19, 'value': 29}}" == str(transaction_json['rows'][0]['tp']),\
         'Content of kv table tuple< pair< uint16_t, uint16_t >> is not correct'

    assert "{'field_0': {'field_0': 1, 'field_1': 2}, 'field_1': {'field_0': 30, 'field_1': 40}, 'field_2': {'field_0': 50, 'field_1': 60}}"\
         == str(transaction_json['rows'][0]['tt']), 'Content of kv table tuple< tuple< uint16_t, uint16_t >, ...> is not correct'


    assert "[{'_count': 18, '_strID': 'dumstr'}, None, {'_count': 19, '_strID': 'dumstr'}]" == str(transaction_json['rows'][0]['vos']), \
         "Content of kv table vector<optional<mystruct>> is not correct"
    
    assert "{'first': 183, 'second': [100, None, 200]}" == str(transaction_json['rows'][0]['pvo']), \
         "Content of kv table pair<uint16_t, vector<optional<uint16_t>>> is not correct"
        
    testSuccessful=True     
        
    assert testSuccessful
    
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet)

exit(0)