#!/usr/bin/env python3

from testUtils import Account
from testUtils import Utils
from Cluster import Cluster, PFSetupPolicy
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper
from TestHelper import AppArgs

import decimal
import re
import json
import os
import copy
import math
import time

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

args = TestHelper.parse_args({"--host","--port","--prod-count","--defproducera_prvt_key","--defproducerb_prvt_key"
                              ,"--dump-error-details","--dont-launch","--keep-logs","-v","--leave-running","--only-bios","--clean-run"
                              ,"--sanity-test","--wallet-port", "--alternate-version-labels-file"})
server=args.host
port=args.port
debug=args.v
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
alternateVersionLabelsFile=args.alternate_version_labels_file

Utils.Debug=debug
localTest=True if server == TestHelper.LOCAL_HOST else False
cluster=Cluster(host=server, 
                port=port, 
                walletd=True,
                defproduceraPrvtKey=defproduceraPrvtKey, 
                defproducerbPrvtKey=defproducerbPrvtKey)
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

    if localTest and not dontLaunch:
        cluster.killall(allInstances=killAll)
        cluster.cleanup()
        Print("Stand up cluster")
        specificExtraNodeosArgs = {}
        associatedNodeLabels = {}
        if len(specificExtraNodeosArgs):
            specificExtraNodeosArgs[prodCount - 2] = " --backing-store=rocksdb"
            specificExtraNodeosArgs[prodCount - 1] = " --backing-store=rocksdb"

        for i in range( int(prodCount / 2) ):
            associatedNodeLabels[str(i)] = "209"
        
        if cluster.launch(totalNodes=prodCount, 
                          prodCount=1, 
                          onlyBios=onlyBios, 
                          dontBootstrap=dontBootstrap,
                          pfSetupPolicy=PFSetupPolicy.PREACTIVATE_FEATURE_ONLY,
                          specificExtraNodeosArgs=specificExtraNodeosArgs,
                          alternateVersionLabelsFile=alternateVersionLabelsFile,
                          associatedNodeLabels=associatedNodeLabels) is False:
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

    accounts=Cluster.createAccountKeys(2)
    if accounts is None:
        errorExit("FAILURE - create keys")
    testeraAccount=accounts[0]
    testeraAccount.name="testerxxxxxa"
    testerbAccount=accounts[1]
    testerbAccount.name="testerxxxxxb"

    testWalletName="test"
    Print("Creating wallet \"%s\"" % (testWalletName))
    walletAccounts=copy.deepcopy(cluster.defProducerAccounts)
    if dontLaunch:
        del walletAccounts["eosio"]
    testWallet = walletMgr.create(testWalletName, walletAccounts.values())

    Print("Wallet \"%s\" password=%s." % (testWalletName, testWallet.password.encode("utf-8")))

    all_acc = accounts + list( cluster.defProducerAccounts.values() )
    for account in all_acc:
        Print("Importing keys for account %s into wallet %s." % (account.name, testWallet.name))
        if not walletMgr.importKey(account, testWallet):
            cmdError("%s wallet import" % (ClientName))
            errorExit("Failed to import key for account %s" % (account.name))
    
    node=cluster.getNode(0)

    Print("Create new account %s via %s" % (testeraAccount.name, cluster.defproduceraAccount.name))
    transId=node.createInitializeAccount(testeraAccount, cluster.defproduceraAccount, stakedDeposit=0, waitForTransBlock=False, exitOnError=True)

    Print("Create new account %s via %s" % (testerbAccount.name, cluster.defproduceraAccount.name))
    transId=node.createInitializeAccount(testerbAccount, cluster.defproduceraAccount, stakedDeposit=0, waitForTransBlock=False, exitOnError=True)

    Print("Validating accounts after user accounts creation")
    accounts=[testeraAccount, testerbAccount]
    cluster.validateAccounts(accounts)

    for i in range(2):
        if i == 2:
            node = cluster.getNode(prodCount - 1)
        
        transferAmount="{0}.0 {1}".format(i, CORE_SYMBOL)
        Print("Transfer funds %s from account %s to %s" % (transferAmount, cluster.defproduceraAccount.name, testeraAccount.name))
        trx = node.transferFunds(cluster.defproduceraAccount, testeraAccount, transferAmount, "test transfer", dontSend=True)

        cmdDesc = "convert pack_transaction"
        cmd     = "%s --pack-action-data '%s'" % (cmdDesc, json.dumps(trx))
        exitMsg = "failed to pack transaction: %s" % (trx)
        packedTrx = node.processCleosCmd(cmd, cmdDesc, silentErrors=False, exitOnError=True, exitMsg=exitMsg)

        packed_trx_param = packedTrx["packed_trx"]
        if packed_trx_param is None:
            cmdError("packed_trx is None. Json: %s" % (packedTrx))
            errorExit("Can't find packed_trx in packed json")
        
        #adding random trailing data
        packedTrx["packed_trx"] = packed_trx_param + "00000000"

        exitMsg = "failed to send packed transaction: %s" % (packedTrx)
        node.processCurlCmd("chain", "send_transaction", json.dumps(packedTrx), silentErrors=False, exitOnError=True, exitMsg=exitMsg)

    info = node.getInfo(exitOnError=True)
    oldLIB = int(info["last_irreversible_block_num"])

    blocksGap = (12 * (2 * math.floor(2 * prodCount / 3)) + 12) * 1.3 #adding 30% more to be on the safe side
    Print("Running nodeos idle for maximum %d seconds" % (blocksGap))
    step = 10
    newLIB = 0
    for i in range(step):
        sleepTime = int(blocksGap / step)
        time.sleep(sleepTime)
        info = node.getInfo(exitOnError=True)
        newLIB = int(info["last_irreversible_block_num"])

        if newLIB > oldLIB + 1:
            Print("LIB advanced. new LIB: %d old LIB: %d" % (newLIB, oldLIB))
            Print("sleep time : %d" %( i * sleepTime ))
            break

    assert newLIB > oldLIB + 1, Print("ERROR: LIB doesn't move %d" % (newLIB))

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)
