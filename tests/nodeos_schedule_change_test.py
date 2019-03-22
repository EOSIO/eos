#!/usr/bin/env python3

from testUtils import Utils
import testUtils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from TestHelper import TestHelper

import decimal
import math
import re
import time

###############################################################
# nodeos_schedule_change_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################
class ProducerToNode:
    map={}

    @staticmethod
    def populate(node, num):
        for prod in node.producers:
            ProducerToNode.map[prod]=num
            Utils.Print("Producer=%s for nodeNum=%s" % (prod,num))

def isValidBlockProducer(prodsActive, blockNum, node):
    blockProducer=node.getBlockProducerByNum(blockNum)
    if blockProducer not in prodsActive:
        return False
    return prodsActive[blockProducer]

def validBlockProducer(prodsActive, prodsSeen, blockNum, node):
    blockProducer=node.getBlockProducerByNum(blockNum)
    if blockProducer not in prodsActive:
        Utils.cmdError("unexpected block producer %s at blockNum=%s" % (blockProducer,blockNum))
        Utils.errorExit("Failed because of invalid block producer")
    if not prodsActive[blockProducer]:
        Utils.cmdError("block producer %s for blockNum=%s not elected, belongs to node %s" % (blockProducer, blockNum, ProducerToNode.map[blockProducer]))
        Utils.errorExit("Failed because of incorrect block producer")
    prodsSeen[blockProducer]=True

def setActiveProducers(cluster, node, prodsActive):
    # construct action data
    # jsonVersion = "'{ \"version\": \"1.1\", \"producers\": [" # JSON head
    jsonVersion = "'{\"schedule\": [" # JSON head
    jsonProdList = "" # JSON body
    # jsonTail = "]}'" # JSON tail
    for producer in node.producers:
        # jsonProdList += "{\"producer_name\": \"" + producer + "\",\"block_signing_key\": \"" + cluster.defProducerAccounts[ node0.producers[0] ].activePublicKey + "\"}, "
        jsonProdList += "[\"" + producer + "\", \"" + cluster.defProducerAccounts[ node0.producers[0] ].activePublicKey + "\"], "
        prodsActive[producer]=prod in node.producers # update local list of producers
    jsonProdList = jsonProdList[:-2] # remove last comma
    # change block producers using bios
    txnPermissions = "-p " + cluster.eosioAccount.name # @active by default
    txnAccount = cluster.eosioAccount.name
    txnAction = "setprods"
    # txnData = jsonVersion + jsonProdList + jsonTail # construct full JSON payload
    txnData = "[[" + jsonProdList + "]]"
    return cluster.biosNode.pushMessage(account = txnAccount, action = txnAction, data = txnData, opts = txnPermissions)

