#!/usr/bin/env python3

from core_symbol import CORE_SYMBOL
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from TestHelper import TestHelper
from testUtils import Utils
import testUtils
import time

import decimal
import math
import re

Print=Utils.Print
errorExit=Utils.errorExit

class NamedAccounts:

    def __init__(self, cluster, numAccounts):
        Print("NamedAccounts %d" % (numAccounts))
        self.numAccounts=numAccounts
        self.accounts=cluster.createAccountKeys(numAccounts)
        if self.accounts is None:
            errorExit("FAILURE - create keys")
        accountNum = 0
        for account in self.accounts:
            Print("NamedAccounts Name for %d" % (accountNum))
            account.name=self.setName(accountNum)
            accountNum+=1

    def setName(self, num):
        retStr="test"
        digits=[]
        maxDigitVal=5
        maxDigits=8
        temp=num
        while len(digits) < maxDigits:
            digit=(num % maxDigitVal)+1
            num=int(num/maxDigitVal)
            digits.append(digit)

        digits.reverse()
        for digit in digits:
            retStr=retStr+str(digit)

        Print("NamedAccounts Name for %d is %s" % (temp, retStr))
        return retStr

###############################################################
# nodeos_voting_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"})
Utils.Debug=args.v
totalNodes=4
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
killAll=args.clean_run

walletMgr=WalletMgr(True)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName="keosd"
ClientName="cleos"

