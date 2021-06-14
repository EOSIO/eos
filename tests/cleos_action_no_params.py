#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from testUtils import ReturnType
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper
import subprocess  # for subprocess.CalledProcessError

###############################################################
# cleos_action_no_params
#
# Tests that cleos does not report error when no "data" field 
# is present in action results.  This happens if contract action
# does not have anhy parameters
#
# To easily test this against an older version, simply copy 
# cleos executable from older build into programs/cleos/cleos 
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
contractDir='unittests/contracts/'
wasmFile='test_action_no_param.wasm'
abiFile='test_action_no_param.abi'

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

    Print("Creating test account")
    testacct = Account('testacct')
    testacct.ownerPublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    testacct.activePublicKey = EOSIO_ACCT_PUBLIC_DEFAULT_KEY
    cluster.createAccountAndVerify(testacct, cluster.eosioAccount, buyRAM=70000)

    Print("Validating accounts after bootstrap")
    cluster.validateAccounts([testacct])

    node = cluster.getNode()

    Print("Loading contract")
    node.publishContract(testacct, contractDir, wasmFile, abiFile, waitForTransBlock=True)

    Print("Test action with no parameters")
    cmdArr= [Utils.EosClientPath, '-v', 'push', 'action', 'testacct', 'foo', '[]', '-p', 'testacct@active']
    clargs = node.eosClientArgs().split()
    for x in clargs[::-1]:
        cmdArr.insert(1, x)
    # we need to use checkoutput here because pushMessages uses the -j option which suppresses error message
    try:
        result = Utils.checkOutput(cmdArr, ignoreError=False)
        Print("result= ", result)

        testSuccessful=True
    except subprocess.CalledProcessError as e:
        Print('error running cleos')
        Print('cmd= ', e.cmd)
        Print('output= ', e.output.decode("utf-8"))        
        
    assert testSuccessful
    
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet)

exit(0)