def verifyProductionRounds(trans, node, prodsActive, rounds):
    # blockNum=node.getNextCleanProductionCycle(trans)
    blockNum = node.getHeadBlockNum()
    Utils.Print("Validating blockNum=%s" % (blockNum))

    temp=Utils.Debug
    Utils.Debug=False
    Utils.Print("FIND VALID BLOCK PRODUCER")
    blockProducer=node.getBlockProducerByNum(blockNum)
    lastBlockProducer=blockProducer
    adjust=False
    while not isValidBlockProducer(prodsActive, blockNum, node):
        adjust=True
        blockProducer=node.getBlockProducerByNum(blockNum)
        if lastBlockProducer!=blockProducer:
            Utils.Print("blockProducer=%s for blockNum=%s is for node=%s" % (blockProducer, blockNum, ProducerToNode.map[blockProducer]))
        lastBlockProducer=blockProducer
        blockNum+=1

    Utils.Print("VALID BLOCK PRODUCER")
    saw=0
    sawHigh=0
    startingFrom=blockNum
    doPrint=0
    invalidCount=0
    while adjust:
        invalidCount+=1
        if lastBlockProducer==blockProducer:
            saw+=1;
        else:
            if saw>=12:
                startingFrom=blockNum
                if saw>12:
                    Utils.Print("ERROR!!!!!!!!!!!!!!      saw=%s, blockProducer=%s, blockNum=%s" % (saw,blockProducer,blockNum))
                break
            else:
                if saw > sawHigh:
                    sawHigh = saw
                    Utils.Print("sawHigh=%s" % (sawHigh))
                if doPrint < 5:
                    doPrint+=1
                    Utils.Print("saw=%s, blockProducer=%s, blockNum=%s" % (saw,blockProducer,blockNum))
                lastBlockProducer=blockProducer
                saw=1
        blockProducer=node.getBlockProducerByNum(blockNum)
        blockNum+=1

    if adjust:
        blockNum-=1

    Utils.Print("ADJUSTED %s blocks" % (invalidCount-1))

    prodsSeen=None
    reportFirstMissedBlock=False
    Utils.Print("Verify %s complete rounds of all producers producing" % (rounds))
    for i in range(0, rounds):
        prodsSeen={}
        lastBlockProducer=None
        for j in range(0, 21):
            # each new set of 12 blocks should have a different blockProducer 
            if lastBlockProducer is not None and lastBlockProducer==node.getBlockProducerByNum(blockNum):
                Utils.cmdError("expected blockNum %s to be produced by any of the valid producers except %s" % (blockNum, lastBlockProducer))
                Utils.errorExit("Failed because of incorrect block producer order")

            # make sure that the next set of 12 blocks all have the same blockProducer
            lastBlockProducer=node.getBlockProducerByNum(blockNum)
            for k in range(0, 12):
                validBlockProducer(prodsActive, prodsSeen, blockNum, node1)
                blockProducer=node.getBlockProducerByNum(blockNum)
                if lastBlockProducer!=blockProducer:
                    if not reportFirstMissedBlock:
                        printStr=""
                        newBlockNum=blockNum-18
                        for l in range(0,36):
                            printStr+="%s" % (newBlockNum)
                            printStr+=":"
                            newBlockProducer=node.getBlockProducerByNum(newBlockNum)
                            printStr+="%s" % (newBlockProducer)
                            printStr+="  "
                            newBlockNum+=1
                        Utils.Print("NOTE: expected blockNum %s (started from %s) to be produced by %s, but produded by %s: round=%s, prod slot=%s, prod num=%s - %s" % (blockNum, startingFrom, lastBlockProducer, blockProducer, i, j, k, printStr))
                    reportFirstMissedBlock=True
                    break
                blockNum+=1

    # make sure that we have seen all 21 producers
    prodsSeenKeys=prodsSeen.keys()
    if len(prodsSeenKeys)!=21:
        Utils.cmdError("only saw %s producers of expected 21. At blockNum %s only the following producers were seen: %s" % (len(prodsSeenKeys), blockNum, ",".join(prodsSeenKeys)))
        Utils.errorExit("Failed because of missing block producers")

    Utils.Debug=temp

