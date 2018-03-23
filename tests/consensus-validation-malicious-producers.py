#!/usr/bin/env python3

import testUtils

import argparse
import signal
from collections import namedtuple
import os
import shutil

###############################################################
# Test for validating consensus based block production. We introduce malicious producers which
#  reject all transactions.
# We have three test scenarios:
#  - No malicious producers. Transactions should be incorporated into the chain.
#  - Minority malicious producers (less than a third producer count). Transactions will get incorporated
# into the chain as majority appoves the transactions.
#  - Majority malicious producer count (greater than a third producer count). Transactions won't get
# incorporated into the chain as majority rejects the transactions.
###############################################################


Print=testUtils.Utils.Print

StagedNodeInfo=namedtuple("StagedNodeInfo", "config logging")


logging00="""{
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
    },{
      "name": "net",
      "type": "gelf",
      "args": {
        "endpoint": "10.160.11.21:12201",
        "host": "testnet_00"
      },
      "enabled": true
    }
  ],
  "loggers": [{
      "name": "default",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    }
  ]
}"""

config00="""genesis-json = ./genesis.json
block-log-dir = blocks
readonly = 0
send-whole-blocks = true
shared-file-dir = blockchain
shared-file-size = 8192
http-server-address = 127.0.0.1:8888
p2p-listen-endpoint = 0.0.0.0:9876
p2p-server-address = localhost:9876
allowed-connection = any
p2p-peer-address = localhost:9877
required-participation = true
private-key = ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]
producer-name = initu
plugin = eosio::producer_plugin
plugin = eosio::chain_api_plugin
plugin = eosio::account_history_plugin
plugin = eosio::account_history_api_plugin"""


config01="""genesis-json = ./genesis.json
block-log-dir = blocks
readonly = 0
send-whole-blocks = true
shared-file-dir = blockchain
shared-file-size = 8192
http-server-address = 127.0.0.1:8889
p2p-listen-endpoint = 0.0.0.0:9877
p2p-server-address = localhost:9877
allowed-connection = any
p2p-peer-address = localhost:9876
required-participation = true
private-key = ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]
producer-name = initb
plugin = eosio::producer_plugin
plugin = eosio::chain_api_plugin
plugin = eosio::account_history_plugin
plugin = eosio::account_history_api_plugin"""


producers="""producer-name = initd
producer-name = initf
producer-name = inith
producer-name = initj
producer-name = initl
producer-name = initn
producer-name = initp
producer-name = initr
producer-name = initt
producer-name = inita
producer-name = initc
producer-name = inite
producer-name = initg
producer-name = initi
producer-name = initk
producer-name = initm
producer-name = inito
producer-name = initq
producer-name = inits"""

zeroExecTime="trans-execution-time = 0"

def getNoMaliciousStagedNodesInfo():
    stagedNodesInfo=[]
    myConfig00=config00
    stagedNodesInfo.append(StagedNodeInfo(myConfig00, logging00))
    myConfig01=config01+"\n"+producers
    stagedNodesInfo.append(StagedNodeInfo(myConfig01, logging00))
    return stagedNodesInfo

def getMinorityMaliciousProducerStagedNodesInfo():
    stagedNodesInfo=[]
    myConfig00=config00+"\n"+producers
    stagedNodesInfo.append(StagedNodeInfo(myConfig00, logging00))
    myConfig01=config01+"\n"+zeroExecTime
    stagedNodesInfo.append(StagedNodeInfo(myConfig01, logging00))
    return stagedNodesInfo

def getMajorityMaliciousProducerStagedNodesInfo():
    stagedNodesInfo=[]
    myConfig00=config00
    stagedNodesInfo.append(StagedNodeInfo(myConfig00, logging00))
    myConfig01=config01+"\n"+producers+"\n"+zeroExecTime
    stagedNodesInfo.append(StagedNodeInfo(myConfig01, logging00))
    return stagedNodesInfo

