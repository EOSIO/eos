#!/usr/bin/env python3

from testUtils import Utils
import signal
import time
from Cluster import Cluster
from Cluster import NamedAccounts
from core_symbol import CORE_SYMBOL
from WalletMgr import WalletMgr
from Node import Node
from TestHelper import TestHelper
from TestHelper import AppArgs

import json

###############################################################
# nodeos_high_transaction_test
# 
# This test sets up <-p> producing node(s) and <-n - -p>
#   non-producing node(s). The non-producing node will be sent
#   many transfers.  When it is complete it verifies that all
#   of the transactions made it into blocks.
#
###############################################################

Print=Utils.Print

appArgs = AppArgs()
minTotalAccounts = 20
extraArgs = appArgs.add(flag="--transaction-time-delta", type=int, help="How many seconds seconds behind an earlier sent transaction should be received after a later one", default=5)
extraArgs = appArgs.add(flag="--num-transactions", type=int, help="How many total transactions should be sent", default=10000)
extraArgs = appArgs.add(flag="--max-transactions-per-second", type=int, help="How many transactions per second should be sent", default=500)
extraArgs = appArgs.add(flag="--total-accounts", type=int, help="How many accounts should be involved in sending transfers.  Must be greater than %d" % (minTotalAccounts), default=100)
extraArgs = appArgs.add_bool(flag="--send-duplicates", help="If identical transactions should be sent to all nodes")
args = TestHelper.parse_args({"-p", "-n","--dump-error-details","--keep-logs","-v","--leave-running","--clean-run","--amqp-address"}, applicationSpecificArgs=appArgs)

Utils.Debug=args.v
totalProducerNodes=args.p
totalNodes=args.n
if totalNodes<=totalProducerNodes:
    totalNodes=totalProducerNodes+1
totalNonProducerNodes=totalNodes-totalProducerNodes
maxActiveProducers=totalProducerNodes
totalProducers=totalProducerNodes
amqpAddr=args.amqp_address
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
killAll=args.clean_run
walletPort=TestHelper.DEFAULT_WALLET_PORT
blocksPerSec=2
transBlocksBehind=args.transaction_time_delta * blocksPerSec
numTransactions = args.num_transactions
maxTransactionsPerSecond = args.max_transactions_per_second
assert args.total_accounts >= minTotalAccounts, Print("ERROR: Only %d was selected for --total-accounts, must have at least %d" % (args.total_accounts, minTotalAccounts))
if numTransactions % args.total_accounts > 0:
    oldNumTransactions = numTransactions
    numTransactions = int((oldNumTransactions + args.total_accounts - 1)/args.total_accounts) * args.total_accounts
    Print("NOTE: --num-transactions passed as %d, but rounding to %d so each of the %d accounts gets the same number of transactions" %
          (oldNumTransactions, numTransactions, args.total_accounts))
numRounds = int(numTransactions / args.total_accounts)


walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName=Utils.EosWalletName
ClientName="cleos"

maxTransactionAttempts = 2            # max number of attempts to try to send a transaction
maxTransactionAttemptsNoSend = 1      # max number of attempts to try to create a transaction to be sent as a duplicate

