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

class TraceApiPluginTest(unittest.TestCase):
    sleep_s = 2
    base_node_cmd_str = ("curl http://%s:%s/v1/") % (TestHelper.LOCAL_HOST, TestHelper.DEFAULT_PORT)
    base_wallet_cmd_str = ("curl http://%s:%s/v1/") % (TestHelper.LOCAL_HOST, TestHelper.DEFAULT_WALLET_PORT)
    keosd = WalletMgr(True, TestHelper.DEFAULT_PORT, TestHelper.LOCAL_HOST, TestHelper.DEFAULT_WALLET_PORT, TestHelper.LOCAL_HOST)
    node_id = 1
    nodeos = Node(TestHelper.LOCAL_HOST, TestHelper.DEFAULT_PORT, node_id, None, None, keosd)
    data_dir = Utils.getNodeDataDir(node_id)
    test_wallet_name="testwallet"
    acct_list = None
    test_wallet=None

    # make a fresh data dir
    def createDataDir(self):
        if os.path.exists(self.data_dir):
            shutil.rmtree(self.data_dir)
        os.makedirs(self.data_dir)

    # kill nodeos and keosd and clean up dir
    def cleanEnv(self) :
        self.keosd.killall(True)
        WalletMgr.cleanup()
        Node.killAllNodeos()
        if os.path.exists(self.data_dir):
            shutil.rmtree(self.data_dir)
        time.sleep(self.sleep_s)

    # start keosd and nodeos
    def startEnv(self) :
        self.createDataDir(self)
        self.keosd.launch()
        nodeos_plugins = (" --plugin %s --plugin %s --plugin %s --plugin %s --plugin %s ") % (  "eosio::trace_api_plugin",
                                                                                                "eosio::producer_plugin",
                                                                                                "eosio::producer_api_plugin",
                                                                                                "eosio::chain_api_plugin",
                                                                                                "eosio::http_plugin")
        nodeos_flags = (" --data-dir=%s --trace-dir=%s --trace-no-abis --filter-on=%s --access-control-allow-origin=%s "
                        "--contracts-console --http-validate-host=%s --verbose-http-errors ") % (self.data_dir, self.data_dir, "\"*\"", "\'*\'", "false")
        start_nodeos_cmd = ("%s -e -p eosio %s %s ") % (Utils.EosServerPath, nodeos_plugins, nodeos_flags)
        self.nodeos.launchCmd(start_nodeos_cmd, self.node_id)
        time.sleep(self.sleep_s)

    def add_accounts(self) :
        ''' add user account '''
        for acct in self.acct_list:
            self.nodeos.createInitializeAccount(acct, creatorAccount)

    def add_transactions(self) :
        ''' add transactions '''

    def test_TraceApi(self) :
        ''' 1. add accounts,
            2. push a few transactions,
            3. verify via get_block
        '''

    @classmethod
    def setUpClass(self):
        self.cleanEnv(self)
        self.startEnv(self)
        self.test_wallet=self.keosd.create(self.test_wallet_name, self.acct_list)
        self.acct_list = Cluster.createAccountKeys(3)
        self.acct_list[0].name = "alice"
        self.acct_list[1].name = "bob"
        self.acct_list[2].name = "charlie"

    @classmethod
    def tearDownClass(self):
        self.cleanEnv(self)

if __name__ == "__main__":
    unittest.main()