stagingDir="staging"
def stageScenario(stagedNodeInfos):
    assert(stagedNodeInfos != None)
    assert(len(stagedNodeInfos) > 1)

    os.makedirs(stagingDir)
    count=0
    for stagedNodeInfo in stagedNodeInfos:
        configPath=os.path.join(stagingDir, "etc/eosio/node_%02d" % (count))
        os.makedirs(configPath)
        with open(os.path.join(configPath, "config.ini"), "w") as textFile:
            print(stagedNodeInfo.config,file=textFile)
        with open(os.path.join(configPath, "logging.json"), "w") as textFile:
            print(stagedNodeInfo.logging,file=textFile)
        count += 1
    return

def cleanStaging():
    os.path.exists(stagingDir) and shutil.rmtree(stagingDir)


def errorExit(msg="", errorCode=1):
    Print("ERROR:", msg)
    exit(errorCode)

def error(msg="", errorCode=1):
    Print("ERROR:", msg)

parser = argparse.ArgumentParser()
tests=[1,2,3]

parser.add_argument("-t", "--tests", type=str, help="1|2|3 1=run no malicious producers test, 2=minority malicious, 3=majority malicious.", default=None)
parser.add_argument("-w", type=int, help="system wait time", default=testUtils.Utils.systemWaitTimeout)
parser.add_argument("-v", help="verbose logging", action='store_true')
parser.add_argument("--dump-error-details",
                    help="Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout",
                    action='store_true')
parser.add_argument("--keep-logs", help="Don't delete var/lib/node_* folders upon test completion",
                    action='store_true')
parser.add_argument("--not-noon", help="This is not the Noon branch.", action='store_true')
parser.add_argument("--dont-kill", help="Leave cluster running after test finishes", action='store_true')

args = parser.parse_args()
testsArg=args.tests
debug=args.v
waitTimeout=args.w
dumpErrorDetails=args.dump-error-details
keepLogs=args.keep-logs
amINoon=not args.not_noon
killEosInstances= not args.dont-kill
killWallet= not args.dont-kill

testUtils.Utils.Debug=debug

assert (testsArg is None or testsArg == "1" or testsArg == "2" or testsArg == "3")
if testsArg is not None:
    tests=[int(testsArg)]

testUtils.Utils.setSystemWaitTimeout(waitTimeout)
testUtils.Utils.iAmNotNoon()

