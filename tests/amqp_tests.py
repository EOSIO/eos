#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper

import decimal
import re
import json
import os
import signal

###############################################################
# amqp_tests 
#
# tests for the amqp functionality 
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit
cmdError=Utils.cmdError
from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--host","--port"
                                 ,"--dump-error-details","--dont-launch","--keep-logs","-v","--leave-running","--clean-run"
                                 ,"--wallet-port","--amqp-address"})
server=args.host
port=args.port
debug=args.v
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontLaunch=args.dont_launch
dontKill=args.leave_running
killAll=args.clean_run
walletPort=args.wallet_port
amqpAddr=args.amqp_address

Utils.Debug=debug
localTest=True if server == TestHelper.LOCAL_HOST else False
cluster=Cluster(host=server, port=port, walletd=True)
walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName=Utils.EosWalletName
ClientName="cleos"

try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.setWalletMgr(walletMgr)
    Print("SERVER: %s" % (server))
    Print("PORT: %d" % (port))
    cluster.createAMQPQueue("trx")
    if localTest and not dontLaunch:
        cluster.killall(allInstances=killAll)
        cluster.cleanup()
        Print("Stand up cluster")

        amqProducerAccount = cluster.defProducerAccounts["eosio"]

        specificExtraNodeosArgs={ 0: " --backing-store=chainbase --plugin eosio::amqp_trx_plugin --amqp-trx-address %s" % (amqpAddr),
                                  1: " --backing-store=chainbase --plugin eosio::amqp_trx_plugin --amqp-trx-address %s" % (amqpAddr)}

        if cluster.launch(totalNodes=2, totalProducers=3, pnodes=2, dontBootstrap=False, onlyBios=False, useBiosBootFile=True, specificExtraNodeosArgs=specificExtraNodeosArgs) is False:
            cmdError("launcher")
            errorExit("Failed to stand up eos cluster.")
    else:
        Print("Collecting cluster info.")
        killEosInstances=False
        Print("Stand up %s" % (WalletdName))
        walletMgr.killall(allInstances=killAll)
        walletMgr.cleanup()
        print("Stand up walletd")
        if walletMgr.launch() is False:
            cmdError("%s" % (WalletdName))
            errorExit("Failed to stand up eos walletd.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    amqProducerAccount = cluster.defProducerAccounts["eosio"]
    backup_node = cluster.getNode(1)
    backup_node.kill(signal.SIGTERM)
    backup_node.relaunch(addSwapFlags={
        "--pause-on-startup": "",
        "--producer-name": "eosio",
        "--plugin": "eosio::producer_plugin",
        "--plugin": "eosio::producer_api_plugin",
        "--signature-provider": "{}=KEY:{}".format(amqProducerAccount.ownerPublicKey, amqProducerAccount.ownerPrivateKey)
    })

    accounts=Cluster.createAccountKeys(2)
    if accounts is None:
        errorExit("FAILURE - create keys")
    testeraAccount=accounts[0]
    testeraAccount.name="testera11111"
    currencyAccount=accounts[1]
    currencyAccount.name="currency1111"

    testWalletName="test"
    Print("Creating wallet \"%s\"." % (testWalletName))
    walletAccounts=[cluster.defproduceraAccount]
    if not dontLaunch:
        walletAccounts.append(cluster.eosioAccount)
    testWallet=walletMgr.create(testWalletName, walletAccounts)

    Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))

    for account in accounts:
        Print("Importing keys for account %s into wallet %s." % (account.name, testWallet.name))
        if not walletMgr.importKey(account, testWallet):
            cmdError("%s wallet import" % (ClientName))
            errorExit("Failed to import key for account %s" % (account.name))

    defproduceraWalletName="defproducera"
    Print("Creating wallet \"%s\"." % (defproduceraWalletName))
    defproduceraWallet=walletMgr.create(defproduceraWalletName)

    Print("Wallet \"%s\" password=%s." % (defproduceraWalletName, defproduceraWallet.password.encode("utf-8")))

    defproduceraAccount=cluster.defproduceraAccount

    Print("Importing keys for account %s into wallet %s." % (defproduceraAccount.name, defproduceraWallet.name))
    if not walletMgr.importKey(defproduceraAccount, defproduceraWallet):
        cmdError("%s wallet import" % (ClientName))
        errorExit("Failed to import key for account %s" % (defproduceraAccount.name))


    node=cluster.getNode(0)

    Print("Validating accounts before user accounts creation")
    cluster.validateAccounts(None)

    Print("Create new account %s via %s" % (testeraAccount.name, cluster.defproduceraAccount.name))
    transId=node.createInitializeAccount(testeraAccount, cluster.defproduceraAccount, stakedDeposit=0, waitForTransBlock=False, exitOnError=True)

    Print("Create new account %s via %s" % (currencyAccount.name, cluster.defproduceraAccount.name))
    transId=node.createInitializeAccount(currencyAccount, cluster.defproduceraAccount, buyRAM=200000, stakedDeposit=5000, exitOnError=True)

    Print("Validating accounts after user accounts creation")
    accounts=[testeraAccount, currencyAccount]
    cluster.validateAccounts(accounts)


    transferAmount="97.5321 {0}".format(CORE_SYMBOL)
    Print("Transfer funds %s from account %s to %s" % (transferAmount, defproduceraAccount.name, testeraAccount.name))
    node.transferFunds(defproduceraAccount, testeraAccount, transferAmount, "test transfer", waitForTransBlock=True)

    expectedAmount=transferAmount
    Print("Verify transfer, Expected: %s" % (expectedAmount))
    actualAmount=node.getAccountEosBalanceStr(testeraAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %s, actual: %s" % (expectedAmount, actualAmount))

    Print("**** Killing Main Producer Node & bios node ****")
    cluster.getNode(0).kill(signal.SIGTERM)
    cluster.discoverBiosNode().kill(signal.SIGTERM)

    node = cluster.getNode(1)
    if amqpAddr:
        node.setAMQPAddress(amqpAddr)

    Print("**** Transfer with no producer, waits on timeout ****")
    transferAmount="0.0100 {0}".format(CORE_SYMBOL)
    Print("Force transfer funds %s from account %s to %s" % (
        transferAmount, defproduceraAccount.name, testeraAccount.name))
    result = node.transferFunds(defproduceraAccount, testeraAccount, transferAmount, "test transfer", expiration=3600, waitForTransBlock=False, exitOnError=False)
    transId = node.getTransId(result)
    noTrans = node.getTransaction(transId, silentErrors=True)
    if noTrans is not None:
        cmdError("FAILURE - transfer should not have been executed yet")
        errorExit("result: %s" % (noTrans))

    Print("**** Resuming Backup Node ****")
    resumeResults = node.processCurlCmd(resource="producer", command="resume", payload="{}")
    Print(resumeResults)

    node.waitForHeadToAdvance()

    Print("**** Waiting for transaction ****")
    node.waitForTransInBlock(transId)

    expectedAmount="97.5421 {0}".format(CORE_SYMBOL)
    Print("Verify transfer, Expected: %s" % (expectedAmount))
    actualAmount=node.getAccountEosBalanceStr(testeraAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %s, actual: %s" % (expectedAmount, actualAmount))


    Print("**** Processed Transfer ****")


    transferAmount="97.5311 {0}".format(CORE_SYMBOL)
    Print("Transfer funds %s from account %s to %s" % (
        transferAmount, testeraAccount.name, currencyAccount.name))
    trans=node.transferFunds(testeraAccount, currencyAccount, transferAmount, "test transfer a->b", waitForTransBlock=True)
    transId=Node.getTransId(trans)

    expectedAmount="98.0311 {0}".format(CORE_SYMBOL) # 5000 initial deposit
    Print("Verify transfer, Expected: %s" % (expectedAmount))
    actualAmount=node.getAccountEosBalanceStr(currencyAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %s, actual: %s" % (expectedAmount, actualAmount))

    Print("Validate last action for account %s" % (testeraAccount.name))
    actions=node.getActions(testeraAccount, -1, -1, exitOnError=True)
    try:
        assert (actions["actions"][0]["action_trace"]["act"]["name"] == "transfer")
    except (AssertionError, TypeError, KeyError) as _:
        Print("Action validation failed. Actions: %s" % (actions))
        raise

    node.waitForTransInBlock(transId)

    transaction=node.getTransaction(transId, exitOnError=True, delayedRetry=False)

    typeVal=None
    amountVal=None
    key=""
    try:
        key = "[traces][0][act][name]"
        typeVal = transaction["traces"][0]["act"]["name"]
        key = "[traces][0][act][data][quantity]"
        amountVal = transaction["traces"][0]["act"]["data"]["quantity"]
        amountVal = int(decimal.Decimal(amountVal.split()[0]) * 10000)
    except (TypeError, KeyError) as e:
        Print("transaction%s not found. Transaction: %s" % (key, transaction))
        raise

    if typeVal != "transfer" or amountVal != 975311:
        errorExit("FAILURE - get transaction trans_id failed: %s %s %s" % (transId, typeVal, amountVal), raw=True)


    if localTest:
        p = re.compile('Assert')
        errFileName="var/lib/node_00/stderr.txt"
        assertionsFound=False
        with open(errFileName) as errFile:
            for line in errFile:
                if p.search(line):
                    assertionsFound=True

        if assertionsFound:
            # Too many assertion logs, hard to validate how many are genuine. Make this a warning
            #  for now, hopefully the logs will get cleaned up in future.
            Print("WARNING: Asserts in var/lib/node_00/stderr.txt")
            #errorExit("FAILURE - Assert in var/lib/node_00/stderr.txt")

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