### environment variables ###
Print=Utils.Print
errorExit=Utils.errorExit
from core_symbol import CORE_SYMBOL
args = TestHelper.parse_args({"--prod-count", "--dump-error-details", "--keep-logs", "-v", "--leave-running", "--clean-run", "--p2p-plugin", "--wallet-port"})
Utils.Debug=args.v
totalNodes=4
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
prodCount=args.prod_count
killAll=args.clean_run
p2pPlugin=args.p2p_plugin
walletPort=args.wallet_port
walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill
WalletdName=Utils.EosWalletName
ClientName="cleos"
### test body ###
try:
    TestHelper.printSystemInfo("BEGIN")
    # prepare clean environment
    cluster.setWalletMgr(walletMgr)
    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    # start nodeos instances
    Print("Stand up cluster")
    if cluster.launch(prodCount=prodCount, onlyBios=False, pnodes=totalNodes, totalNodes=totalNodes, totalProducers=totalNodes*21, p2pPlugin=p2pPlugin, useBiosBootFile=False, minimize=True) is False:
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    # check accounts exist using cleos
    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)
    # create keys and names
    accounts=cluster.createAccountKeys(5)
    if accounts is None:
        Utils.errorExit("FAILURE - create keys")
    accounts[0].name="tester111111"
    accounts[1].name="tester222222"
    accounts[2].name="tester333333"
    accounts[3].name="tester444444"
    accounts[4].name="tester555555"
    # create wallet
    testWalletName="test"
    Print("Creating wallet \"%s\"." % (testWalletName))
    testWallet=walletMgr.create(testWalletName, [cluster.eosioAccount,accounts[0],accounts[1],accounts[2],accounts[3],accounts[4]])
    # debug statements
    Print("##### DEBUG 1 #####") # DEBUG
    Print("cluster.eosioAccount") # DEBUG
    Print(cluster.eosioAccount) # DEBUG
    Print("cluster.eosioAccount.name") #DEBUG
    Print(cluster.eosioAccount.name) # DEBUG
    Print("cluster.eosioAccount.ownerPublicKey") # DEBUG
    Print(cluster.eosioAccount.ownerPublicKey) # DEBUG
    Print("cluster.eosioAccount.ownerPrivateKey") # DEBUG
    Print(cluster.eosioAccount.ownerPrivateKey) # DEBUG
    Print("cluster.eosioAccount.activePublicKey") # DEBUG
    Print(cluster.eosioAccount.activePublicKey) # DEBUG
    Print("cluster.eosioAccount.activePrivateKey") # DEBUG
    Print(cluster.eosioAccount.activePrivateKey) # DEBUG
    Print("=====") # DEBUG
    Print("accounts[0].name") # DEBUG
    Print(accounts[0].name) # DEBUG
    Print("accounts[0].activePublicKey") # DEBUG
    Print(accounts[0].activePublicKey) # DEBUG
    Print() # DEBUG
    Print("accounts[1].name") # DEBUG
    Print(accounts[1].name) # DEBUG
    Print("accounts[1].activePublicKey") # DEBUG
    Print(accounts[1].activePublicKey) # DEBUG
    Print() # DEBUG
    Print("accounts[2].name") # DEBUG
    Print(accounts[2].name) # DEBUG
    Print("accounts[2].activePublicKey") # DEBUG
    Print(accounts[2].activePublicKey) # DEBUG
    Print() # DEBUG
    Print("accounts[3].name") # DEBUG
    Print(accounts[3].name) # DEBUG
    Print("accounts[3].activePublicKey") # DEBUG
    Print(accounts[3].activePublicKey) # DEBUG
    Print() # DEBUG
    Print("accounts[4].name") # DEBUG
    Print(accounts[4].name) # DEBUG
    Print("accounts[4].activePublicKey") # DEBUG
    Print(accounts[4].activePublicKey) # DEBUG
    Print("###################") # DEBUG
    # import keys into wallet corresponding to test nodes
    for _, account in cluster.defProducerAccounts.items():
        walletMgr.importKey(account, testWallet, ignoreDupKeyWarning=True)
        Print(" #### account") # DEBUG
        Print(account) # DEBUG
        Print(account.ownerPublicKey) # DEBUG
        Print(account.ownerPrivateKey) # DEBUG
        Print(account.activePublicKey) # DEBUG
        Print(account.activePrivateKey) # DEBUG

    Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))
    # register block producers with system contract
    for i in range(0, totalNodes):
        node=cluster.getNode(i)
        node.producers=Cluster.parseProducers(i)
        for prod in node.producers:
            trans=node.regproducer(cluster.defProducerAccounts[prod], "http::/mysite.com", 0, waitForTransBlock=False, exitOnError=True)

    node0=cluster.getNode(0)
    node1=cluster.getNode(1)
    node2=cluster.getNode(2)
    node3=cluster.getNode(3)

    Print("##### DEBUG 2 #####") # DEBUG
    Print("cluster.defProducerAccounts")
    Print(cluster.defProducerAccounts)
    Print("===== Node 0 =====")
    Print(node0.producers)
    for prod in node0.producers:
        Print(prod)
        Print(cluster.defProducerAccounts[prod].activePublicKey)
    Print("===== Node 1 =====")
    Print(node1.producers)
    for prod in node1.producers:
        Print(prod)
        Print(cluster.defProducerAccounts[prod].activePublicKey)
    Print("===== Node 2 =====")
    Print(node2.producers)
    for prod in node2.producers:
        Print(prod)
        Print(cluster.defProducerAccounts[prod].activePublicKey)
    Print("===== Node 3 =====")
    Print(node3.producers)
    for prod in node3.producers:
        Print(prod)
        Print(cluster.defProducerAccounts[prod].activePublicKey)
    Print("===== walletMgr =====")
    Print("walletMgr.getKeys(testWallet)")
    Print(walletMgr.getKeys(testWallet))
    Print("###################") # DEBUG

    node=node0
    # create accounts via eosio as otherwise a bid is needed
    txnAccount = cluster.eosioAccount.name
    txnPermissions = "-p " + txnAccount + "@active"
    # for account in accounts:
        # Print('Create new account "%s" using account "%s"' % (account.name, cluster.eosioAccount.name))
        # (txnSuccess, txnID) = cluster.biosNode.pushMessage(account = txnAccount, action = "newaccount", data = '[["eosio", "' + account.name + '", "' + account.ownerPublicKey + '", "' + account.activePublicKey + '"]]', opts = txnPermissions)
        # (txnSuccess, txnID) = cluster.biosNode.pushMessage(account = txnAccount, action = "newaccount", data = ' ' + account.name + ' ' + account.ownerPublicKey + ' ' + account.activePublicKey + '"]', opts = txnPermissions)
        # if not txnSuccess:
        #    Utils.errorExit('Failed to create account "' + account + '" using account "' + txnAccount + '" !')
        # trans=node.createInitializeAccount(account, cluster.eosioAccount, stakedDeposit=0, waitForTransBlock=False, stakeNet=1000, stakeCPU=1000, buyRAM=1000, exitOnError=True)
        # transferAmount="100000000.0000 {0}".format(CORE_SYMBOL)
        # Print("Transfer funds %s from account %s to %s" % (transferAmount, cluster.eosioAccount.name, account.name))
        # node.transferFunds(cluster.eosioAccount, account, transferAmount, "test transfer")
        # trans=node.delegatebw(account, 20000000.0000, 20000000.0000, waitForTransBlock=True, exitOnError=True)
    
    # containers for tracking producers
    prodsActive={}
    for i in range(0, 4):
        node=cluster.getNode(i)
        ProducerToNode.populate(node, i)
        for prod in node.producers:
            prodsActive[prod]=False
    # set block producer schedule to use node 1
    # $ cleos push action setalimits '["eosio", -1, -1, -1]' -p eosio
    (txnSuccess, txnID) = cluster.biosNode.pushMessage(account = txnAccount, action = "setalimits", data = '["eosio", -1, -1, -1]', opts = txnPermissions)
    if not txnSuccess:
        Utils.errorExit("Failed setalimits!")
    # $ cleos push action reqauth '["eosio"]' -p eosio
    (txnSuccess, txnID) = cluster.biosNode.pushMessage(account = txnAccount, action = "reqauth", data = '["eosio"]', opts = txnPermissions)
    if not txnSuccess:
        Utils.Print("##### system contract!?!")
        # Utils.errorExit("Failed reqauth!")
    else:
        Utils.Print("##### bios contract!!!")
    Print("###### Begin Schedule Change 1") # DEBUG
    (txnSuccess, txnID) = setActiveProducers(cluster, node1, prodsActive)
    if not txnSuccess:
        Utils.errorExit("Failed to set initial block producer schedule to node1's block producers using bios account!")
    Print("###### End Schedule Change 1") # DEBUG
    delay = math.ceil(2*(2/3)*21*12*2*0.5) # LIB proposed-approved rounds * min BP approval * #BPs * blocks/BP * proposed>pending>active * 500ms block time (seconds)
    time.sleep(delay) # wait for schedule to be implemented
    verifyProductionRounds(txnID, node2, prodsActive, 2)
    # change block producer schedule to node2
    Print("###### Begin Schedule Change 2") # DEBUG
    (txnSuccess, txnID) = setActiveProducers(cluster, node2, prodsActive)
    if not txnSuccess:
        Utils.errorExit("Failed to change block producer schedule from node1 to node2 using bios account!")
    Print("###### End Schedule Change 2") # DEBUG
    time.sleep(delay) # wait for schedule to be implemented
    verifyProductionRounds(trans, node1, prodsActive, 2)
    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)
exit(0)
