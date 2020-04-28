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

###############################################################
# nodeos_run_test
#
# General test that tests a wide range of general use actions around nodeos and keosd
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit
cmdError=Utils.cmdError
from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--host","--port","--prod-count","--defproducera_prvt_key","--defproducerb_prvt_key","--mongodb"
                              ,"--dump-error-details","--dont-launch","--keep-logs","-v","--leave-running","--only-bios","--clean-run"
                              ,"--sanity-test","--wallet-port"})
server=args.host
port=args.port
debug=args.v
enableMongo=args.mongodb
defproduceraPrvtKey=args.defproducera_prvt_key
defproducerbPrvtKey=args.defproducerb_prvt_key
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontLaunch=args.dont_launch
dontKill=args.leave_running
prodCount=args.prod_count
onlyBios=args.only_bios
killAll=args.clean_run
sanityTest=args.sanity_test
walletPort=args.wallet_port

Utils.Debug=debug
localTest=True if server == TestHelper.LOCAL_HOST else False
cluster=Cluster(host=server, port=port, walletd=True, enableMongo=enableMongo, defproduceraPrvtKey=defproduceraPrvtKey, defproducerbPrvtKey=defproducerbPrvtKey)
walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill
dontBootstrap=sanityTest # intent is to limit the scope of the sanity test to just verifying that nodes can be started

WalletdName=Utils.EosWalletName
ClientName="cleos"
timeout = .5 * 12 * 2 + 60 # time for finalization with 1 producer + 60 seconds padding
Utils.setIrreversibleTimeout(timeout)

