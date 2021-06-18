#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper


import time
import shutil
import signal
import subprocess
import atexit

def killAndRelaunch(cluster, nodeToKill, addSwapFlags={}):
    nodeId = nodeToKill.nodeId
    Print("Kill node %d and then remove the state and blocks" % nodeId)

    nodeToKill.kill(signal.SIGTERM)
    shutil.rmtree(Utils.getNodeDataDir(nodeId,"state"))
    shutil.rmtree(Utils.getNodeDataDir(nodeId,"blocks"))

    assert nodeToKill.relaunch(timeout=30, skipGenesis=False, cachePopen=True, addSwapFlags=addSwapFlags), "Fail to relaunch"

def changePostgressPassword(password):
    stmt = "ALTER USER postgres WITH PASSWORD '{}';".format(password)
    subprocess.check_output([ "scripts/postgres_control.sh", "exec", stmt])

def num_rows_in_table(tableName):
    stmt = 'SELECT COUNT(*) FROM {};'.format(tableName)
    return int(subprocess.check_output([ "scripts/postgres_control.sh", "exec", stmt]).splitlines()[2])


Print=Utils.Print
errorExit=Utils.errorExit

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run",
                              "--wallet-port"})
Utils.Debug=args.v
producers=1
apiNodes=2
totalNodes=producers+apiNodes
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
killAll=args.clean_run
walletPort=args.wallet_port

walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName=Utils.EosWalletName
ClientName="cleos"

try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.setWalletMgr(walletMgr)

    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    Print("start postgres")
    if subprocess.run(['scripts/postgres_control.sh', 'status']):
        subprocess.check_call(['scripts/postgres_control.sh', 'stop'])
    
    reset_statement="DROP TABLE IF EXISTS BlockData; DROP TABLE IF EXISTS SnapshotData; SELECT lo_unlink(l.oid) FROM pg_largeobject_metadata l;"
    subprocess.check_call(['scripts/postgres_control.sh', 'start', reset_statement])
    if not dontKill:
        atexit.register(subprocess.call, ['scripts/postgres_control.sh', 'stop'])

    extraProducerAccounts = Cluster.createAccountKeys(1)
    vltproducerAccount = extraProducerAccounts[0]

    Print("Launch a cluster")
    # Producer: node 1, Blockvault access: node 1 and node 2, node 0 standby
    if cluster.launch(onlyBios=False, pnodes=1, totalNodes=totalNodes,
                    useBiosBootFile=False, onlySetProds=False, configSecurityGroup=True,
                    extraNodeosArgs=" --blocks-log-stride 20 --max-retained-block-files 3",
                    specificExtraNodeosArgs={
                        1:"--plugin eosio::blockvault_client_plugin --block-vault-backend postgresql://postgres:password@localhost",
                        2:"--plugin eosio::blockvault_client_plugin --block-vault-backend postgresql://postgres:password@localhost"},
                    manualProducerNodeConf={ 1: { 'key': vltproducerAccount, 'names': ['vltproducera']}}) is False:
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    node0 = cluster.getNode(0)
    node1 = cluster.getNode(1)
    node2 = cluster.getNode(2)

    securityGroup = cluster.getSecurityGroup()

    time.sleep(10)

    Print("#################################################################################")
    Print("# Scenario 1: No security group defined#")
    Print("#################################################################################")

    Print("Wait for blocks produced by vltproducera(node 1) seen by node 0")
    assert node0.waitForLibToAdvance(), "failed to see blocks produced by vltproducera"

    Print("Wait for blocks produced by vltproducera(node 1) seen by node 2")
    assert node2.waitForLibToAdvance(), "failed to see blocks produced by vltproducera"


    node2.createSnapshot()
    Print("Wait until the snapshot appears on postgres")
    assert Utils.waitForTruth(lambda: num_rows_in_table('SnapshotData') != 0), "Node 2 doesn't have access to blockvault" 

    Print("#################################################################################")
    Print("# Scenario 2: Adding node 0 and node 1 to a security group, check access of node 2 and node 0 to blocks produced by node 1#")
    Print("#################################################################################")

    Print("Add nodes 0 and 1 to security group")
    securityGroup.editSecurityGroup(addNodes=[node0, node1])
    securityGroup.verifySecurityGroup()

    Print("### change postgre password to restrict node 2 access to it ###")
    changePostgressPassword('newpassword')
    killAndRelaunch(cluster, nodeToKill=node1, addSwapFlags={
        "--plugin": "eosio::blockvault_client_plugin",
        "--block-vault-backend": "postgresql://postgres:newpassword@localhost",
        "--producer-name": "vltproducera",
        "--signature-provider": "{}=KEY:{}".format(vltproducerAccount.ownerPublicKey, vltproducerAccount.ownerPrivateKey)
    })

    Print("Wait for blocks produced by vltproducera(node 1) seen by node 0")
    assert node0.waitForLibToAdvance(60), "failed to see blocks produced by vltproducera"

    Print("Wait for blocks produced by vltproducera(node 1) seen by node 2")
    assert not node2.waitForLibToAdvance(60), "Node 2 should NOT see blocks produced by vltproducera"

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exit(0)