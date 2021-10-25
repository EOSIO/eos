#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from Node import Node
from WalletMgr import WalletMgr
from TestHelper import TestHelper
from core_symbol import CORE_SYMBOL

import argparse
import signal


###############################################################
# Test for validating consensus based block production. We introduce malicious producers which
#  reject all transactions.
# We have three test scenarios:
#  - No malicious producers. Transactions should be incorporated into the chain.#
#  - Minority malicious producers (less than a third producer count). Transactions will get incorporated
#    into the chain as majority appoves the transactions.
#  - Majority malicious producer count (greater than a third producer count). Transactions won't get
# incorporated into the chain as majority rejects the transactions.
###############################################################


Print = Utils.Print
cmdError = Utils.cmdError
errorExit = Utils.errorExit
ClientName = "cleos"

parser = argparse.ArgumentParser()
tests = [1, 2, 3]

parser.add_argument("-t", "--tests", type=str, help="1|2|3 1=run no malicious producers test, 2=minority malicious, 3=majority malicious.", default=None)
parser.add_argument("-w", type=int, help="system wait time", default=Utils.systemWaitTimeout)
parser.add_argument("-v", help="verbose logging", action='store_true')
parser.add_argument("--dump-error-details",
                    help="Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout",
                    action='store_true')
parser.add_argument("--keep-logs", help="Don't delete var/lib/node_* folders upon test completion",
                    action='store_true')
parser.add_argument("--not-noon", help="This is not the Noon branch.", action='store_true')
parser.add_argument("--dont-kill", help="Leave cluster running after test finishes", action='store_true')
parser.add_argument("--clean-run", help="Kill all nodeos and kleos instances", action='store_true')

args = parser.parse_args()

testsArg = args.tests
debug = args.v
waitTimeout = args.w
dumpErrorDetails = args.dump_error_details
keepLogs = args.keep_logs
amINoon = not args.not_noon
killEosInstances = not args.dont_kill
killWallet = not args.dont_kill
killAll = args.clean_run

Utils.Debug = debug

WalletdName = Utils.EosWalletName

assert (testsArg is None or testsArg == "1" or testsArg == "2" or testsArg == "3")
if testsArg is not None:
    tests = [int(testsArg)]

Utils.setSystemWaitTimeout(waitTimeout)

