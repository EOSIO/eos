#!/usr/bin/env python3

import testUtils

import decimal
import argparse
import random
import re

###############################################################
# eosiod_run_test
# --dump-error-details <Upon error print tn_data_*/config.ini and tn_data_*/stderr.log to stdout>
# --keep-logs <Don't delete tn_data_* folders upon test completion>
###############################################################

Print=testUtils.Utils.Print
errorExit=testUtils.Utils.errorExit


def cmdError(name, code=0, exitNow=False):
    msg="FAILURE - %s%s" % (name, ("" if code == 0 else (" returned error code %d" % code)))
    if exitNow:
        errorExit(msg, True)
    else:
        Print(msg)

TEST_OUTPUT_DEFAULT="test_output_0.txt"
LOCAL_HOST="localhost"
DEFAULT_PORT=8888

parser = argparse.ArgumentParser(add_help=False)
# Override default help argument so that only --help (and not -h) can call help
parser.add_argument('-?', action='help', default=argparse.SUPPRESS,
                    help=argparse._('show this help message and exit'))
parser.add_argument("-o", "--output", type=str, help="output file", default=TEST_OUTPUT_DEFAULT)
parser.add_argument("-h", "--host", type=str, help="%s host name" % (testUtils.Utils.EosServerName),
                    default=LOCAL_HOST)
parser.add_argument("-p", "--port", type=int, help="%s host port" % testUtils.Utils.EosServerName,
                    default=DEFAULT_PORT)
parser.add_argument("--inita_prvt_key", type=str, help="Inita private key.",
                    default=testUtils.Cluster.initaAccount.ownerPrivateKey)
parser.add_argument("--initb_prvt_key", type=str, help="Initb private key.",
                    default=testUtils.Cluster.initbAccount.ownerPrivateKey)
parser.add_argument("--mongodb", help="Configure a MongoDb instance", action='store_true')
parser.add_argument("--dump-error-details",
                    help="Upon error print tn_data_*/config.ini and tn_data_*/stderr.log to stdout",
                    action='store_true')
parser.add_argument("--dont-launch", help="Don't launch own node. Assume node is already running.",
                    action='store_true')
parser.add_argument("--keep-logs", help="Don't delete tn_data_* folders upon test completion",
                    action='store_true')
parser.add_argument("--exit-early", help="Exit prior to known error point.", action='store_true')
parser.add_argument("-v", help="verbose logging", action='store_true')
parser.add_argument("--not-noon", help="This is not the Noon branch.", action='store_true')


args = parser.parse_args()
testOutputFile=args.output
server=args.host
port=args.port
debug=args.v
exitEarly=args.exit_early
enableMongo=args.mongodb
amINoon=not args.not_noon
initaPrvtKey=args.inita_prvt_key
initbPrvtKey=args.initb_prvt_key
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontLaunch=args.dont_launch

testUtils.Utils.Debug=debug
localTest=True if server == LOCAL_HOST else False
cluster=testUtils.Cluster(walletd=True, enableMongo=enableMongo, initaPrvtKey=initaPrvtKey, initbPrvtKey=initbPrvtKey)
walletMgr=testUtils.WalletMgr(True)

testSuccessful=False
killEosInstances=True
killWallet=True

WalletdName="eos-walletd"
ClientName="eosc"

if amINoon:
    WalletdName="eosio-walletd"
    ClientName="eosioc"
    # noon branch requires longer mongo sync time.
    testUtils.Utils.setMongoSyncTime(50)
else:
    testUtils.Utils.iAmNotNoon()

