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

from core_symbol import CORE_SYMBOL

def cmdError(name, cmdCode=0, exitNow=False):
    msg="FAILURE - %s%s" % (name, ("" if cmdCode == 0 else (" returned error code %d" % cmdCode)))
    if exitNow:
        errorExit(msg, True)
    else:
        Print(msg)

args = TestHelper.parse_args({"--host","--port","--prod-count","--defproducera_prvt_key","--defproducerb_prvt_key","--mongodb"
                              ,"--dump-error-details","--dont-launch","--keep-logs","-v","--leave-running","--only-bios","--clean-run"
                              ,"--sanity-test"})
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

Utils.Debug=debug
localTest=True if server == TestHelper.LOCAL_HOST else False
cluster=Cluster(walletd=True, enableMongo=enableMongo, defproduceraPrvtKey=defproduceraPrvtKey, defproducerbPrvtKey=defproducerbPrvtKey)
walletMgr=WalletMgr(True)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill
dontBootstrap=sanityTest

WalletdName="keosd"
ClientName="cleos"
# Utils.setMongoSyncTime(50)

try:
    TestHelper.printSystemInfo("BEGIN")
    Print("SERVER: %s" % (server))
    Print("PORT: %d" % (port))

    if enableMongo and not cluster.isMongodDbRunning():
        errorExit("MongoDb doesn't seem to be running.")

    walletMgr.killall(allInstances=killAll)
    walletMgr.cleanup()

    if localTest and not dontLaunch:
        cluster.killall(allInstances=killAll)
        cluster.cleanup()
        Print("Stand up cluster")
        if cluster.launch(prodCount=prodCount, onlyBios=onlyBios, dontKill=dontKill, dontBootstrap=dontBootstrap) is False:
            cmdError("launcher")
            errorExit("Failed to stand up eos cluster.")
    else:
        cluster.initializeNodes(defproduceraPrvtKey=defproduceraPrvtKey, defproducerbPrvtKey=defproducerbPrvtKey)
        killEosInstances=False

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

    Print("Stand up %s" % (WalletdName))
    walletMgr.killall(allInstances=killAll)
    walletMgr.cleanup()
    if walletMgr.launch() is False:
        cmdError("%s" % (WalletdName))
        errorExit("Failed to stand up eos walletd.")

    testWalletName="test"
    Print("Creating wallet \"%s\"." % (testWalletName))
    testWallet=walletMgr.create(testWalletName, [cluster.eosioAccount,cluster.defproduceraAccount,cluster.defproducerbAccount])
    if testWallet is None:
        cmdError("eos wallet create")
        errorExit("Failed to create wallet %s." % (testWalletName))

    Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))

    for account in accounts:
        Print("Importing keys for account %s into wallet %s." % (account.name, testWallet.name))
        if not walletMgr.importKey(account, testWallet):
            cmdError("%s wallet import" % (ClientName))
            errorExit("Failed to import key for account %s" % (account.name))

    defproduceraWalletName="defproducera"
    Print("Creating wallet \"%s\"." % (defproduceraWalletName))
    defproduceraWallet=walletMgr.create(defproduceraWalletName)
    if defproduceraWallet is None:
        cmdError("eos wallet create")
        errorExit("Failed to create wallet %s." % (defproduceraWalletName))

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
    if node is None:
        errorExit("Cluster in bad state, received None node")

    Print("Validating accounts before user accounts creation")
    cluster.validateAccounts(None)

    # create accounts via eosio as otherwise a bid is needed 
    Print("Create new account %s via %s" % (testeraAccount.name, cluster.eosioAccount.name))
    transId=node.createInitializeAccount(testeraAccount, cluster.eosioAccount, stakedDeposit=0, waitForTransBlock=False)
    if transId is None:
        cmdError("%s create account" % (testeraAccount.name))
        errorExit("Failed to create account %s" % (testeraAccount.name))

    Print("Create new account %s via %s" % (currencyAccount.name, cluster.eosioAccount.name))
    transId=node.createInitializeAccount(currencyAccount, cluster.eosioAccount, stakedDeposit=5000)
    if transId is None:
        cmdError("%s create account" % (ClientName))
        errorExit("Failed to create account %s" % (currencyAccount.name))

    Print("Create new account %s via %s" % (exchangeAccount.name, cluster.eosioAccount.name))
    transId=node.createInitializeAccount(exchangeAccount, cluster.eosioAccount, waitForTransBlock=True)
    if transId is None:
        cmdError("%s create account" % (ClientName))
        errorExit("Failed to create account %s" % (exchangeAccount.name))

    Print("Validating accounts after user accounts creation")
    accounts=[testeraAccount, currencyAccount, exchangeAccount]
    cluster.validateAccounts(accounts)

    Print("Verify account %s" % (testeraAccount))
    if not node.verifyAccount(testeraAccount):
        errorExit("FAILURE - account creation failed.", raw=True)

    transferAmount="97.5321 {0}".format(CORE_SYMBOL)
    Print("Transfer funds %s from account %s to %s" % (transferAmount, defproduceraAccount.name, testeraAccount.name))
    if node.transferFunds(defproduceraAccount, testeraAccount, transferAmount, "test transfer") is None:
        cmdError("%s transfer" % (ClientName))
        errorExit("Failed to transfer funds %d from account %s to %s" % (
            transferAmount, defproduceraAccount.name, testeraAccount.name))

    expectedAmount=transferAmount
    Print("Verify transfer, Expected: %s" % (expectedAmount))
    actualAmount=node.getAccountEosBalanceStr(testeraAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %s, actual: %s" % (expectedAmount, actualAmount))

    transferAmount="0.0100 {0}".format(CORE_SYMBOL)
    Print("Force transfer funds %s from account %s to %s" % (
        transferAmount, defproduceraAccount.name, testeraAccount.name))
    if node.transferFunds(defproduceraAccount, testeraAccount, transferAmount, "test transfer", force=True) is None:
        cmdError("%s transfer" % (ClientName))
        errorExit("Failed to force transfer funds %d from account %s to %s" % (
            transferAmount, defproduceraAccount.name, testeraAccount.name))

    expectedAmount="97.5421 {0}".format(CORE_SYMBOL)
    Print("Verify transfer, Expected: %s" % (expectedAmount))
    actualAmount=node.getAccountEosBalanceStr(testeraAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %s, actual: %s" % (expectedAmount, actualAmount))

    Print("Validating accounts after some user trasactions")
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
    if trans is None:
        cmdError("%s transfer" % (ClientName))
        errorExit("Failed to transfer funds %d from account %s to %s" % (
            transferAmount, testeraAccount.name, currencyAccount.name))
    transId=Node.getTransId(trans)

    expectedAmount="98.0311 {0}".format(CORE_SYMBOL) # 5000 initial deposit
    Print("Verify transfer, Expected: %s" % (expectedAmount))
    actualAmount=node.getAccountEosBalanceStr(currencyAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %s, actual: %s" % (expectedAmount, actualAmount))

    Print("Validate last action for account %s" % (testeraAccount.name))
    actions=node.getActions(testeraAccount, -1, -1)
    assert(actions)
    try:
        assert(actions["actions"][0]["action_trace"]["act"]["name"] == "transfer")
    except (AssertionError, TypeError, KeyError) as _:
        Print("Last action validation failed. Actions: %s" % (actions))
        raise

    # This API (get accounts) is no longer supported (Issue 2876)
    # expectedAccounts=[testeraAccount.name, currencyAccount.name, exchangeAccount.name]
    # Print("Get accounts by key %s, Expected: %s" % (PUB_KEY3, expectedAccounts))
    # actualAccounts=node.getAccountsArrByKey(PUB_KEY3)
    # if actualAccounts is None:
    #     cmdError("%s get accounts pub_key3" % (ClientName))
    #     errorExit("Failed to retrieve accounts by key %s" % (PUB_KEY3))
    # noMatch=list(set(expectedAccounts) - set(actualAccounts))
    # if len(noMatch) > 0:
    #     errorExit("FAILURE - Accounts lookup by key %s. Expected: %s, Actual: %s" % (
    #         PUB_KEY3, expectedAccounts, actualAccounts), raw=True)
    #
    # expectedAccounts=[testeraAccount.name]
    # Print("Get accounts by key %s, Expected: %s" % (PUB_KEY1, expectedAccounts))
    # actualAccounts=node.getAccountsArrByKey(PUB_KEY1)
    # if actualAccounts is None:
    #     cmdError("%s get accounts pub_key1" % (ClientName))
    #     errorExit("Failed to retrieve accounts by key %s" % (PUB_KEY1))
    # noMatch=list(set(expectedAccounts) - set(actualAccounts))
    # if len(noMatch) > 0:
    #     errorExit("FAILURE - Accounts lookup by key %s. Expected: %s, Actual: %s" % (
    #         PUB_KEY1, expectedAccounts, actualAccounts), raw=True)

    # This API (get servants) is no longer supported. (Issue 3160)
    # expectedServants=[testeraAccount.name, currencyAccount.name]
    # Print("Get %s servants, Expected: %s" % (defproduceraAccount.name, expectedServants))
    # actualServants=node.getServantsArr(defproduceraAccount.name)
    # if actualServants is None:
    #     cmdError("%s get servants testera11111" % (ClientName))
    #     errorExit("Failed to retrieve %s servants" % (defproduceraAccount.name))
    # noMatch=list(set(expectedAccounts) - set(actualAccounts))
    # if len(noMatch) > 0:
    #     errorExit("FAILURE - %s servants. Expected: %s, Actual: %s" % (
    #         defproduceraAccount.name, expectedServants, actualServants), raw=True)
    #
    # Print("Get %s servants, Expected: []" % (testeraAccount.name))
    # actualServants=node.getServantsArr(testeraAccount.name)
    # if actualServants is None:
    #     cmdError("%s get servants testera11111" % (ClientName))
    #     errorExit("Failed to retrieve %s servants" % (testeraAccount.name))
    # if len(actualServants) > 0:
    #     errorExit("FAILURE - %s servants. Expected: [], Actual: %s" % (
    #         testeraAccount.name, actualServants), raw=True)

    node.waitForTransInBlock(transId)

    transaction=None
    if not enableMongo:
        transaction=node.getTransaction(transId)
    else:
        transaction=node.getActionFromDb(transId)
    if transaction is None:
        cmdError("%s get transaction trans_id" % (ClientName))
        errorExit("Failed to retrieve transaction details %s" % (transId))

    typeVal=None
    amountVal=None
    assert(transaction)
    try:
        if not enableMongo:
            typeVal=  transaction["traces"][0]["act"]["name"]
            amountVal=transaction["traces"][0]["act"]["data"]["quantity"]
            amountVal=int(decimal.Decimal(amountVal.split()[0])*10000)
        else:
            typeVal=  transaction["name"]
            amountVal=transaction["data"]["quantity"]
            amountVal=int(decimal.Decimal(amountVal.split()[0])*10000)
    except (TypeError, KeyError) as e:
        Print("Transaction validation parsing failed. Transaction: %s" % (transaction))
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

    contractDir="contracts/eosio.token"
    wastFile="contracts/eosio.token/eosio.token.wast"
    abiFile="contracts/eosio.token/eosio.token.abi"
    Print("Publish contract")
    trans=node.publishContract(currencyAccount.name, contractDir, wastFile, abiFile, waitForTransBlock=True)
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
        if abiName != "transfer" or abiActionName != "transfer" or abiType != "transfer":
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
    res=node.getCurrencyStats(contract, "CUR")
    try:
        assert(res)
        assert(res["CUR"]["supply"] == "100000.0000 CUR")
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed get currecy stats assertion. %s" % (res))
        raise

    Print("push transfer action to currency1111 contract")
    contract="currency1111"
    action="transfer"
    data="{\"from\":\"currency1111\",\"to\":\"defproducera\",\"quantity\":"
    data +="\"00.0050 CUR\",\"memo\":\"test\"}"
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
            cmdError("%s push message currency1111 duplicate transfer incorrectly accepted, but they were generated with different transaction ids, it is likely a timing issue, report if problem persists, \norig: %s \ndup: %s" % (ClientName, trans, transDuplicate))
        else:
            cmdError("%s push message currency1111 transfer, \norig: %s \ndup: %s" % (ClientName, trans, transDuplicate))
        errorExit("Failed to reject duplicate message for currency1111 contract")

    Print("verify transaction exists")
    if not node.waitForTransInBlock(transId):
        cmdError("%s get transaction trans_id" % (ClientName))
        errorExit("Failed to verify push message transaction id.")

    Print("read current contract balance")
    amountStr=node.getTableAccountBalance("currency1111", defproduceraAccount.name)

    expected="0.0050 CUR"
    actual=amountStr
    if actual != expected:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (str(expected), str(actual)), raw=True)

    amountStr=node.getTableAccountBalance("currency1111", currencyAccount.name)

    expected="99999.9950 CUR"
    actual=amountStr
    if actual != expected:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (str(expected), str(actual)), raw=True)

    amountStr=node.getCurrencyBalance("currency1111", currencyAccount.name, "CUR")
    try:
        assert(actual)
        assert(isinstance(actual, str))
        actual=amountStr.strip()
        assert(expected == actual)
    except (AssertionError, KeyError) as _:
        Print("ERROR: Failed get currecy balance assertion. (expected=<%s>, actual=<%s>)" % (str(expected), str(actual)))
        raise

    Print("Test for block decoded packed transaction (issue 2932)")
    blockId=node.getBlockIdByTransId(transId)
    assert(blockId)
    block=node.getBlock(blockId)
    assert(block)
    transactions=None
    try:
        transactions=block["transactions"]
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
        assert(myTrans["actions"][0]["data"]["quantity"] == "0.0050 CUR")
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

    expected="0.0050 CUR"
    actual=amountStr
    if actual != expected:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (str(expected), str(actual)), raw=True)

    amountStr=node.getTableAccountBalance("currency1111", currencyAccount.name)

    expected="99999.9950 CUR"
    actual=amountStr
    if actual != expected:
        errorExit("FAILURE - Wrong currency1111 balance (expected=%s, actual=%s)" % (str(expected), str(actual)), raw=True)

    Print("push another transfer action to currency1111 contract")
    contract="currency1111"
    action="transfer"
    data="{\"from\":\"defproducera\",\"to\":\"currency1111\",\"quantity\":"
    data +="\"00.0050 CUR\",\"memo\":\"test\"}"
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

    Print("Locking wallet \"%s\"." % (defproduceraWallet.name))
    if not walletMgr.lockWallet(defproduceraWallet):
        cmdError("%s wallet lock" % (ClientName))
        errorExit("Failed to lock wallet %s" % (defproduceraWallet.name))


    Print("Exchange Contract Tests")
    Print("upload exchange contract")

    contractDir="contracts/exchange"
    wastFile="contracts/exchange/exchange.wast"
    abiFile="contracts/exchange/exchange.abi"
    Print("Publish exchange contract")
    trans=node.publishContract(exchangeAccount.name, contractDir, wastFile, abiFile, waitForTransBlock=True)
    if trans is None:
        cmdError("%s set contract exchange" % (ClientName))
        errorExit("Failed to publish contract.")

    contractDir="contracts/simpledb"
    wastFile="contracts/simpledb/simpledb.wast"
    abiFile="contracts/simpledb/simpledb.abi"
    Print("Setting simpledb contract without simpledb account was causing core dump in %s." % (ClientName))
    Print("Verify %s generates an error, but does not core dump." % (ClientName))
    retMap=node.publishContract("simpledb", contractDir, wastFile, abiFile, shouldFail=True)
    if retMap is None:
        errorExit("Failed to publish, but should have returned a details map")
    if retMap["returncode"] == 0 or retMap["returncode"] == 139: # 139 SIGSEGV
        errorExit("FAILURE - set contract exchange failed", raw=True)
    else:
        Print("Test successful, %s returned error code: %d" % (ClientName, retMap["returncode"]))

    Print("set permission")
    code="currency1111"
    pType="transfer"
    requirement="active"
    trans=node.setPermission(testeraAccount.name, code, pType, requirement, waitForTransBlock=True)
    if trans is None:
        cmdError("%s set action permission set" % (ClientName))
        errorExit("Failed to set permission")

    Print("remove permission")
    requirement="null"
    trans=node.setPermission(testeraAccount.name, code, pType, requirement, waitForTransBlock=True)
    if trans is None:
        cmdError("%s set action permission set" % (ClientName))
        errorExit("Failed to remove permission")

    Print("Locking all wallets.")
    if not walletMgr.lockAllWallets():
        cmdError("%s wallet lock_all" % (ClientName))
        errorExit("Failed to lock all wallets")

    Print("Unlocking wallet \"%s\"." % (defproduceraWallet.name))
    if not walletMgr.unlockWallet(defproduceraWallet):
        cmdError("%s wallet unlock defproducera" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (defproduceraWallet.name))

    Print("Get account defproducera")
    account=node.getEosAccount(defproduceraAccount.name)
    if account is None:
        cmdError("%s get account" % (ClientName))
        errorExit("Failed to get account %s" % (defproduceraAccount.name))

    #
    # Proxy
    #
    # not implemented

    Print("Get head block num.")
    currentBlockNum=node.getHeadBlockNum()
    Print("CurrentBlockNum: %d" % (currentBlockNum))
    Print("Request blocks 1-%d" % (currentBlockNum))
    for blockNum in range(1, currentBlockNum+1):
        block=node.getBlock(blockNum, retry=False, silentErrors=False)
        if block is None:
            cmdError("%s get block" % (ClientName))
            errorExit("get block by num %d" % blockNum)

        if enableMongo:
            blockId=block["block_id"]
            block2=node.getBlockById(blockId, retry=False)
            if block2 is None:
                errorExit("mongo get block by id %s" % blockId)

            # TBD: getTransByBlockId() needs to handle multiple returned transactions
            # trans=node.getTransByBlockId(blockId, retry=False)
            # if trans is not None:
            #     transId=Node.getTransId(trans)
            #     trans2=node.getMessageFromDb(transId)
            #     if trans2 is None:
            #         errorExit("mongo get messages by transaction id %s" % (transId))


    Print("Request invalid block numbered %d. This will generate an expected error message." % (currentBlockNum+1000))
    block=node.getBlock(currentBlockNum+1000, silentErrors=True, retry=False)
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