def myTest(testScenario):
    testSuccessful = False

    cluster = Cluster(walletd=True)
    walletMgr = WalletMgr(True)
    cluster.setWalletMgr(walletMgr)

    try:
        cluster.killall(allInstances=True)
        cluster.cleanup()
        walletMgr.killall(allInstances=True)
        walletMgr.cleanup()

        pnodes = 2
        total_nodes = pnodes
        topo = "mesh"
        delay = 0
        Print("Stand up cluster")

        traceNodeosArgs = " --plugin eosio::trace_api_plugin --trace-no-abis "
        #if cluster.launch(prodCount=1, onlyBios=False, pnodes=pnodes, totalNodes=pnodes, totalProducers=pnodes*21, useBiosBootFile=False, extraNodeosArgs=traceNodeosArgs) is False:
        if cluster.launch(prodCount=1, onlyBios=False, pnodes=total_nodes, totalNodes=total_nodes, totalProducers=total_nodes, useBiosBootFile=False, topo=topo, delay=delay, extraNodeosArgs=traceNodeosArgs) is False:
            cmdError("launcher")
            errorExit("Failed to stand up eos cluster.")

        Print("Validating system accounts after bootstrap")
        cluster.validateAccounts(None)

        accounts = Cluster.createAccountKeys(2)
        if accounts is None:
            errorExit("FAILURE - create keys")

        accounts[0].name="tester111111"
        accounts[1].name="tester222222"

        testWalletName="test"

        Print("Creating wallet \"%s\"." % (testWalletName))
        testWallet = walletMgr.create(testWalletName, [cluster.eosioAccount,accounts[0], accounts[1]])

        for _, account in cluster.defProducerAccounts.items():
            walletMgr.importKey(account, testWallet, ignoreDupKeyWarning=True)

        Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))

        for i in range(0, total_nodes):
            node = cluster.getNode(i)
            node.producers = node.getProducers()
            for prod in node.producers:
                trans = node.regproducer(cluster.defProducerAccounts[prod], "http::/mysite.com", 0, waitForTransBlock=False, exitOnError=True)

        node0 = cluster.getNode(0)
        node1 = cluster.getNode(1)

        node = node0
        # create accounts via eosio as otherwise a bid is needed
        for account in accounts:
            Print("Create new account %s via %s" % (account.name, cluster.eosioAccount.name))
            trans = node.createInitializeAccount(account, cluster.eosioAccount, stakedDeposit=0, waitForTransBlock=False, stakeNet=1000, stakeCPU=1000, buyRAM=1000, exitOnError=True)
            transferAmount = "100000000.0000 {0}".format(CORE_SYMBOL)
            Print("Transfer funds %s from account %s to %s" % (transferAmount, cluster.eosioAccount.name, account.name))
            node.transferFunds(cluster.eosioAccount, account, transferAmount, "test transfer")
            trans = node.delegatebw(account, 20000000.0000, 20000000.0000, waitForTransBlock=True, exitOnError=True)

        #first account will vote for node0 producers, the other will vote for node1 producers
        node = node0
        for account in accounts:
            trans = node.vote(account, node.producers, waitForTransBlock=True)
            node = node1

        currencyAccount=accounts[0]

        contractDir = "unittests/contracts/eosio.token"
        wasmFile = "eosio.token.wasm"
        abiFile = "eosio.token.abi"
        Print("Publish contract")
        trans = node0.publishContract(currencyAccount, contractDir, wasmFile, abiFile, waitForTransBlock=True)
        if trans is None:
            errorExit("Failed to publish contract.")

        #
        Print("push create action to tester111111 contract")
        contract = "tester111111"
        action = "create"
        data = "{\"issuer\":\"tester111111\",\"maximum_supply\":\"100000.0000 SYS\",\"can_freeze\":\"0\",\"can_recall\":\"0\",\"can_whitelist\":\"0\"}"
        opts = "--permission tester111111@active"
        trans = node0.pushMessage(contract, action, data, opts)
        try:
            assert(trans)
            assert(trans[0])
        except (AssertionError, KeyError) as _:
            Print("ERROR: Failed push create action to tester111111 contract assertion. %s" % (trans))
            raise

        Print("push issue action to tester111111 contract")
        action = "issue"
        data = "{\"to\":\"tester111111\",\"quantity\":\"100000.0000 SYS\",\"memo\":\"issue\"}"
        opts = "--permission tester111111@active"
        trans = node0.pushMessage(contract, action, data, opts)
        try:
            assert(trans)
            assert(trans[0])
        except (AssertionError, KeyError) as _:
            Print("ERROR: Failed push issue action to tester111111 contract assertion. %s" % (trans))
            raise

        if testScenario == 2:
            # make node1 reject transactions
            node1.kill(signal.SIGTERM)
            node1.relaunch(addSwapFlags={
                "--max-transaction-time": "0",
            })

        Print("push transfer action to tester111111 contract")
        contract = "tester111111"
        action = "transfer"
        #data = "{\"from\":\"tester111111\",\"to\":\"defproducera\",\"quantity\":"
        data = "{\"from\":\"tester111111\",\"to\":\"tester222222\",\"quantity\":"
        data += "\"50.0000 SYS\",\"memo\":\"test\"}"
        opts = "--permission tester111111@active"
        trans = node0.pushMessage(contract, action, data, opts, silentErrors=True)

        transInBlock = False
        if not trans[0]:
            errorExit("Exception in push message. %s" % (trans[1]))
        else:
            transId = Node.getTransId(trans[1])

            Print("verify transaction exists")
            if not node1.waitForTransInBlock(transId):
                errorExit("Transaction never made it to node1")

            Print("Get details for transaction %s" % (transId))
            transaction = node1.getTransaction(transId, exitOnError=True)
            signature = transaction["signatures"][0]

            blockNum = int(transaction["block_num"])
            Print("Our transaction is in block %d" % (blockNum))

            block = node1.getBlock(blockNum, exitOnError=True)
            # cycles=block["cycles"]
            # if len(cycles) > 0:
            #     blockTransSignature=cycles[0][0]["user_input"][0]["signatures"][0]
            #     # Print("Transaction signature: %s\nBlock transaction signature: %s" %
            #     #       (signature, blockTransSignature))
            #     transInBlock=(signature == blockTransSignature)
            transactions = block["transactions"]
            blockTransSignature = transactions[0]["trx"]["signatures"][0]
            transInBlock = (signature == blockTransSignature)

        if testScenario == 0 or testScenario == 1:
            if not transInBlock:
                errorExit("Transaction did not enter the chain.")
            else:
                Print("SUCCESS: Transaction1 entered in the chain.")
        else:
            if transInBlock:
                errorExit("Transaction entered the chain.")
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


if 1 in tests:
    Print("Cluster with no malicious producers. All producers expected to approve transaction. Hence transaction is expected to enter the chain.")
    if not myTest(0):
        exit(1)

if 2 in tests:
    Print("\nCluster with minority(1) malicious nodes. Majority producers expected to approve transaction. Hence transaction is expected to enter the chain.")
    if not myTest(1):
        exit(1)

# if 3 in tests:
#     Print("\nCluster with majority(20) malicious nodes. Majority producers expected to block transaction. Hence transaction is not expected to enter the chain.")
#     if not myTest(2):
#         exit(1)

exit(0)