try:
    TestHelper.printSystemInfo("BEGIN")

    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    Print("Stand up cluster")
    minRAMFlag="--chain-state-db-guard-size-mb"
    minRAMValue=1002
    maxRAMFlag="--chain-state-db-size-mb"
    maxRAMValue=1010
    extraNodeosArgs=" %s %d %s %d " % (minRAMFlag, minRAMValue, maxRAMFlag, maxRAMValue)
    if cluster.launch(onlyBios=False, dontKill=dontKill, pnodes=totalNodes, totalNodes=totalNodes, totalProducers=totalNodes, extraNodeosArgs=extraNodeosArgs, useBiosBootFile=False) is False:
        Utils.cmdError("launcher")
        errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    Print("creating accounts")
    namedAccounts=NamedAccounts(cluster,10)
    accounts=namedAccounts.accounts

    testWalletName="test"

    Print("Creating wallet \"%s\"." % (testWalletName))
    walletMgr.killall(allInstances=killAll)
    walletMgr.cleanup()
    if walletMgr.launch() is False:
        Utils.cmdError("%s" % (WalletdName))
        errorExit("Failed to stand up eos walletd.")

    testWallet=walletMgr.create(testWalletName, [cluster.eosioAccount])

    for _, account in cluster.defProducerAccounts.items():
        walletMgr.importKey(account, testWallet, ignoreDupKeyWarning=True)

    Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))

    nodes=[]
    nodes.append(cluster.getNode(0))
    nodes.append(cluster.getNode(1))
    nodes.append(cluster.getNode(2))
    nodes.append(cluster.getNode(3))


    for account in accounts:
        walletMgr.importKey(account, testWallet)

    # create accounts via eosio as otherwise a bid is needed
    for account in accounts:
        Print("Create new account %s via %s" % (account.name, cluster.eosioAccount.name))
        trans=nodes[0].createInitializeAccount(account, cluster.eosioAccount, stakedDeposit=500000, waitForTransBlock=False, stakeNet=50000, stakeCPU=50000, buyRAM=50000, exitOnError=True)
        transferAmount="70000000.0000 {0}".format(CORE_SYMBOL)
        Print("Transfer funds %s from account %s to %s" % (transferAmount, cluster.eosioAccount.name, account.name))
        nodes[0].transferFunds(cluster.eosioAccount, account, transferAmount, "test transfer")
        trans=nodes[0].delegatebw(account, 1000000.0000, 68000000.0000, exitOnError=True)

    contractAccount=cluster.createAccountKeys(1)[0]
    contractAccount.name="contracttest"
    walletMgr.importKey(contractAccount, testWallet)
    Print("Create new account %s via %s" % (contractAccount.name, cluster.eosioAccount.name))
    trans=nodes[0].createInitializeAccount(contractAccount, cluster.eosioAccount, stakedDeposit=500000, waitForTransBlock=False, stakeNet=50000, stakeCPU=50000, buyRAM=50000, exitOnError=True)
    transferAmount="90000000.0000 {0}".format(CORE_SYMBOL)
    Print("Transfer funds %s from account %s to %s" % (transferAmount, cluster.eosioAccount.name, contractAccount.name))
    nodes[0].transferFunds(cluster.eosioAccount, contractAccount, transferAmount, "test transfer")
    trans=nodes[0].delegatebw(contractAccount, 1000000.0000, 88000000.0000, exitOnError=True)

    contractDir="contracts/integration_test"
    wasmFile="integration_test.wasm"
    abiFile="integration_test.abi"
    Print("Publish contract")
    trans=nodes[0].publishContract(contractAccount.name, contractDir, wasmFile, abiFile, waitForTransBlock=True)
    if trans is None:
        Utils.cmdError("%s set contract %s" % (ClientName, contractAccount.name))
        errorExit("Failed to publish contract.")

    contract=contractAccount.name
    Print("push create action to %s contract" % (contract))
    action="store"
    numAmount=5000
    keepProcessing=True
    count=0
    while keepProcessing:
        numAmount+=1
        for fromIndex in range(namedAccounts.numAccounts):
            count+=1
            toIndex=fromIndex+1
            if toIndex==namedAccounts.numAccounts:
                toIndex=0
            fromAccount=accounts[fromIndex]
            toAccount=accounts[toIndex]
            data="{\"from\":\"%s\",\"to\":\"%s\",\"num\":%d}" % (fromAccount.name, toAccount.name, numAmount)
            opts="--permission %s@active --permission %s@active --expiration 90" % (contract, fromAccount.name)
            try:
                trans=nodes[0].pushMessage(contract, action, data, opts)
                if trans is None or not trans[0]:
                    Print("Failed to push create action to eosio contract. sleep for 60 seconds")
                    time.sleep(60)
                time.sleep(1)
            except TypeError as ex:
                keepProcessing=False
                break

    #spread the actions to all accounts, to use each accounts tps bandwidth
    fromIndexStart=fromIndex+1 if fromIndex+1<namedAccounts.numAccounts else 0

    if count < 5 or count > 15:
        strMsg="little" if count < 15 else "much"
        Utils.cmdError("Was able to send %d store actions which was too %s" % (count, strMsg))
        errorExit("Incorrect number of store actions sent")

    # Make sure all the nodes are shutdown (may take a little while for this to happen, so making multiple passes)
    allDone=False
    count=0
    while not allDone:
        allDone=True
        for node in nodes:
            if node.verifyAlive():
                allDone=False
        if not allDone:
            time.sleep(5)
        if ++count>5:
            Utils.cmdError("All Nodes should have died")
            errorExit("Failure - All Nodes should have died")

    Print("relaunch nodes with new capacity")
    addOrSwapFlags={}
    numNodes=len(nodes)
    maxRAMValue+=2
    currentMinimumMaxRAM=maxRAMValue
    enabledStaleProduction=False
    for i in range(numNodes):
        addOrSwapFlags[maxRAMFlag]=str(maxRAMValue)
        #addOrSwapFlags["--max-irreversible-block-age"]=str(-1)
        nodeIndex=numNodes-i-1
        if not enabledStaleProduction:
            addOrSwapFlags["--enable-stale-production"]=""   # just enable stale production for the first node
            enabledStaleProduction=True
        if not nodes[nodeIndex].relaunch(nodeIndex, "", newChain=False, addOrSwapFlags=addOrSwapFlags):
            Utils.cmdError("Failed to restart node0 with new capacity %s" % (maxRAMValue))
            errorExit("Failure - Node should have restarted")
        addOrSwapFlags={}
        maxRAMValue=currentMinimumMaxRAM+30

    time.sleep(20)
    for i in range(numNodes):
        if not nodes[i].verifyAlive():
            Utils.cmdError("Node %d should be alive" % (i))
            errorExit("Failure - All Nodes should be alive")

    # get all the nodes to get info, so reported status (on error) reflects their current state
    Print("push more actions to %s contract" % (contract))
    cluster.getInfos()
    action="store"
    keepProcessing=True
    count=0
    while keepProcessing and count < 40:
        Print("Send %s" % (action))
        numAmount+=1
        for fromIndexOffset in range(namedAccounts.numAccounts):
            count+=1
            fromIndex=fromIndexStart+fromIndexOffset
            if fromIndex>=namedAccounts.numAccounts:
                fromIndex-=namedAccounts.numAccounts 
            toIndex=fromIndex+1
            if toIndex==namedAccounts.numAccounts:
                toIndex=0
            fromAccount=accounts[fromIndex]
            toAccount=accounts[toIndex]
            data="{\"from\":\"%s\",\"to\":\"%s\",\"num\":%d}" % (fromAccount.name, toAccount.name, numAmount)
            opts="--permission %s@active --permission %s@active --expiration 90" % (contract, fromAccount.name)
            try:
                trans=nodes[0].pushMessage(contract, action, data, opts)
                if trans is None or not trans[0]:
                    Print("Failed to push create action to eosio contract. sleep for 60 seconds")
                    time.sleep(60)
                time.sleep(1)
            except TypeError as ex:
                Print("Failed to send %s" % (action))

            if not nodes[len(nodes)-1].verifyAlive():
                keepProcessing=False
                break

    if keepProcessing:
        Utils.cmdError("node[%d] never shutdown" % (numNodes-1))
        errorExit("Failure - Node should be shutdown")

    for i in range(numNodes):
        # only the last node should be dead
        if not nodes[i].verifyAlive() and i<numNodes-1:
            Utils.cmdError("Node %d should be alive" % (i))
            errorExit("Failure - Node should be alive")

    Print("relaunch node with even more capacity")
    addOrSwapFlags={}

    time.sleep(10)
    maxRAMValue=currentMinimumMaxRAM+5
    currentMinimumMaxRAM=maxRAMValue
    addOrSwapFlags[maxRAMFlag]=str(maxRAMValue)
    if not nodes[len(nodes)-1].relaunch(nodeIndex, "", newChain=False, addOrSwapFlags=addOrSwapFlags):
        Utils.cmdError("Failed to restart node %d with new capacity %s" % (numNodes-1, maxRAMValue))
        errorExit("Failure - Node should have restarted")
    addOrSwapFlags={}

    time.sleep(10)
    allDone=True
    for node in nodes:
        if not node.verifyAlive():
            Utils.cmdError("All Nodes should be alive")
            errorExit("Failure - All Nodes should be alive")

    time.sleep(20)
    Print("Send 1 more action to every node")
    numAmount+=1
    for fromIndexOffset in range(namedAccounts.numAccounts):
        # just sending one node to each
        if fromIndexOffset>=len(nodes):
           break
        fromIndex=fromIndexStart+fromIndexOffset
        if fromIndex>=namedAccounts.numAccounts:
            fromIndex-=namedAccounts.numAccounts 
        toIndex=fromIndex+1
        if toIndex==namedAccounts.numAccounts:
            toIndex=0
        fromAccount=accounts[fromIndex]
        toAccount=accounts[toIndex]
        node=nodes[fromIndexOffset]
        data="{\"from\":\"%s\",\"to\":\"%s\",\"num\":%d}" % (fromAccount.name, toAccount.name, numAmount)
        opts="--permission %s@active --permission %s@active --expiration 90" % (contract, fromAccount.name)
        try:
            trans=nodes[0].pushMessage(contract, action, data, opts)
            if trans is None or not trans[0]:
                Print("Failed to push create action to eosio contract. sleep for 60 seconds")
                time.sleep(60)
                continue
            time.sleep(1)
        except TypeError as ex:
            Utils.cmdError("Failed to send %s action to node %d" % (fromAccount, fromIndexOffset, action))
            errorExit("Failure - send %s action should have succeeded" % (action))

    time.sleep(10)
    Print("Check nodes are alive")
    allDone=True
    for node in nodes:
        if not node.verifyAlive():
            Utils.cmdError("All Nodes should be alive")
            errorExit("Failure - All Nodes should be alive")

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