try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.setWalletMgr(walletMgr)
    Print("SERVER: %s" % (server))
    Print("PORT: %d" % (port))

    if enableMongo and not cluster.isMongodDbRunning():
        errorExit("MongoDb doesn't seem to be running.")

    if localTest and not dontLaunch:
        cluster.killall(allInstances=killAll)
        cluster.cleanup()
        Print("Stand up cluster")
        if cluster.launch(prodCount=prodCount, onlyBios=onlyBios, dontBootstrap=dontBootstrap) is False:
            cmdError("launcher")
            errorExit("Failed to stand up eos cluster.")
    else:
        Print("Collecting cluster info.")
        cluster.initializeNodes(defproduceraPrvtKey=defproduceraPrvtKey, defproducerbPrvtKey=defproducerbPrvtKey)
        killEosInstances=False
        Print("Stand up %s" % (WalletdName))
        walletMgr.killall(allInstances=killAll)
        walletMgr.cleanup()
        print("Stand up walletd")
        if walletMgr.launch() is False:
            cmdError("%s" % (WalletdName))
            errorExit("Failed to stand up eos walletd.")

    if sanityTest:
        testSuccessful=True
        exit(0)

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
    walletAccounts=[cluster.defproduceraAccount,cluster.defproducerbAccount]
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
    defproducerbAccount=cluster.defproducerbAccount

    Print("Importing keys for account %s into wallet %s." % (defproduceraAccount.name, defproduceraWallet.name))
    if not walletMgr.importKey(defproduceraAccount, defproduceraWallet):
        cmdError("%s wallet import" % (ClientName))
        errorExit("Failed to import key for account %s" % (defproduceraAccount.name))

    Print("Locking wallet \"%s\"." % (testWallet.name))
    if not walletMgr.lockWallet(testWallet):
        cmdError("%s wallet lock" % (ClientName))
        errorExit("Failed to lock wallet %s" % (testWallet.name))

    Print("Unlocking wallet \"%s\"." % (testWallet.name))
    if not walletMgr.unlockWallet(testWallet):
        cmdError("%s wallet unlock" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (testWallet.name))

    Print("Locking all wallets.")
    if not walletMgr.lockAllWallets():
        cmdError("%s wallet lock_all" % (ClientName))
        errorExit("Failed to lock all wallets")

    Print("Unlocking wallet \"%s\"." % (testWallet.name))
    if not walletMgr.unlockWallet(testWallet):
        cmdError("%s wallet unlock" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (testWallet.name))

    Print("Getting open wallet list.")
    wallets=walletMgr.getOpenWallets()
    if len(wallets) == 0 or wallets[0] != testWallet.name or len(wallets) > 1:
        Print("FAILURE - wallet list did not include %s" % (testWallet.name))
        errorExit("Unexpected wallet list: %s" % (wallets))

    Print("Getting wallet keys.")
    actualKeys=walletMgr.getKeys(testWallet)
    expectedkeys=[]
    for account in accounts:
        expectedkeys.append(account.ownerPrivateKey)
        expectedkeys.append(account.activePrivateKey)
    noMatch=list(set(expectedkeys) - set(actualKeys))
    if len(noMatch) > 0:
        errorExit("FAILURE - wallet keys did not include %s" % (noMatch), raw=True)

    Print("Locking all wallets.")
    if not walletMgr.lockAllWallets():
        cmdError("%s wallet lock_all" % (ClientName))
        errorExit("Failed to lock all wallets")

    Print("Unlocking wallet \"%s\"." % (defproduceraWallet.name))
    if not walletMgr.unlockWallet(defproduceraWallet):
        cmdError("%s wallet unlock" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (defproduceraWallet.name))

    Print("Unlocking wallet \"%s\"." % (testWallet.name))
    if not walletMgr.unlockWallet(testWallet):
        cmdError("%s wallet unlock" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (testWallet.name))

    Print("Getting wallet keys.")
    actualKeys=walletMgr.getKeys(defproduceraWallet)
    expectedkeys=[defproduceraAccount.ownerPrivateKey]
    noMatch=list(set(expectedkeys) - set(actualKeys))
    if len(noMatch) > 0:
        errorExit("FAILURE - wallet keys did not include %s" % (noMatch), raw=True)

    node=cluster.getNode(0)

    Print("Validating accounts before user accounts creation")
    cluster.validateAccounts(None)

    Print("Create new account %s via %s" % (testeraAccount.name, cluster.defproduceraAccount.name))
    transId=node.createInitializeAccount(testeraAccount, cluster.defproduceraAccount, stakedDeposit=0, waitForTransBlock=False, exitOnError=True)

    Print("Create new account %s via %s" % (currencyAccount.name, cluster.defproduceraAccount.name))
    transId=node.createInitializeAccount(currencyAccount, cluster.defproduceraAccount, buyRAM=200000, stakedDeposit=5000, exitOnError=True)

    Print("Create new account %s via %s" % (exchangeAccount.name, cluster.defproduceraAccount.name))
    transId=node.createInitializeAccount(exchangeAccount, cluster.defproduceraAccount, buyRAM=200000, waitForTransBlock=True, exitOnError=True)

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

    Print("Locking all wallets.")
    if not walletMgr.lockAllWallets():
        cmdError("%s wallet lock_all" % (ClientName))
        errorExit("Failed to lock all wallets")

    Print("Unlocking wallet \"%s\"." % (testWallet.name))
    if not walletMgr.unlockWallet(testWallet):
        cmdError("%s wallet unlock" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (testWallet.name))

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
        if not enableMongo:
            assert(actions["actions"][0]["action_trace"]["act"]["name"] == "transfer")
        else:
            assert(actions["act"]["name"] == "transfer")
    except (AssertionError, TypeError, KeyError) as _:
        Print("Action validation failed. Actions: %s" % (actions))
        raise

    node.waitForTransInBlock(transId)

    transaction=node.getTransaction(transId, exitOnError=True, delayedRetry=False)

    typeVal=None
    amountVal=None
    key=""
    try:
        if not enableMongo:
            key="[traces][0][act][name]"
            typeVal=  transaction["traces"][0]["act"]["name"]
            key="[traces][0][act][data][quantity]"
            amountVal=transaction["traces"][0]["act"]["data"]["quantity"]
            amountVal=int(decimal.Decimal(amountVal.split()[0])*10000)
        else:
            key="[actions][0][name]"
            typeVal=  transaction["actions"][0]["name"]
            key="[actions][0][data][quantity]"
            amountVal=transaction["actions"][0]["data"]["quantity"]
            amountVal=int(decimal.Decimal(amountVal.split()[0])*10000)
    except (TypeError, KeyError) as e:
        Print("transaction%s not found. Transaction: %s" % (key, transaction))
        raise

    if typeVal != "transfer" or amountVal != 975311:
        errorExit("FAILURE - get transaction trans_id failed: %s %s %s" % (transId, typeVal, amountVal), raw=True)

    Print("Currency Contract Tests")
    Print("verify no contract in place")
    Print("Get code hash for account %s" % (currencyAccount.name))
    codeHash=node.getAccountCodeHash(currencyAccount.name)
    if codeHash is None:
        cmdError("%s get code currency1111" % (ClientName))
        errorExit("Failed to get code hash for account %s" % (currencyAccount.name))
    hashNum=int(codeHash, 16)
    if hashNum != 0:
        errorExit("FAILURE - get code currency1111 failed", raw=True)

    contractDir="unittests/contracts/eosio.token"
    wasmFile="eosio.token.wasm"
    abiFile="eosio.token.abi"
    Print("Publish contract")
    trans=node.publishContract(currencyAccount, contractDir, wasmFile, abiFile, waitForTransBlock=True)
    if trans is None:
        cmdError("%s set contract currency1111" % (ClientName))
        errorExit("Failed to publish contract.")

    if not enableMongo:
        Print("Get code hash for account %s" % (currencyAccount.name))
        codeHash=node.getAccountCodeHash(currencyAccount.name)
        if codeHash is None:
            cmdError("%s get code currency1111" % (ClientName))
            errorExit("Failed to get code hash for account %s" % (currencyAccount.name))
        hashNum=int(codeHash, 16)
        if hashNum == 0:
            errorExit("FAILURE - get code currency1111 failed", raw=True)
    else:
        Print("verify abi is set")
        account=node.getEosAccountFromDb(currencyAccount.name)
        abiName=account["abi"]["structs"][0]["name"]
        abiActionName=account["abi"]["actions"][0]["name"]
        abiType=account["abi"]["actions"][0]["type"]
        if abiName != "account" or abiActionName != "close" or abiType != "close":
            errorExit("FAILURE - get EOS account failed", raw=True)

    Print("push create action to currency1111 contract")
    contract="currency1111"
    action="create"
    data="{\"issuer\":\"currency1111\",\"maximum_supply\":\"100000.0000 CUR\",\"can_freeze\":\"0\",\"can_recall\":\"0\",\"can_whitelist\":\"0\"}"
    opts="--permission currency1111@active"
    trans=node.pushMessage(contract, action, data, opts)
    try:
        assert(trans)
        assert(trans[0])
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed push create action to currency1111 contract assertion. %s" % (trans))
        raise

    Print("push issue action to currency1111 contract")
    action="issue"
    data="{\"to\":\"currency1111\",\"quantity\":\"100000.0000 CUR\",\"memo\":\"issue\"}"
    opts="--permission currency1111@active"
    trans=node.pushMessage(contract, action, data, opts)
    try:
        assert(trans)
        assert(trans[0])
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed push issue action to currency1111 contract assertion. %s" % (trans))
        raise

    Print("Verify currency1111 contract has proper initial balance (via get table)")
    contract="currency1111"
    table="accounts"
    row0=node.getTableRow(contract, currencyAccount.name, table, 0)
    try:
        assert(row0)
        assert(row0["balance"] == "100000.0000 CUR")
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed get table row assertion. %s" % (row0))
        raise

    Print("Verify currency1111 contract has proper initial balance (via get currency1111 balance)")
    amountStr=node.getTableAccountBalance("currency1111", currencyAccount.name)

    expected="100000.0000 CUR"
    actual=amountStr
    if actual != expected:
        errorExit("FAILURE - currency1111 balance check failed. Expected: %s, Recieved %s" % (expected, actual), raw=True)

    Print("Verify currency1111 contract has proper total supply of CUR (via get currency1111 stats)")
    res=node.getCurrencyStats(contract, "CUR", exitOnError=True)
    try:
        assert(res["CUR"]["supply"] == "100000.0000 CUR")
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed get currecy stats assertion. %s" % (res))
        raise

    dupRejected=False
    dupTransAmount=10
    totalTransfer=dupTransAmount
    contract="currency1111"
    action="transfer"
    for _ in range(5):
        Print("push transfer action to currency1111 contract")
        data="{\"from\":\"currency1111\",\"to\":\"defproducera\",\"quantity\":"
        data +="\"00.00%s CUR\",\"memo\":\"test\"}" % (dupTransAmount)
        opts="--permission currency1111@active"
        trans=node.pushMessage(contract, action, data, opts)
        if trans is None or not trans[0]:
            cmdError("%s push message currency1111 transfer" % (ClientName))
            errorExit("Failed to push message to currency1111 contract")
        transId=Node.getTransId(trans[1])

        Print("push duplicate transfer action to currency1111 contract")
        transDuplicate=node.pushMessage(contract, action, data, opts, True)
        if transDuplicate is not None and transDuplicate[0]:
            transDuplicateId=Node.getTransId(transDuplicate[1])
            if transId != transDuplicateId:
                Print("%s push message currency1111 duplicate transfer incorrectly accepted, but they were generated with different transaction ids, this is a timing setup issue, trying again" % (ClientName))
                # add the transfer that wasn't supposed to work
                totalTransfer+=dupTransAmount
                dupTransAmount+=1
                # add the new first transfer that is expected to work
                totalTransfer+=dupTransAmount
                continue
            else:
                cmdError("%s push message currency1111 transfer, \norig: %s \ndup: %s" % (ClientName, trans, transDuplicate))
            errorExit("Failed to reject duplicate message for currency1111 contract")
        else:
            dupRejected=True
            break

    if not dupRejected:
        errorExit("Failed to reject duplicate message for currency1111 contract")

    Print("verify transaction exists")
    if not node.waitForTransInBlock(transId):
        cmdError("%s get transaction trans_id" % (ClientName))
        errorExit("Failed to verify push message transaction id.")

    Print("read current contract balance")
    amountStr=node.getTableAccountBalance("currency1111", defproduceraAccount.name)

    expectedDefproduceraBalance="0.00%s CUR" % (totalTransfer)
    actual=amountStr
    if actual != expectedDefproduceraBalance:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (expectedDefproduceraBalance, actual), raw=True)

    amountStr=node.getTableAccountBalance("currency1111", currencyAccount.name)

    expExtension=100-totalTransfer
    expectedCurrency1111Balance="99999.99%s CUR" % (expExtension)
    actual=amountStr
    if actual != expectedCurrency1111Balance:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (expectedCurrency1111Balance, actual), raw=True)

    amountStr=node.getCurrencyBalance("currency1111", currencyAccount.name, "CUR")
    try:
        assert(actual)
        assert(isinstance(actual, str))
        actual=amountStr.strip()
        assert(expectedCurrency1111Balance == actual)
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed get currecy balance assertion. (expected=<%s>, actual=<%s>)" % (expectedCurrency1111Balance, actual))
        raise

    Print("Test for block decoded packed transaction (issue 2932)")
    blockId=node.getBlockIdByTransId(transId)
    assert(blockId)
    block=node.getBlock(blockId, exitOnError=True)

    transactions=None
    try:
        if not enableMongo:
            transactions=block["transactions"]
        else:
            transactions=block["block"]["transactions"]
        assert(transactions)
    except (AssertionError, TypeError, KeyError) as _:
        Print("FAILURE - Failed to parse block. %s" % (block))
        raise

    myTrans=None
    for trans in transactions:
        assert(trans)
        try:
            myTransId=trans["trx"]["id"]
            if transId == myTransId:
                myTrans=trans["trx"]["transaction"]
                assert(myTrans)
                break
        except (AssertionError, TypeError, KeyError) as _:
            Print("FAILURE - Failed to parse block transactions. %s" % (trans))
            raise

    assert(myTrans)
    try:
        assert(myTrans["actions"][0]["name"] == "transfer")
        assert(myTrans["actions"][0]["account"] == "currency1111")
        assert(myTrans["actions"][0]["authorization"][0]["actor"] == "currency1111")
        assert(myTrans["actions"][0]["authorization"][0]["permission"] == "active")
        assert(myTrans["actions"][0]["data"]["from"] == "currency1111")
        assert(myTrans["actions"][0]["data"]["to"] == "defproducera")
        assert(myTrans["actions"][0]["data"]["quantity"] == "0.00%s CUR" % (dupTransAmount))
        assert(myTrans["actions"][0]["data"]["memo"] == "test")
    except (AssertionError, TypeError, KeyError) as _:
        Print("FAILURE - Failed to parse block transaction. %s" % (myTrans))
        raise

    Print("Unlocking wallet \"%s\"." % (defproduceraWallet.name))
    if not walletMgr.unlockWallet(defproduceraWallet):
        cmdError("%s wallet unlock" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (defproduceraWallet.name))

    Print("push transfer action to currency1111 contract that would go negative")
    contract="currency1111"
    action="transfer"
    data="{\"from\":\"defproducera\",\"to\":\"currency1111\",\"quantity\":"
    data +="\"00.0051 CUR\",\"memo\":\"test\"}"
    opts="--permission defproducera@active"
    trans=node.pushMessage(contract, action, data, opts, True)
    if trans is None or trans[0]:
        cmdError("%s push message currency1111 transfer should have failed" % (ClientName))
        errorExit("Failed to reject invalid transfer message to currency1111 contract")

    Print("read current contract balance")
    amountStr=node.getTableAccountBalance("currency1111", defproduceraAccount.name)

    actual=amountStr
    if actual != expectedDefproduceraBalance:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (expectedDefproduceraBalance, actual), raw=True)

    amountStr=node.getTableAccountBalance("currency1111", currencyAccount.name)

    actual=amountStr
    if actual != expectedCurrency1111Balance:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (expectedCurrency1111Balance, actual), raw=True)

    Print("push another transfer action to currency1111 contract")
    contract="currency1111"
    action="transfer"
    data="{\"from\":\"defproducera\",\"to\":\"currency1111\",\"quantity\":"
    data +="\"00.00%s CUR\",\"memo\":\"test\"}" % (totalTransfer)
    opts="--permission defproducera@active"
    trans=node.pushMessage(contract, action, data, opts)
    if trans is None or not trans[0]:
        cmdError("%s push message currency1111 transfer" % (ClientName))
        errorExit("Failed to push message to currency1111 contract")
    transId=Node.getTransId(trans[1])

    Print("read current contract balance")
    amountStr=node.getCurrencyBalance("currency1111", defproduceraAccount.name, "CUR")
    expected="0.0000 CUR"
    try:
        actual=amountStr.strip()
        assert(expected == actual or not actual)
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed get currecy balance assertion. (expected=<%s>, actual=<%s>)" % (str(expected), str(actual)))
        raise

    amountStr=node.getTableAccountBalance("currency1111", currencyAccount.name)

    expected="100000.0000 CUR"
    actual=amountStr
    if actual != expected:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (str(expected), str(actual)), raw=True)

    Print("push transfer action to currency1111 contract that would go negative")
    contract="currency1111"
    action="transfer"
    data="{\"from\":\"defproducera\",\"to\":\"currency1111\",\"quantity\":"
    data +="\"00.0025 CUR\",\"memo\":\"test\"}"
    opts="--permission defproducera@active"
    trans=node.pushMessage(contract, action, data, opts, True)
    if trans is None or trans[0]:
        cmdError("%s push message currency1111 transfer should have failed" % (ClientName))
        errorExit("Failed to reject invalid transfer message to currency1111 contract")

    Print("read current contract balance")
    amountStr=node.getCurrencyBalance("currency1111", defproduceraAccount.name, "CUR")
    expected="0.0000 CUR"
    try:
        actual=amountStr.strip()
        assert(expected == actual or not actual)
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed get currecy balance assertion. (expected=<%s>, actual=<%s>)" % (str(expected), str(actual)))
        raise

    amountStr=node.getTableAccountBalance("currency1111", currencyAccount.name)

    expected="100000.0000 CUR"
    actual=amountStr
    if actual != expected:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (str(expected), str(actual)), raw=True)

    Print("push transfer action to currency1111 contract with sign skipping option enabled")
    data="{\"from\":\"currency1111\",\"to\":\"defproducera\",\"quantity\":"
    data +="\"00.0001 CUR\",\"memo\":\"test\"}"
    opts="-s -d --permission currency1111@active"
    trans=node.pushMessage(contract, action, data, opts)

    try:
        assert(not trans[1]["signatures"])
    except (AssertionError, KeyError) as _:
        Print("ERROR: Expected signatures array to be empty due to skipping option enabled.")
        raise

    try:
        assert(trans[1]["actions"][0]["data"]["from"] == "currency1111")
        assert(trans[1]["actions"][0]["data"]["to"] == "defproducera")
        assert(trans[1]["actions"][0]["data"]["quantity"] == "0.0001 CUR")
        assert(trans[1]["actions"][0]["data"]["memo"] == "test")
    except (AssertionError, KeyError) as _:
        Print("ERROR: Expecting unpacked data fields on push transfer action json result.")
        raise

    result=node.pushTransaction(trans[1], None)

    amountStr=node.getTableAccountBalance("currency1111", currencyAccount.name)

    expected="99999.9999 CUR"
    actual=amountStr
    if actual != expected:
        errorExit("FAILURE - Wrong currency1111 balance (expectedgma=%s, actual=%s)" % (str(expected), str(actual)), raw=True)

    Print("---- Test for signing transaction ----")
    testeraAccountAmountBeforeTrx=node.getAccountEosBalanceStr(testeraAccount.name)
    currencyAccountAmountBeforeTrx=node.getAccountEosBalanceStr(currencyAccount.name)

    xferAmount="1.2345 {0}".format(CORE_SYMBOL)
    unsignedTrxRet = node.transferFunds(currencyAccount, testeraAccount, xferAmount, "unsigned trx", False, False, True, False, False, True, None, True)
    unsignedTrxJsonFile = "unsigned_trx_file"
    with open(unsignedTrxJsonFile, 'w') as outfile:
        json.dump(unsignedTrxRet, outfile)
    testeraAccountAmountAftrTrx=node.getAccountEosBalanceStr(testeraAccount.name)
    currencyAccountAmountAftrTrx=node.getAccountEosBalanceStr(currencyAccount.name)
    try:
        assert(testeraAccountAmountBeforeTrx == testeraAccountAmountAftrTrx)
        assert(currencyAccountAmountBeforeTrx == currencyAccountAmountAftrTrx)
    except (AssertionError) as _:
        Print("ERROR: Expecting transfer is not executed.")
        raise

    signCmd = "sign --public-key {0} {1} -p".format(currencyAccount.activePublicKey, unsignedTrxJsonFile)
    node.processCleosCmd(signCmd, "Sign and push a transaction", False, True)
    os.remove(unsignedTrxJsonFile)

    testeraAccountAmountAfterSign=node.getAccountEosBalanceStr(testeraAccount.name)
    currencyAccountAmountAfterSign=node.getAccountEosBalanceStr(currencyAccount.name)
    try:
        assert(Utils.addAmount(testeraAccountAmountAftrTrx, xferAmount) == testeraAccountAmountAfterSign)
        assert(Utils.deduceAmount(currencyAccountAmountAftrTrx, xferAmount) == currencyAccountAmountAfterSign)
    except (AssertionError) as _:
        Print("ERROR: Expecting transfer has been executed with exact amount.")
        raise

    Print("Locking wallet \"%s\"." % (defproduceraWallet.name))
    if not walletMgr.lockWallet(defproduceraWallet):
        cmdError("%s wallet lock" % (ClientName))
        errorExit("Failed to lock wallet %s" % (defproduceraWallet.name))


    simpleDB = Account("simpledb")
    contractDir="contracts/simpledb"
    wasmFile="simpledb.wasm"
    abiFile="simpledb.abi"
    Print("Setting simpledb contract without simpledb account was causing core dump in %s." % (ClientName))
    Print("Verify %s generates an error, but does not core dump." % (ClientName))
    retMap=node.publishContract(simpleDB, contractDir, wasmFile, abiFile, shouldFail=True)
    if retMap is None:
        errorExit("Failed to publish, but should have returned a details map")
    if retMap["returncode"] == 0 or retMap["returncode"] == 139: # 139 SIGSEGV
        errorExit("FAILURE - set contract simpledb failed", raw=True)
    else:
        Print("Test successful, %s returned error code: %d" % (ClientName, retMap["returncode"]))

    Print("set permission")
    pType="transfer"
    requirement="active"
    trans=node.setPermission(testeraAccount, currencyAccount, pType, requirement, waitForTransBlock=True, exitOnError=True)

    Print("remove permission")
    requirement="null"
    trans=node.setPermission(testeraAccount, currencyAccount, pType, requirement, waitForTransBlock=True, exitOnError=True)

    Print("Locking all wallets.")
    if not walletMgr.lockAllWallets():
        cmdError("%s wallet lock_all" % (ClientName))
        errorExit("Failed to lock all wallets")

    Print("Unlocking wallet \"%s\"." % (defproduceraWallet.name))
    if not walletMgr.unlockWallet(defproduceraWallet):
        cmdError("%s wallet unlock defproducera" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (defproduceraWallet.name))

    Print("Get account defproducera")
    account=node.getEosAccount(defproduceraAccount.name, exitOnError=True)

    Print("Unlocking wallet \"%s\"." % (defproduceraWallet.name))
    if not walletMgr.unlockWallet(testWallet):
        cmdError("%s wallet unlock test" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (testWallet.name))

    if not enableMongo:
        Print("Verify non-JSON call works")
        rawAccount=node.getEosAccount(defproduceraAccount.name, exitOnError=True, returnType=ReturnType.raw)
        coreLiquidBalance=account['core_liquid_balance']
        match=re.search(r'\bliquid:\s*%s\s' % (coreLiquidBalance), rawAccount, re.MULTILINE | re.DOTALL)
        assert match is not None, "did not find the core liquid balance (\"liquid:\") of {} in \"{}\"".format(coreLiquidBalance, rawAccount)

    Print("Get head block num.")
    currentBlockNum=node.getHeadBlockNum()
    Print("CurrentBlockNum: %d" % (currentBlockNum))
    Print("Request blocks 1-%d" % (currentBlockNum))
    start=1
    if enableMongo:
        start=2 # block 1 (genesis block) is not signaled to the plugins, so not available in DB
    for blockNum in range(start, currentBlockNum+1):
        block=node.getBlock(blockNum, silentErrors=False, exitOnError=True)

        if enableMongo:
            blockId=block["block_id"]
            block2=node.getBlockByIdMdb(blockId)
            if block2 is None:
                errorExit("mongo get block by id %s" % blockId)

    Print("Request invalid block numbered %d. This will generate an expected error message." % (currentBlockNum+1000))
    currentBlockNum=node.getHeadBlockNum() # If the tests take too long, we could be far beyond currentBlockNum+1000 and that'll cause a block to be found.
    block=node.getBlock(currentBlockNum+1000, silentErrors=True)
    if block is not None:
        errorExit("ERROR: Received block where not expected")
    else:
        Print("Success: No such block found")

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

    Print("Validating accounts at end of test")
    accounts=[testeraAccount, currencyAccount, exchangeAccount]
    cluster.validateAccounts(accounts)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