def myTest(transWillEnterBlock):
    testSuccessful=False

    cluster=testUtils.Cluster(walletd=True, staging=True)
    walletMgr=testUtils.WalletMgr(True)

    try:
        cluster.killall()
        cluster.cleanup()
        walletMgr.killall()
        walletMgr.cleanup()

        pnodes=2
        total_nodes=pnodes
        topo="mesh"
        delay=0
        Print("Stand up cluster")
        if cluster.launch(pnodes, total_nodes, topo, delay) is False:
            error("Failed to stand up eos cluster.")
            return False

        accounts=testUtils.Cluster.createAccountKeys(1)
        if accounts is None:
            error("FAILURE - create keys")
            return False
        currencyAccount=accounts[0]
        currencyAccount.name="currency"

        Print("Stand up walletd")
        if walletMgr.launch() is False:
            error("Failed to stand up eos walletd.")
            return False

        testWalletName="test"
        Print("Creating wallet \"%s\"." % (testWalletName))
        testWallet=walletMgr.create(testWalletName)
        if testWallet is None:
            error("Failed to create wallet %s." % (testWalletName))
            return False

        for account in accounts:
            Print("Importing keys for account %s into wallet %s." % (account.name, testWallet.name))
            if not walletMgr.importKey(account, testWallet):
                error("Failed to import key for account %s" % (account.name))
                return False

        node=cluster.getNode(0)
        node2=cluster.getNode(1)
        if node is None or node2 is None:
            error("Cluster in bad state, received None node")
            return False

        initaAccount=testUtils.Cluster.initaAccount

        Print("Importing keys for account %s into wallet %s." % (initaAccount.name, testWallet.name))
        if not walletMgr.importKey(initaAccount, testWallet):
            error("Failed to import key for account %s" % (initaAccount.name))
            return False

        Print("Create new account %s via %s" % (currencyAccount.name, initaAccount.name))
        transId=node.createAccount(currencyAccount, initaAccount, stakedDeposit=5000, waitForTransBlock=True)
        if transId is None:
            error("Failed to create account %s" % (currencyAccount.name))
            return False

        wastFile="contracts/currency/currency.wast"
        abiFile="contracts/currency/currency.abi"
        Print("Publish contract")
        trans=node.publishContract(currencyAccount.name, wastFile, abiFile, waitForTransBlock=True)
        if trans is None:
            error("Failed to publish contract.")
            return False

        Print("push transfer action to currency contract")
        contract="currency"
        action="transfer"
        data="{\"from\":\"currency\",\"to\":\"inita\",\"quantity\":"
        if amINoon:
            data +="\"00.0050 CUR\",\"memo\":\"test\"}"
        else:
            data +="50}"
        opts="--permission currency@active"
        if not amINoon:
            opts += " --scope currency,inita"

        trans=node.pushMessage(contract, action, data, opts, silentErrors=True)
        transInBlock=False
        if not trans[0]:
            # On slower systems e.g Travis the transaction rejection can happen immediately
            #  We want to handle fast and slow failures.
            if "allocated processing time was exceeded" in trans[1]:
                Print("Push message transaction immediately failed.")
            else:
                error("Exception in push message. %s" % (trans[1]))
                return False

        else:
            transId=testUtils.Node.getTransId(trans[1])

            Print("verify transaction exists")
            if not node2.waitForTransIdOnNode(transId):
                error("Transaction never made it to node2")
                return False

            Print("Get details for transaction %s" % (transId))
            transaction=node2.getTransaction(transId)
            signature=transaction["transaction"]["signatures"][0]

            blockNum=int(transaction["transaction"]["ref_block_num"])
            blockNum += 1
            Print("Our transaction is in block %d" % (blockNum))

            block=node2.getBlock(blockNum)
            cycles=block["cycles"]
            if len(cycles) > 0:
                blockTransSignature=cycles[0][0]["user_input"][0]["signatures"][0]
                # Print("Transaction signature: %s\nBlock transaction signature: %s" %
                #       (signature, blockTransSignature))
                transInBlock=(signature == blockTransSignature)

        if transWillEnterBlock:
            if not transInBlock:
                error("Transaction did not enter the chain.")
                return False
            else:
                Print("SUCCESS: Transaction1 entered in the chain.")
        elif not transWillEnterBlock:
            if transInBlock:
                error("Transaction entered the chain.")
                return False
            else:
                Print("SUCCESS: Transaction2 did not enter the chain.")

        testSuccessful=True
    finally:
        if not testSuccessful and dumpErrorDetails:
            cluster.dumpErrorDetails()
            walletMgr.dumpErrorDetails()
            Print("== Errors see above ==")

        if killEosInstances:
            Print("Shut down the cluster%s" % (" and cleanup." if (testSuccessful and not keepLogs) else "."))
            cluster.killall()
            walletMgr.killall()
            if testSuccessful and not keepLogs:
                Print("Cleanup cluster and wallet data.")
                cluster.cleanup()
                walletMgr.cleanup()

    return True


try:
    if 1 in tests:
        Print("Cluster with no malicious producers. All producers expected to approve transaction. Hence transaction is expected to enter the chain.")
        cleanStaging()
        stageScenario(getNoMaliciousStagedNodesInfo())
        if not myTest(True):
            exit(1)

    if 2 in tests:
        Print("\nCluster with minority(1) malicious nodes. Majority producers expected to approve transaction. Hence transaction is expected to enter the chain.")
        cleanStaging()
        stageScenario(getMinorityMaliciousProducerStagedNodesInfo())
        if not myTest(True):
            exit(1)

    if 3 in tests:
        Print("\nCluster with majority(20) malicious nodes. Majority producers expected to block transaction. Hence transaction is not expected to enter the chain.")
        cleanStaging()
        stageScenario(getMajorityMaliciousProducerStagedNodesInfo())
        if not myTest(False):
            exit(1)

finally:
    cleanStaging()

exit(0)