try:
    Print("BEGIN")
    print("TEST_OUTPUT: %s" % (testOutputFile))
    print("SERVER: %s" % (server))
    print("PORT: %d" % (port))

    if localTest and not dontLaunch:
        cluster.killall()
        cluster.cleanup()
        Print("Stand up cluster")
        if cluster.launch() is False:
            cmdError("launcher")
            errorExit("Failed to stand up eos cluster.")
    else:
        cluster.initializeNodes()
        killEosInstances=False

    walletMgr.killall()
    walletMgr.cleanup()

    accounts=testUtils.Cluster.createAccountKeys(3)
    if accounts is None:
        errorExit("FAILURE - create keys")
    testeraAccount=accounts[0]
    testeraAccount.name="testera"
    currencyAccount=accounts[1]
    currencyAccount.name="currency"
    exchangeAccount=accounts[2]
    exchangeAccount.name="exchange"

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
    
    Print("Stand up walletd")
    if walletMgr.launch() is False:
        cmdError("%s" % (WalletdName))
        errorExit("Failed to stand up eos walletd.")

    testWalletName="test"
    Print("Creating wallet \"%s\"." % (testWalletName))
    testWallet=walletMgr.create(testWalletName)
    if testWallet is None:
        cmdError("eos wallet create")
        errorExit("Failed to create wallet %s." % (testWalletName))

    for account in accounts:
        Print("Importing keys for account %s into wallet %s." % (account.name, testWallet.name))
        if not walletMgr.importKey(account, testWallet):
            cmdError("%s wallet import" % (ClientName))
            errorExit("Failed to import key for account %s" % (account.name))

    initaWalletName="inita"
    Print("Creating wallet \"%s\"." % (initaWalletName))
    initaWallet=walletMgr.create(initaWalletName)
    if initaWallet is None:
        cmdError("eos wallet create")
        errorExit("Failed to create wallet %s." % (initaWalletName))
    
    initaAccount=testUtils.Cluster.initaAccount
    initbAccount=testUtils.Cluster.initbAccount

    Print("Importing keys for account %s into wallet %s." % (initaAccount.name, initaWallet.name))
    if not walletMgr.importKey(initaAccount, initaWallet):
        cmdError("%s wallet import" % (ClientName))
        errorExit("Failed to import key for account %s" % (initaAccount.name))

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
    actualKeys=walletMgr.getKeys()
    expectedkeys=[]
    for account in accounts:
        expectedkeys.append(account.ownerPrivateKey)
        expectedkeys.append(account.activePrivateKey)
    noMatch=list(set(expectedkeys) - set(actualKeys))
    if len(noMatch) > 0:
        errorExit("FAILURE - wallet keys did not include %s" % (noMatch), raw=true)

    Print("Locking all wallets.")
    if not walletMgr.lockAllWallets():
        cmdError("%s wallet lock_all" % (ClientName))
        errorExit("Failed to lock all wallets")

    Print("Unlocking wallet \"%s\"." % (initaWallet.name))
    if not walletMgr.unlockWallet(initaWallet):
        cmdError("%s wallet unlock" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (testWallet.name))

    Print("Getting wallet keys.")
    actualKeys=walletMgr.getKeys()
    expectedkeys=[initaAccount.ownerPrivateKey]
    noMatch=list(set(expectedkeys) - set(actualKeys))
    if len(noMatch) > 0:
        errorExit("FAILURE - wallet keys did not include %s" % (noMatch), raw=true)
        
    node=cluster.getNode(0)
    if node is None:
        errorExit("Cluster in bad state, received None node")

    Print("Create initial accounts")
    node.createInitAccounts()

    Print("Create new account %s via %s" % (testeraAccount.name, initaAccount.name))
    transId=node.createAccount(testeraAccount, initaAccount, stakedDeposit=0, waitForTransBlock=True)
    if transId is None:
        cmdError("%s create account" % (ClientName))
        errorExit("Failed to create account %s" % (testeraAccount.name))

    Print("Verify account %s" % (testeraAccount))
    if not node.verifyAccount(testeraAccount):
        errorExit("FAILURE - account creation failed.", raw=True)

    transferAmount=975321
    Print("Transfer funds %d from account %s to %s" % (transferAmount, initaAccount.name, testeraAccount.name))
    if node.transferFunds(initaAccount, testeraAccount, transferAmount, "test transfer") is None:
        cmdError("%s transfer" % (ClientName))
        errorExit("Failed to transfer funds %d from account %s to %s" % (
            transferAmount, initaAccount.name, testeraAccount.name))

    expectedAmount=transferAmount
    Print("Verify transfer, Expected: %d" % (expectedAmount))
    actualAmount=node.getAccountBalance(testeraAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %d, actual: %d" % (expectedAmount, actualAmount))

    transferAmount=100
    Print("Force transfer funds %d from account %s to %s" % (
        transferAmount, initaAccount.name, testeraAccount.name))
    if node.transferFunds(initaAccount, testeraAccount, transferAmount, "test transfer", force=True) is None:
        cmdError("%s transfer" % (ClientName))
        errorExit("Failed to force transfer funds %d from account %s to %s" % (
            transferAmount, initaAccount.name, testeraAccount.name))

    expectedAmount=975421
    Print("Verify transfer, Expected: %d" % (expectedAmount))
    actualAmount=node.getAccountBalance(testeraAccount.name)
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %d, actual: %d" % (expectedAmount, actualAmount))

    Print("Create new account %s via %s" % (currencyAccount.name, initbAccount.name))
    transId=node.createAccount(currencyAccount, initbAccount, stakedDeposit=5000)
    if transId is None:
        cmdError("%s create account" % (ClientName))
        errorExit("Failed to create account %s" % (currencyAccount.name))

    Print("Create new account %s via %s" % (exchangeAccount.name, initaAccount.name))
    transId=node.createAccount(exchangeAccount, initaAccount, waitForTransBlock=True)
    if transId is None:
        cmdError("%s create account" % (ClientName))
        errorExit("Failed to create account %s" % (exchangeAccount.name))

    Print("Locking all wallets.")
    if not walletMgr.lockAllWallets():
        cmdError("%s wallet lock_all" % (ClientName))
        errorExit("Failed to lock all wallets")

    Print("Unlocking wallet \"%s\"." % (testWallet.name))
    if not walletMgr.unlockWallet(testWallet):
        cmdError("%s wallet unlock" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (testWallet.name))

    transferAmount=975311
    Print("Transfer funds %d from account %s to %s" % (
        transferAmount, testeraAccount.name, currencyAccount.name))
    trans=node.transferFunds(testeraAccount, currencyAccount, transferAmount, "test transfer a->b")
    if trans is None:
        cmdError("%s transfer" % (ClientName))
        errorExit("Failed to transfer funds %d from account %s to %s" % (
            transferAmount, initaAccount.name, testeraAccount.name))
    transId=testUtils.Node.getTransId(trans)

    expectedAmount=975311+5000 # 5000 initial deposit
    Print("Verify transfer, Expected: %d" % (expectedAmount))
    actualAmount=node.getAccountBalance(currencyAccount.name)
    if actualAmount is None:
        cmdError("%s get account currency" % (ClientName))
        errorExit("Failed to retrieve balance for account %s" % (currencyAccount.name))
    if expectedAmount != actualAmount:
        cmdError("FAILURE - transfer failed")
        errorExit("Transfer verification failed. Excepted %d, actual: %d" % (expectedAmount, actualAmount))

    expectedAccounts=[testeraAccount.name, currencyAccount.name, exchangeAccount.name]
    Print("Get accounts by key %s, Expected: %s" % (PUB_KEY3, expectedAccounts))
    actualAccounts=node.getAccountsArrByKey(PUB_KEY3)
    if actualAccounts is None:
        cmdError("%s get accounts pub_key3" % (ClientName))
        errorExit("Failed to retrieve accounts by key %s" % (PUB_KEY3))
    noMatch=list(set(expectedAccounts) - set(actualAccounts))
    if len(noMatch) > 0:
        errorExit("FAILURE - Accounts lookup by key %s. Expected: %s, Actual: %s" % (
            PUB_KEY3, expectedAccounts, actualAccounts), raw=True)

    expectedAccounts=[testeraAccount.name]
    Print("Get accounts by key %s, Expected: %s" % (PUB_KEY1, expectedAccounts))
    actualAccounts=node.getAccountsArrByKey(PUB_KEY1)
    if actualAccounts is None:
        cmdError("%s get accounts pub_key1" % (ClientName))
        errorExit("Failed to retrieve accounts by key %s" % (PUB_KEY1))
    noMatch=list(set(expectedAccounts) - set(actualAccounts))
    if len(noMatch) > 0:
        errorExit("FAILURE - Accounts lookup by key %s. Expected: %s, Actual: %s" % (
            PUB_KEY1, expectedAccounts, actualAccounts), raw=True)

    expectedServants=[testeraAccount.name, currencyAccount.name]
    Print("Get %s servants, Expected: %s" % (initaAccount.name, expectedServants))
    actualServants=node.getServantsArr(initaAccount.name)
    if actualServants is None:
        cmdError("%s get servants testera" % (ClientName))
        errorExit("Failed to retrieve %s servants" % (initaAccount.name))
    noMatch=list(set(expectedAccounts) - set(actualAccounts))
    if len(noMatch) > 0:
        errorExit("FAILURE - %s servants. Expected: %s, Actual: %s" % (
            initaAccount.name, expectedServants, actualServants), raw=True)

    Print("Get %s servants, Expected: []" % (testeraAccount.name))
    actualServants=node.getServantsArr(testeraAccount.name)
    if actualServants is None:
        cmdError("%s get servants testera" % (ClientName))
        errorExit("Failed to retrieve %s servants" % (testeraAccount.name))
    if len(actualServants) > 0:
        errorExit("FAILURE - %s servants. Expected: [], Actual: %s" % (
            testeraAccount.name, actualServants), raw=True)

    node.waitForTransIdOnNode(transId)

    Print("Get transaction details %s" % (transId))
    transaction=None
    if not enableMongo:
        transaction=node.getTransaction(transId)
    else:
        if amINoon:
            transaction=node.getActionFromDb(transId)
        else:
            transaction=node.getMessageFromDb(transId)
    if transaction is None:
        cmdError("%s get transaction trans_id" % (ClientName))
        errorExit("Failed to retrieve transaction details %s" % (transId))

    typeVal=None
    amountVal=None
    if amINoon:
        debug and Print("Transaction:", transaction)
        if not enableMongo:
            typeVal=  transaction["transaction"]["data"]["actions"][0]["name"]
            amountVal=transaction["transaction"]["data"]["actions"][0]["data"]["quantity"]
            amountVal=int(decimal.Decimal(amountVal.split()[0])*10000)
        else:
            typeVal=  transaction["name"]
            amountVal=transaction["data"]["quantity"]
            amountVal=int(decimal.Decimal(amountVal.split()[0])*10000)
    else:
        if not enableMongo:
            typeVal=  transaction["transaction"]["messages"][0]["type"]
            amountVal=transaction["transaction"]["messages"][0]["data"]["amount"]
        else:
            typeVal=  transaction["type"]
            amountVal=transaction["data"]["amount"]

    if typeVal!= "transfer" or amountVal != 975311:
        errorExit("FAILURE - get transaction trans_id failed: %s %s %s" % (transId, typeVal, amountVal), raw=True)

    Print("Get transactions for account %s" % (testeraAccount.name))
    actualTransactions=node.getTransactionsArrByAccount(testeraAccount.name)
    if actualTransactions is None:
        cmdError("%s get transactions testera" % (ClientName))
        errorExit("Failed to get transactions by account %s" % (testeraAccount.name))
    if transId not in actualTransactions:
        errorExit("FAILURE - get transactions testera failed", raw=True)

    Print("Currency Contract Tests")
    Print("verify no contract in place")
    Print("Get code hash for account %s" % (currencyAccount.name))
    codeHash=node.getAccountCodeHash(currencyAccount.name)
    if codeHash is None:
        cmdError("%s get code currency" % (ClientName))
        errorExit("Failed to get code hash for account %s" % (currencyAccount.name))
    hashNum=int(codeHash, 16)
    if hashNum != 0:
        errorExit("FAILURE - get code currency failed", raw=True)

    wastFile="contracts/currency/currency.wast"
    abiFile="contracts/currency/currency.abi"
    Print("Publish contract")
    trans=node.publishContract(currencyAccount.name, wastFile, abiFile, waitForTransBlock=True)
    if trans is None:
        cmdError("%s set contract currency" % (ClientName))
        errorExit("Failed to publish contract.")

    if not enableMongo:
        Print("Get code hash for account %s" % (currencyAccount.name))
        codeHash=node.getAccountCodeHash(currencyAccount.name)
        if codeHash is None:
            cmdError("%s get code currency" % (ClientName))
            errorExit("Failed to get code hash for account %s" % (currencyAccount.name))
        hashNum=int(codeHash, 16)
        if hashNum == 0:
            errorExit("FAILURE - get code currency failed", raw=True)
    else:
        Print("verify abi is set")
        account=node.getEosAccountFromDb(currencyAccount.name)
        abiName=account["abi"]["structs"][0]["name"]
        abiActionName=None
        if not amINoon:
            abiActionName=account["abi"]["actions"][0]["action_name"]
        else:
            abiActionName=account["abi"]["actions"][0]["name"]
        abiType=account["abi"]["actions"][0]["type"]
        if abiName != "transfer" or abiActionName != "transfer" or abiType != "transfer":
            errorExit("FAILURE - get table currency account failed", raw=True)

    if amINoon:
        Print("push issue action to currency contract")
        contract="currency"
        action="issue"
        data="{\"to\":\"currency\",\"quantity\":\"100000.0000 CUR\"}"
        opts="--permission currency@active"
        trans=node.pushMessage(contract, action, data, opts)

    Print("Verify currency contract has proper initial balance")
    contract="currency"
    table="account"
    row0=node.getTableRow(currencyAccount.name, contract, table, 0)
    if row0 is None:
        cmdError("%s get table currency account" % (ClientName))
        errorExit("Failed to retrieve contract %s table %s" % (contract, table))

    balanceKey="balance"
    keyKey="key"
    if row0[balanceKey] != 1000000000:
        errorExit("FAILURE - get table currency account failed", raw=True)

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
    trans=node.pushMessage(contract, action, data, opts)
    if not trans[0]:
        cmdError("%s push message currency transfer" % (ClientName))
        errorExit("Failed to push message to currency contract")
    transId=testUtils.Node.getTransId(trans[1])

    Print("verify transaction exists")
    if not node.waitForTransIdOnNode(transId):
        cmdError("%s get transaction trans_id" % (ClientName))
        errorExit("Failed to verify push message transaction id.")
    
    Print("read current contract balance")
    contract="currency"
    table="account"
    row0=node.getTableRow(initaAccount.name, contract, table, 0)
    if row0 is None:
        cmdError("%s get table currency account" % (ClientName))
        errorExit("Failed to retrieve contract %s table %s" % (contract, table))

    balanceKey="balance"
    keyKey="key"
    if row0[balanceKey] != 50:
        errorExit("FAILURE - get table currency account failed", raw=True)

    row0=node.getTableRow(currencyAccount.name, contract, table, 0)
    if row0 is None:
        cmdError("%s get table currency account" % (ClientName))
        errorExit("Failed to retrieve contract %s table %s" % (contract, table))

    if row0[balanceKey] != 999999950:
        errorExit("FAILURE - get table currency account failed", raw=True)

    Print("Exchange Contract Tests")
    Print("upload exchange contract")

# TODO Exchange contract currently not working on eos-noon
    if not amINoon:
      wastFile="contracts/exchange/exchange.wast"
      abiFile="contracts/exchange/exchange.abi"
      Print("Publish exchange contract")
      trans=node.publishContract(exchangeAccount.name, wastFile, abiFile, waitForTransBlock=True)
      if trans is None:
          cmdError("%s set contract exchange" % (ClientName))
          errorExit("Failed to publish contract.")

    wastFile="contracts/simpledb/simpledb.wast"
    abiFile="contracts/simpledb/simpledb.abi"
    Print("Setting simpledb contract without simpledb account was causing core dump in %s." % (ClientName))
    Print("Verify %s generates an error, but does not core dump." % (ClientName))
    retMap=node.publishContract("simpledb", wastFile, abiFile, shouldFail=True)
    if retMap is None:
        errorExit("Failed to publish, but should have returned a details map")
    if retMap["returncode"] == 0 or retMap["returncode"] == 139: # 139 SIGSEGV
        errorExit("FAILURE - set contract exchange failed", raw=True)
    else:
        Print("Test successful, %s returned error code: %d" % (ClientName, retMap["returncode"]))

# TODO Currently unable to set producer
    if not amINoon:
        Print("Producer tests")
        trans=node.createProducer(testeraAccount.name, testeraAccount.ownerPublicKey, waitForTransBlock=False)
        if trans is None:
            cmdError("%s create producer" % (ClientName))
            errorExit("Failed to create producer %s" % (testeraAccount.name))

    Print("set permission")
    code="currency"
    pType="transfer"
    requirement="active"
    trans=node.setPermission(testeraAccount.name, code, pType, requirement, waitForTransBlock=True)
    if trans is None:
        cmdError("%s set action permission set" % (ClientName))
        errorExit("Failed to set permission")

# TODO remove failed on eos-noon
    if not amINoon:
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

    Print("Unlocking wallet \"%s\"." % (initaWallet.name))
    if not walletMgr.unlockWallet(initaWallet):
        cmdError("%s wallet unlock inita" % (ClientName))
        errorExit("Failed to unlock wallet %s" % (initaWallet.name))

    # TODO: Approving producers currently not supported
    # approve producer
    # INFO="$(programs/eosioc/eosioc --host $SERVER --port $PORT --wallet-port 8899 set producer inita testera approve)"
    # verifyErrorCode "eosioc approve producer"

    Print("Get account inita")
    account=node.getEosAccount(initaAccount.name)
    if account is None:
        cmdError("%s get account" % (ClientName))
        errorExit("Failed to get account %s" % (initaAccount.name))

    # TODO: Unapproving producers currently not supported
    # unapprove producer
    # INFO="$(programs/eosioc/eosioc --host $SERVER --port $PORT --wallet-port 8899 set producer inita testera unapprove)"
    # verifyErrorCode "eosioc unapprove producer"

    #
    # Proxy
    #
    # not implemented

    Print("Get head block num.")
    currentBlockNum=node.getHeadBlockNum()
    Print("CurrentBlockNum: %d" % (currentBlockNum))
    Print("Request blocks 1-%d" % (currentBlockNum))
    for blockNum in range(1, currentBlockNum+1):
        block=node.getBlock(blockNum, retry=False)
        if block is None:
            cmdError("%s get block" % (ClientName))
            errorExit("mongo get block by num %d" % blockNum)

        if enableMongo:
            blockId=block["block_id"]
            block2=node.getBlockById(blockId, retry=False)
            if block2 is None:
                errorExit("mongo get block by id %s" % blockId)

            # TBD: getTransByBlockId() needs to handle multiple returned transactions
            # trans=node.getTransByBlockId(blockId, retry=False)
            # if trans is not None:
            #     transId=testUtils.Node.getTransId(trans)
            #     trans2=node.getMessageFromDb(transId)
            #     if trans2 is None:
            #         errorExit("mongo get messages by transaction id %s" % (transId))

            
    Print("Request invalid block numbered %d" % (currentBlockNum+1000))
    block=node.getBlock(currentBlockNum+1000, silentErrors=True, retry=False)
    if block is not None:
        errorExit("ERROR: Received block where not expected")
    else:
        Print("Success: No such block found")

    if localTest:
        p = re.compile('Assert')
        errFileName="tn_data_00/stderr.txt"
        with open(errFileName) as errFile:
            for line in errFile:
                if p.search(line):
                   errorExit("FAILURE - Assert in tn_data_00/stderr.txt")
        
    testSuccessful=True
    Print("END")
finally:
    if not testSuccessful and dumpErrorDetails:
        cluster.dumpErrorDetails()
        walletMgr.dumpErrorDetails()
        Print("== Errors see above ==")

    if killEosInstances:
        Print("Shut down the cluster.")
        cluster.killall()
        if testSuccessful and not keepLogs:
            Print("Cleanup cluster data.")
            cluster.cleanup()

    if killWallet:
        Print("Shut down the wallet.")
        walletMgr.killall()
        if testSuccessful and not keepLogs:
            Print("Cleanup wallet data.")
            walletMgr.cleanup()

exit(0)