try:
    TestHelper.printSystemInfo("BEGIN")

    cluster.setWalletMgr(walletMgr)
    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    Print("Stand up cluster")

    if not amqpAddr:
        launched = cluster.launch(pnodes=totalProducerNodes,
                                  totalNodes=totalNodes, totalProducers=totalProducers,
                                  useBiosBootFile=False, topo="ring")
    else:
        launched = cluster.launch(pnodes=totalProducerNodes,
                                  totalNodes=totalNodes, totalProducers=totalProducers,
                                  useBiosBootFile=False, topo="ring",
                                  extraNodeosArgs=" --plugin eosio::amqp_trx_plugin --amqp-trx-address %s --amqp-trx-speculative-execution --plugin eosio::amqp_trace_plugin --amqp-trace-address %s" % (amqpAddr, amqpAddr))
    if launched is False:
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")

    # ***   create accounts to vote in desired producers   ***

    Print("creating %d accounts" % (args.total_accounts))
    namedAccounts=NamedAccounts(cluster,args.total_accounts)
    accounts=namedAccounts.accounts

    accountsToCreate = [cluster.eosioAccount]
    for account in accounts:
        accountsToCreate.append(account)

    testWalletName="test"

    Print("Creating wallet \"%s\"." % (testWalletName))
    testWallet=walletMgr.create(testWalletName, accountsToCreate)

    for _, account in cluster.defProducerAccounts.items():
        walletMgr.importKey(account, testWallet, ignoreDupKeyWarning=True)

    Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))

    for account in accounts:
        walletMgr.importKey(account, testWallet)

    # ***   identify each node (producers and non-producing node)   ***

    nonProdNodes=[]
    prodNodes=[]
    allNodes=cluster.getNodes()
    for i in range(0, totalNodes):
        node=allNodes[i]
        nodeProducers=Cluster.parseProducers(i)
        numProducers=len(nodeProducers)
        Print("node has producers=%s" % (nodeProducers))
        if numProducers==0:
            nonProdNodes.append(node)
        else:
            prodNodes.append(node)
    nonProdNodeCount = len(nonProdNodes)

    # ***   delegate bandwidth to accounts   ***

    node=nonProdNodes[0]
    checkTransIds = []
    startTime = time.perf_counter()
    Print("Create new accounts via %s" % (cluster.eosioAccount.name))
    # create accounts via eosio as otherwise a bid is needed
    for account in accounts:
        trans = node.createInitializeAccount(account, cluster.eosioAccount, stakedDeposit=0, waitForTransBlock=False, stakeNet=1000, stakeCPU=1000, buyRAM=1000, exitOnError=True)
        checkTransIds.append(Node.getTransId(trans))

    nextTime = time.perf_counter()
    Print("Create new accounts took %s sec" % (nextTime - startTime))
    startTime = nextTime

    Print("Transfer funds to new accounts via %s" % (cluster.eosioAccount.name))
    for account in accounts:
        transferAmount="1000.0000 {0}".format(CORE_SYMBOL)
        Print("Transfer funds %s from account %s to %s" % (transferAmount, cluster.eosioAccount.name, account.name))
        trans = node.transferFunds(cluster.eosioAccount, account, transferAmount, "test transfer", waitForTransBlock=False, reportStatus=False, sign = True)
        checkTransIds.append(Node.getTransId(trans))

    nextTime = time.perf_counter()
    Print("Transfer funds took %s sec" % (nextTime - startTime))
    startTime = nextTime

    Print("Delegate Bandwidth to new accounts")
    for account in accounts:
        trans=node.delegatebw(account, 200.0000, 200.0000, waitForTransBlock=False, exitOnError=True, reportStatus=False)
        checkTransIds.append(Node.getTransId(trans))

    nextTime = time.perf_counter()
    Print("Delegate Bandwidth took %s sec" % (nextTime - startTime))
    startTime = nextTime
    lastIrreversibleBlockNum = None

    def cacheTransIdInBlock(transId, transToBlock, node):
        global lastIrreversibleBlockNum
        lastPassLIB = None
        blockWaitTimeout = 60
        transTimeDelayed = False
        while True:
            trans = node.getTransaction(transId, delayedRetry=False)
            if trans is None:
                if transTimeDelayed:
                    return (None, None)
                else:
                    if Utils.Debug:
                        Print("Transaction not found for trans id: %s. Will wait %d seconds to see if it arrives in a block." %
                              (transId, args.transaction_time_delta))
                    transTimeDelayed = True
                    node.waitForTransInBlock(transId, timeout = args.transaction_time_delta)
                    continue

            lastIrreversibleBlockNum = trans["last_irreversible_block"]
            blockNum = Node.getTransBlockNum(trans)
            assert blockNum is not None, Print("ERROR: could not retrieve block num from transId: %s, trans: %s" % (transId, json.dumps(trans, indent=2)))
            block = node.getBlock(blockNum)
            if block is not None:
                transactions = block["transactions"]
                for trans_receipt in transactions:
                    btrans = trans_receipt["trx"]
                    assert btrans is not None, Print("ERROR: could not retrieve \"trx\" from transaction_receipt: %s, from transId: %s that led to block: %s" % (json.dumps(trans_receipt, indent=2), transId, json.dumps(block, indent=2)))
                    btransId = btrans["id"]
                    assert btransId is not None, Print("ERROR: could not retrieve \"id\" from \"trx\": %s, from transId: %s that led to block: %s" % (json.dumps(btrans, indent=2), transId, json.dumps(block, indent=2)))
                    assert btransId not in transToBlock, Print("ERROR: transaction_id: %s found in block: %d, but originally seen in block number: %d" % (btransId, blockNum, transToBlock[btransId]["block_num"]))
                    transToBlock[btransId] = block

                break

            if lastPassLIB is not None and lastPassLIB >= lastIrreversibleBlockNum:
                Print("ERROR: could not find block number: %d from transId: %s, waited %d seconds for LIB to advance but it did not. trans: %s" % (blockNum, transId, blockWaitTimeout, json.dumps(trans, indent=2)))
                return (trans, None)
            if Utils.Debug:
                extra = "" if lastPassLIB is None else " and since it progressed from %d in our last pass" % (lastPassLIB)
                Print("Transaction returned for trans id: %s indicated it was in block num: %d, but that block could not be found.  LIB is %d%s, we will wait to see if the block arrives." %
                      (transId, blockNum, lastIrreversibleBlockNum, extra))
            lastPassLIB = lastIrreversibleBlockNum
            node.waitForBlock(blockNum, timeout = blockWaitTimeout)

        return (block, trans)

    def findTransInBlock(transId, transToBlock, node):
        if transId in transToBlock:
            return
        (block, trans) = cacheTransIdInBlock(transId, transToBlock, node)
        assert trans is not None, Print("ERROR: could not find transaction for transId: %s" % (transId))
        assert block is not None, Print("ERROR: could not retrieve block with block num: %d, from transId: %s, trans: %s" % (blockNum, transId, json.dumps(trans, indent=2)))

    transToBlock = {}
    for transId in checkTransIds:
        findTransInBlock(transId, transToBlock, node)

    nextTime = time.perf_counter()
    Print("Verifying transactions took %s sec" % (nextTime - startTime))
    startTransferTime = nextTime

    #verify nodes are in sync and advancing
    cluster.waitOnClusterSync(blockAdvancing=5)

    nodeOrder = []
    if args.send_duplicates:
        # kill bios, since it prevents the ring topography from really being a ring
        cluster.biosNode.kill(signal.SIGTERM)
        nodeOrder.append(0)
        # jump to node furthest in ring from node 0
        next = int((totalNodes + 1) / 2)
        nodeOrder.append(next)
        # then just fill in the rest of the nodes
        for i in range(1, next):
            nodeOrder.append(i)
        for i in range(next + 1, totalNodes):
            nodeOrder.append(i)

    Print("Sending %d transfers" % (numTransactions))
    delayAfterRounds = int(maxTransactionsPerSecond / args.total_accounts)
    history = []
    startTime = time.perf_counter()
    startRound = None
    for round in range(0, numRounds):
        # ensure we are not sending too fast
        startRound = time.perf_counter()
        timeDiff = startRound - startTime
        expectedTransactions = maxTransactionsPerSecond * timeDiff
        sentTransactions = round * args.total_accounts
        if sentTransactions > expectedTransactions:
            excess = sentTransactions - expectedTransactions
            # round up to a second
            delayTime = int((excess + maxTransactionsPerSecond - 1) / maxTransactionsPerSecond)
            Utils.Print("Delay %d seconds to keep max transactions under %d per second" % (delayTime, maxTransactionsPerSecond))
            time.sleep(delayTime)

        transferAmount = Node.currencyIntToStr(round + 1, CORE_SYMBOL)

        Print("Sending round %d, transfer: %s" % (round, transferAmount))
        for accountIndex in range(0, args.total_accounts):
            fromAccount = accounts[accountIndex]
            toAccountIndex = accountIndex + 1 if accountIndex + 1 < args.total_accounts else 0
            toAccount = accounts[toAccountIndex]
            node = nonProdNodes[accountIndex % nonProdNodeCount]
            if amqpAddr:
                node.setAMQPAddress(amqpAddr)

            trans = None
            attempts = 0
            maxAttempts = maxTransactionAttempts if not args.send_duplicates else maxTransactionAttemptsNoSend  # for send_duplicates we are just constructing a transaction, so should never require a second attempt
            # can try up to maxAttempts times to send the transfer
            while trans is None and attempts < maxAttempts:
                if attempts > 0:
                    # delay and see if transfer is accepted now
                    Utils.Print("Transfer rejected, delay 1 second and see if it is then accepted")
                    time.sleep(1)
                trans=node.transferFunds(fromAccount, toAccount, transferAmount, "transfer round %d" % (round), exitOnError=False, reportStatus=False, sign = True, dontSend = args.send_duplicates)
                attempts += 1

            if args.send_duplicates:
                sendTrans = trans
                trans = None
                numAccepted = 0
                attempts = 0
                while trans is None and attempts < maxTransactionAttempts:
                    for node in map(lambda ordinal: allNodes[ordinal], nodeOrder):
                        repeatTrans = node.pushTransaction(sendTrans, silentErrors=True)
                        if repeatTrans is not None:
                            if trans is None and repeatTrans[0]:
                                trans = repeatTrans[1]
                                transId = Node.getTransId(trans)

                            numAccepted += 1

                    attempts += 1

            assert trans is not None, Print("ERROR: failed round: %d, fromAccount: %s, toAccount: %s" % (round, accountIndex, toAccountIndex))
            transId = Node.getTransId(trans)
            history.append(transId)

    nextTime = time.perf_counter()
    Print("Sending transfers took %s sec" % (nextTime - startTransferTime))
    startTranferValidationTime = nextTime

    blocks = {}
    transToBlock = {}
    missingTransactions = []
    transBlockOrderWeird = []
    newestBlockNum = None
    newestBlockNumTransId = None
    newestBlockNumTransOrder = None
    lastBlockNum = None
    lastTransId = None
    transOrder = 0
    lastPassLIB = None
    for transId in history:
        blockNum = None
        block = None
        transDesc = None
        if transId not in transToBlock:
            (block, trans) = cacheTransIdInBlock(transId, transToBlock, node)
            if trans is None or block is None:
                missingTransactions.append({
                    "newer_trans_id" : transId,
                    "newer_trans_index" : transOrder,
                    "newer_bnum" : None,
                    "last_trans_id" : lastTransId,
                    "last_trans_index" : transOrder - 1,
                    "last_bnum" : lastBlockNum,
                })
                if newestBlockNum > lastBlockNum:
                    missingTransactions[-1]["highest_block_seen"] = newestBlockNum
            blockNum = Node.getTransBlockNum(trans)
            assert blockNum is not None, Print("ERROR: could not retrieve block num from transId: %s, trans: %s" % (transId, json.dumps(trans, indent=2)))
        else:
            block = transToBlock[transId]
            blockNum = block["block_num"]
            assert blockNum is not None, Print("ERROR: could not retrieve block num for block retrieved for transId: %s, block: %s" % (transId, json.dumps(block, indent=2)))

        if lastBlockNum is not None:
            if blockNum > lastBlockNum + transBlocksBehind or blockNum + transBlocksBehind < lastBlockNum:
                transBlockOrderWeird.append({
                    "newer_trans_id" : transId,
                    "newer_trans_index" : transOrder,
                    "newer_bnum" : blockNum,
                    "last_trans_id" : lastTransId,
                    "last_trans_index" : transOrder - 1,
                    "last_bnum" : lastBlockNum
                })
                if newestBlockNum > lastBlockNum:
                    last = transBlockOrderWeird[-1]
                    last["older_trans_id"] = newestBlockNumTransId
                    last["older_trans_index"] = newestBlockNumTransOrder
                    last["older_bnum"] = newestBlockNum

        if newestBlockNum is None:
            newestBlockNum = blockNum
            newestBlockNumTransId = transId
            newestBlockNumTransOrder = transOrder
        elif blockNum > newestBlockNum:
            newestBlockNum = blockNum
            newestBlockNumTransId = transId
            newestBlockNumTransOrder = transOrder

        lastTransId = transId
        transOrder += 1
        lastBlockNum = blockNum

    nextTime = time.perf_counter()
    Print("Validating transfers took %s sec" % (nextTime - startTranferValidationTime))

    delayedReportError = False
    if len(missingTransactions) > 0:
        verboseOutput = "Missing transaction information: [" if Utils.Debug else "Missing transaction ids: ["
        verboseOutput = ", ".join([missingTrans if Utils.Debug else missingTrans["newer_trans_id"] for missingTrans in missingTransactions])
        verboseOutput += "]"
        Utils.Print("ERROR: There are %d missing transactions.  %s" % (len(missingTransactions), verboseOutput))
        delayedReportError = True

    if len(transBlockOrderWeird) > 0:
        verboseOutput = "Delayed transaction information: [" if Utils.Debug else "Delayed transaction ids: ["
        verboseOutput = ", ".join([json.dumps(trans, indent=2) if Utils.Debug else trans["newer_trans_id"] for trans in transBlockOrderWeird])
        verboseOutput += "]"
        Utils.Print("ERROR: There are %d transactions delayed more than %d seconds.  %s" % (len(transBlockOrderWeird), args.transaction_time_delta, verboseOutput))
        delayedReportError = True

    testSuccessful = not delayedReportError
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)
    if not testSuccessful:
        Print(Utils.FileDivider)
        Print("Compare Blocklog")
        cluster.compareBlockLogs()
        Print(Utils.FileDivider)
        Print("Print Blocklog")
        cluster.printBlockLog()
        Print(Utils.FileDivider)

errorCode = 0 if testSuccessful else 1
exit(errorCode)
