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
import glob

############################################################################################################################################
# blockvault_test.py
#
# This script requires a PostgreSQL server to work. There are 3 ways to control how to start or use PostgreSQL
#
# 1. The program `docker` can be executed in current environment and can be executed without sudo: the script would create PostgreSQL implicitly.
# 2. The program `pg_ctlcluster` or `pg_ctl` can be executed in current environment and the 'CI' environment variable is 'true': the script would use 
#    'pg_ctlcluster' or `pg_ctl` to start the PostgreSQL server. It is intended to be used by CI docker container only.
# 3. The program 'psql' can be executed in current environment and the environment variables PGHOST, PGPORT, PGPASSWORD, PGUSER are properly
#    defined to access an running PostgreSQL server. Notice that in the mode, the existing database tables would be dropped. DO NOT use this
#    mode to connect to a production PostgreSQL server. 
#
# Overview:
#   The script creates a cluster with a bios and 3 additional node.
#    
#   node 0 : a nodeos with 21 producers 'defproducera', 'defproducerb', ...., 'defproduceru'
#   node 1 : a nodeos with one producer 'vltproducera' and configured to connect to a block vault.
#   node 2 : initially without any producer configured. After it is killed in scenario 3, it is relaunched with
#            one producer 'vltproducera' and uses the same producer key in node 1. 
#
#############################################################################################################################################


logging="""{
  "includes": [],
  "appenders": [{
      "name": "stderr",
      "type": "console",
      "args": {
        "stream": "std_error",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    },{
      "name": "stdout",
      "type": "console",
      "args": {
        "stream": "std_out",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    }
  ],
  "loggers": [{
      "name": "default",
      "level": "info",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    },{
      "name": "net_plugin_impl",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    },{
      "name": "http_plugin",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    },{
      "name": "producer_plugin",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    },{
      "name": "blockvault_client_plugin",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    }
  ]
}"""

loggingFile = os.path.join(os.getcwd(), "logging.json")

with open(loggingFile, "w") as textFile:
    print(logging,file=textFile)

def num_rows_in_table(tableName):
    stmt = 'SELECT COUNT(*) FROM {};'.format(tableName)
    return int(subprocess.check_output([ "scripts/postgres_control.sh", "exec", stmt]).splitlines()[2])

def testFailOver(cluster, nodeToKill, addSwapFlags={}):
    node0 = cluster.getNode(0)
    
    nodeId = nodeToKill.nodeId
    Print("Kill node %d and then remove the state and blocks" % nodeId)

    nodeToKill.kill(signal.SIGTERM)
    shutil.rmtree(Utils.getNodeDataDir(nodeId,"state"))
    shutil.rmtree(Utils.getNodeDataDir(nodeId,"blocks"))

    blockNumAfterNode1Killed = node0.getHeadBlockNum()

    assert nodeToKill.relaunch(timeout=30, skipGenesis=False, cachePopen=True, addSwapFlags=addSwapFlags), "Fail to relaunch"
    assert node0.waitForIrreversibleBlockProducedBy("vltproducera", blockNumAfterNode1Killed, retry=30), "failed to see blocks produced by vltproducera"

def get_successful_constructed_block_numbers_in_file(filename):
  result = []
  with open(filename, "r") as f:
    for line in f:
      m = re.search("propose_constructed_block\(watermark=\{(\d+), \d+\}, lib=\d+\) returns true", line)
      if m:
        result.append(int(m.group(1)))
  return result

def get_successful_constructed_block_numbers_for_node(nodeId):
  result = []
  for filename in glob.glob(os.path.join(Utils.getNodeDataDir(nodeId), 'stderr.*.txt')):
    result.extend(get_successful_constructed_block_numbers_in_file(filename))
  return set(result)

Print=Utils.Print
errorExit=Utils.errorExit

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run",
                              "--wallet-port"})
Utils.Debug=args.v
totalNodes=3
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
    reset_statement="DROP TABLE IF EXISTS BlockData; DROP TABLE IF EXISTS SnapshotData; SELECT lo_unlink(l.oid) FROM pg_largeobject_metadata l;"
    subprocess.check_call(['scripts/postgres_control.sh', 'start', reset_statement])
    if not dontKill:
        atexit.register(subprocess.call, ['scripts/postgres_control.sh', 'stop'])

    extraProducerAccounts = Cluster.createAccountKeys(1)
    vltproducerAccount = extraProducerAccounts[0]

    Print("Stand up cluster")
    if cluster.launch(onlyBios=False, pnodes=1, totalNodes=totalNodes,
                    useBiosBootFile=False, onlySetProds=False,
                    extraNodeosArgs=" --blocks-log-stride 20 --max-retained-block-files 3 --logconf %s" % loggingFile,
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

    Print("#################################################################################")
    Print("# Scenario 1: Test node 1 failover without snapshot in the block vault          #")
    Print("#################################################################################")
    testFailOver(cluster, nodeToKill=node1)
    
    Print("#################################################################################")
    Print("# Scenario 2: Test node 1 failover from the snapshot in the block vault         #")
    Print("#################################################################################")
    Print("Create a snapshot")
    node1.createSnapshot()
    Print("Wait until the snapshot appears in the database")
    Utils.waitForTruth(lambda: num_rows_in_table('SnapshotData') != 0)  
    testFailOver(cluster, nodeToKill=node1)

    Print("#################################################################################")
    Print("# Scenario 3: Test two identical producer nodes connecting to the block vault   #")
    Print("#################################################################################")
    node2 = cluster.getNode(2)
    time.sleep(10)
    testFailOver(cluster, nodeToKill=node2, addSwapFlags={
        "--plugin": "eosio::blockvault_client_plugin",
        "--block-vault-backend": "postgresql://postgres:password@localhost",
        "--producer-name": "vltproducera",
        "--signature-provider": "{}=KEY:{}".format(vltproducerAccount.ownerPublicKey, vltproducerAccount.ownerPrivateKey)
    })

    assert node2.waitForLibToAdvance(timeout=60)

    Print("#################################################################################")
    Print("# Scenario 4: Test one of the two identical producer node fails and the other   #")
    Print("#             can take over.                                                    #")
    Print("#################################################################################")
    Print("Kill bios, node0, node1")
    cluster.biosNode.kill(signal.SIGTERM)
    node0.kill(signal.SIGTERM)
    node1.kill(signal.SIGTERM)
    time.sleep(10)
    assert node2.waitForHeadToAdvance(timeout=60)

    Print("#################################################################################")
    Print("# Verify if there's any double production                                       #")
    Print("#################################################################################")
    double_produced_block_numbers = get_successful_constructed_block_numbers_for_node(1).intersection(get_successful_constructed_block_numbers_for_node(2))

    if len(double_produced_block_numbers) != 0:
      Utils.errorExit("Found double production for the following blocks {}".format(double_produced_block_numbers))

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exit(0)
