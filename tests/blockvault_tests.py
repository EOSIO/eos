#!/usr/bin/env python3

from testUtils import Utils
import testUtils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from TestHelper import TestHelper
from core_symbol import CORE_SYMBOL

import time
import decimal
import math
import re
import shutil
import signal
import subprocess
import os
import os.path
import atexit
###############################################################
# nodeos_producer_watermark_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################

class PostgresDocker:
    def __init__(self, args):
        if args.clean_run:
            subprocess.call(['docker', 'rm', '-f', 'test_postgresql'])
        subprocess.check_call(['docker', 'run', '--rm', '-p' '127.0.0.1:5432:5432', '--name', 'test_postgresql', '-e', 'POSTGRES_PASSWORD=password', '-d', 'postgres',])
        if not args.leave_running:
            atexit.register(subprocess.call, ['docker', 'rm', '-f', 'test_postgresql'])
        while subprocess.call(['docker', 'run', '--rm', '--link', 'test_postgresql:pg', 'postgres', 'pg_isready', '-U', 'postgres', '-h', 'pg']) != 0:
            time.sleep(1)

    def execute(self, stmt):
        return subprocess.check_output(['docker', 'run', '--rm', '-e', 'PGPASSWORD=password', '--link', 'test_postgresql:pq', 'postgres', 'psql', '-U', 'postgres', '-h', 'pq', '-q', '-c', stmt]).splitlines()
        
    def num_rows_in_table(self, tableName):
        return int(self.execute('SELECT COUNT(*) FROM {};'.format(tableName))[2])

class PostgressCI:
    def __init__(self):
        subprocess.check_all(['pg_ctlcluster', '13', 'main', 'start'])
        atexit.register(subprocess.call, ['pg_ctlcluster', '13', 'main', 'stop'])
        subprocess.check_all(['su', '-c', "psql  --command \"ALTER USER postgres WITH PASSWORD 'password';\"", '-', 'postgres'])

    def execute(self, stmt):
        return subprocess.check_output(['su', '-c', "psql  --command \"{};\"".format(stmt), '-', 'postgres']).splitlines()

    def num_rows_in_table(self, tableName):
        return int(self.execute('SELECT COUNT(*) FROM {};'.format(tableName))[2])

def testFailOver(cluster):
    node0=cluster.getNode(0)
    node1=cluster.getNode(1)
    Print("Kill node 1 and then remove the state and blocks")
    node1.kill(signal.SIGTERM)
    shutil.rmtree("var/lib/node_01/state")
    shutil.rmtree("var/lib/node_01/blocks")

    blockNumAfterNode1Killed = node0.getHeadBlockNum()

    assert node1.relaunch(timeout=30, newChain=True, cachePopen=True), "Fail to relaunch"
    assert node0.waitForIrreversibleBlockProducedBy("vltproducera", blockNumAfterNode1Killed, retry=30), "failed to see blocks produced by vltproducera"


Print=Utils.Print
errorExit=Utils.errorExit

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run",
                              "--wallet-port"})
Utils.Debug=args.v
totalNodes=2
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
    
    Print("start postgres")
    postgres = None
    if shutil.which('docker'):
        postgres = PostgresDocker(args)
    elif shutil.which('pg_ctlcluster') and os.environ['CI'] == "true":
        postgres = PostgresCI()

    TestHelper.printSystemInfo("BEGIN")
    cluster.setWalletMgr(walletMgr)

    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    extraProducerAccounts = Cluster.createAccountKeys(1)
    vltproducerAccount = extraProducerAccounts[0]

    Print("Stand up cluster")
    if cluster.launch(onlyBios=False, pnodes=1, totalNodes=totalNodes,
                    useBiosBootFile=False, onlySetProds=False,
                    extraNodeosArgs=" --blocks-log-stride 20 --max-retained-block-files 3",
                    specificExtraNodeosArgs={
                        1:"--plugin eosio::blockvault_client_plugin --block-vault-backend postgresql://postgres:password@localhost"},
                    manualProducerNodeConf={ 1: { 'key': vltproducerAccount, 'names': ['vltproducera']}}) is False:
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    node0 = cluster.getNode(0)
    node1 = cluster.getNode(1)

    Print("Wait for blocks produced by vltproducera(node 1) seen by node 0")
    assert node0.waitForIrreversibleBlockProducedBy("vltproducera"), "failed to see blocks produced by vltproducera"

    Print("Wait until block 1 is no longer retrievable, this ensures newly joined nodeos cannot sync from net plugin")
    while os.path.exists('var/lib/node_00/archive/blocks-1-20.log'):
        time.sleep(2)

    Print("Testing recovering without snapshot...")
    testFailOver(cluster)
    
    Print("Testing recovering from snapshot...")
    Print("Create a snapshot")
    node1.createSnapshot()
    Print("Wait until the snapshot appears in the database")
    Utils.waitForTruth(lambda: postgres.num_rows_in_table('SnapshotData') != 0)  
    testFailOver(cluster)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exit(0)
