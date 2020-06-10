#!/usr/bin/env python3
import json
import os
import shutil
import time
import unittest

from testUtils import Utils
from testUtils import Account
from Cluster import Cluster
from TestHelper import TestHelper
from Node import Node
from WalletMgr import WalletMgr
from core_symbol import CORE_SYMBOL

class TraceApiPluginTest(unittest.TestCase):
    sleep_s = 1
    cluster=Cluster(walletd=True, defproduceraPrvtKey=None)
    walletMgr=WalletMgr(True, TestHelper.DEFAULT_PORT, TestHelper.LOCAL_HOST, TestHelper.DEFAULT_WALLET_PORT, TestHelper.LOCAL_HOST)
    accounts = []
    account_names = ["alice", "bob", "charlie"]

    # kill nodeos and keosd and clean up dir
    def cleanEnv(self) :
        self.cluster.killall(allInstances=True)
        self.cluster.cleanup()
        self.walletMgr.killall(allInstances=True)
        self.walletMgr.cleanup()

    # start keosd and nodeos
    def startEnv(self) :
        self.walletMgr.launch()
#        self.cluster.setWalletMgr(self.walletMgr)
        self.cluster.launch(totalNodes=1, enableTrace=True)
        testWalletName="testwallet"
        testWallet=self.walletMgr.create(testWalletName, [self.cluster.eosioAccount, self.cluster.defproduceraAccount])
        self.cluster.validateAccounts(None)
        self.accounts=Cluster.createAccountKeys(len(self.account_names))
        node = self.cluster.getNode(0)
        for idx in range(len(self.account_names)):
            self.accounts[idx].name =  self.account_names[idx]
            self.walletMgr.importKey(self.accounts[idx], testWallet)
        for account in self.accounts:
            node.createInitializeAccount(account, self.cluster.eosioAccount, buyRAM=1000000, stakedDeposit=5000000, waitForTransBlock=True, exitOnError=True)
        time.sleep(self.sleep_s)

    def test_TraceApi(self) :
        node = self.cluster.getNode(0)
        for account in self.accounts:
            self.assertIsNotNone(node.verifyAccount(account))

        expectedAmount = Node.currencyIntToStr(5000000, CORE_SYMBOL)
        account_balances = []
        for account in self.accounts:
            amount = node.getAccountEosBalanceStr(account.name)
            self.assertEqual(amount, expectedAmount)
            account_balances.append(amount)

        xferAmount = Node.currencyIntToStr(123456, CORE_SYMBOL)
        trans = node.transferFunds(self.accounts[0], self.accounts[1], xferAmount, "test transfer a->b")
        transId = Node.getTransId(trans)
        blockNum = Node.getTransBlockNum(trans)

        self.assertEqual(node.getAccountEosBalanceStr(self.accounts[0].name), Utils.deduceAmount(expectedAmount, xferAmount))
        self.assertEqual(node.getAccountEosBalanceStr(self.accounts[1].name), Utils.addAmount(expectedAmount, xferAmount))
        time.sleep(self.sleep_s)
        base_cmd_str = ("curl http://%s:%s/v1/") % (TestHelper.LOCAL_HOST, node.port)
        cmd_str = base_cmd_str + "trace_api/get_block  -X POST -d " + ("'{\"block_num\":%s}'") % blockNum
        ret_json = Utils.runCmdReturnJson(cmd_str)
        self.assertIn("transactions", ret_json)

        isTrxInBlock = False
        for trx in ret_json["transactions"]:
            self.assertIn("id", trx)
            if (trx["id"] == transId) :
                isTrxInBlock = True
        self.assertTrue(isTrxInBlock)

    @classmethod
    def setUpClass(self):
        self.cleanEnv(self)
        self.startEnv(self)

    #@classmethod
    #def tearDownClass(self):
    #    self.cleanEnv(self)

if __name__ == "__main__":
    unittest.main()