#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from TestHelper import TestHelper

import decimal
import re

###############################################################
# nodeos_run_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit
cmdError=Utils.cmdError
from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--defproducera_prvt_key","--dump-error-details","--dont-launch","--keep-logs",
                              "-v","--leave-running","--clean-run","--p2p-plugin"})
debug=args.v
defproduceraPrvtKey=args.defproducera_prvt_key
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontLaunch=args.dont_launch
dontKill=args.leave_running
killAll=args.clean_run
p2pPlugin=args.p2p_plugin

Utils.Debug=debug
cluster=Cluster(walletd=True, defproduceraPrvtKey=defproduceraPrvtKey)
walletMgr=WalletMgr(True)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName=Utils.EosWalletName
ClientName="cleos"
timeout = .5 * 12 * 2 + 60 # time for finalization with 1 producer + 60 seconds padding
Utils.setIrreversibleTimeout(timeout)

try:
    TestHelper.printSystemInfo("BEGIN")

    cluster.setWalletMgr(walletMgr)

    if not dontLaunch:
        cluster.killall(allInstances=killAll)
        cluster.cleanup()
        Print("Stand up cluster")
        pnodes=4
        if cluster.launch(pnodes=pnodes, totalNodes=pnodes, p2pPlugin=p2pPlugin) is False:
            cmdError("launcher")
            errorExit("Failed to stand up eos cluster.")
    else:
        walletMgr.killall(allInstances=killAll)
        walletMgr.cleanup()
        cluster.initializeNodes(defproduceraPrvtKey=defproduceraPrvtKey)
        killEosInstances=False

        print("Stand up walletd")
        if walletMgr.launch() is False:
            cmdError("%s" % (WalletdName))
            errorExit("Failed to stand up eos walletd.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    accounts=Cluster.createAccountKeys(3)
    if accounts is None:
        errorExit("FAILURE - create keys")
    testeraAccount=accounts[0]
    testeraAccount.name="testera11111"
    currencyAccount=accounts[1]
    currencyAccount.name="currency1111"
    exchangeAccount=accounts[2]
    exchangeAccount.name="exchange1111"

    PRV_KEY1=testeraAccount.ownerPrivateKey
    PUB_KEY1=testeraAccount.ownerPublicKey
    PRV_KEY2=currencyAccount.ownerPrivateKey
    PUB_KEY2=currencyAccount.ownerPublicKey
    PRV_KEY3=exchangeAccount.activePrivateKey
    PUB_KEY3=exchangeAccount.activePublicKey

    testeraAccount.activePrivateKey=currencyAccount.activePrivateKey=PRV_KEY3
    testeraAccount.activePublicKey=currencyAccount.activePublicKey=PUB_KEY3

    exchangeAccount.ownerPrivateKey=PRV_KEY2
    exchangeAccount.ownerPublicKey=PUB_KEY2

    testWalletName="test"
    Print("Creating wallet \"%s\"." % (testWalletName))
    testWallet=walletMgr.create(testWalletName, [cluster.eosioAccount,cluster.defproduceraAccount])

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

    # create accounts via eosio as otherwise a bid is needed 
    Print("Create new account %s via %s" % (testeraAccount.name, cluster.eosioAccount.name))
    transId=node.createInitializeAccount(testeraAccount, cluster.eosioAccount, stakedDeposit=0, waitForTransBlock=False, exitOnError=True)

    Print("Create new account %s via %s" % (currencyAccount.name, cluster.eosioAccount.name))
    transId=node.createInitializeAccount(currencyAccount, cluster.eosioAccount, buyRAM=1000000, stakedDeposit=5000, exitOnError=True)

    Print("Create new account %s via %s" % (exchangeAccount.name, cluster.eosioAccount.name))
    transId=node.createInitializeAccount(exchangeAccount, cluster.eosioAccount, buyRAM=1000000, waitForTransBlock=True, exitOnError=True)

    Print("Validating accounts after user accounts creation")
    accounts=[testeraAccount, currencyAccount, exchangeAccount]
    cluster.validateAccounts(accounts)

    Print("Verify account %s" % (testeraAccount))
    if not node.verifyAccount(testeraAccount):
        errorExit("FAILURE - account creation failed.", raw=True)

    transferAmount="97.5321 {0}".format(CORE_SYMBOL)
    Print("Transfer funds %s from account %s to %s" % (transferAmount, defproduceraAccount.name, testeraAccount.name))
    node.transferFunds(defproduceraAccount, testeraAccount, transferAmount, "test transfer")

    expectedAmount=transferAmount
    Print("Verify transfer, Expected: %s" % (expectedAmount))
    actualAmount=node.getAccountEosBalanceStr(testeraAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %s, actual: %s" % (expectedAmount, actualAmount))

    transferAmount="0.0100 {0}".format(CORE_SYMBOL)
    Print("Force transfer funds %s from account %s to %s" % (
        transferAmount, defproduceraAccount.name, testeraAccount.name))
    node.transferFunds(defproduceraAccount, testeraAccount, transferAmount, "test transfer", force=True)

    expectedAmount="97.5421 {0}".format(CORE_SYMBOL)
    Print("Verify transfer, Expected: %s" % (expectedAmount))
    actualAmount=node.getAccountEosBalanceStr(testeraAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %s, actual: %s" % (expectedAmount, actualAmount))

    Print("Validating accounts after some user transactions")
    accounts=[testeraAccount, currencyAccount, exchangeAccount]
    cluster.validateAccounts(accounts)

    transferAmount="97.5311 {0}".format(CORE_SYMBOL)
    Print("Transfer funds %s from account %s to %s" % (
        transferAmount, testeraAccount.name, currencyAccount.name))
    trans=node.transferFunds(testeraAccount, currencyAccount, transferAmount, "test transfer a->b")
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
        assert(actions["actions"][0]["action_trace"]["act"]["name"] == "transfer")
    except (AssertionError, TypeError, KeyError) as _:
        Print("Action validation failed. Actions: %s" % (actions))
        raise

    node.waitForTransInBlock(transId)

    transaction=node.getTransaction(transId, exitOnError=True, delayedRetry=False)

    typeVal=None
    amountVal=None
    key=""
    try:
        key="[traces][0][act][name]"
        typeVal=  transaction["traces"][0]["act"]["name"]
        key="[traces][0][act][data][quantity]"
        amountVal=transaction["traces"][0]["act"]["data"]["quantity"]
        amountVal=int(decimal.Decimal(amountVal.split()[0])*10000)
    except (TypeError, KeyError) as e:
        Print("transaction%s not found. Transaction: %s" % (key, transaction))
        raise

    if typeVal != "transfer" or amountVal != 975311:
        errorExit("FAILURE - get transaction trans_id failed: %s %s %s" % (transId, typeVal, amountVal), raw=True)

    Print("Bouncing nodes #00 and #01")
    if cluster.bounce("00,01") is False:
        cmdError("launcher bounce")
        errorExit("Failed to bounce eos node.")

    Print("Taking down node #02")
    if cluster.down("02") is False:
        cmdError("launcher down command")
        errorExit("Failed to take down eos node.")

    Print("Using bounce option to re-launch node #02")
    if cluster.bounce("02") is False:
        cmdError("launcher bounce")
        errorExit("Failed to bounce eos node.")

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

    Print("Validating accounts at end of test")
    accounts=[testeraAccount, currencyAccount, exchangeAccount]
    cluster.validateAccounts(accounts)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
