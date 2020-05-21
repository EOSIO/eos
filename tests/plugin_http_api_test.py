#!/usr/bin/env python3
from testUtils import Utils
from TestHelper import TestHelper
from Node import Node
from WalletMgr import WalletMgr

import os
import shutil
import time

# kill nodeos and keosd and clean up dir
def clean(keosd : WalletMgr, data_dir : str) :
    keosd.killall(True)
    WalletMgr.cleanup()
    Node.killAllNodeos()
    if os.path.exists(data_dir):
        shutil.rmtree(data_dir)

def start(keosd : WalletMgr, nodeos : Node, nodeos_cmd : str, node_id : int) :
    keosd.launch()
    nodeos.launchCmd(nodeos_cmd, node_id)

def createDataDir(data_dir : str):
    if os.path.exists(data_dir):
        shutil.rmtree(data_dir)
    os.makedirs(data_dir)

def main():
    keosd = WalletMgr(True, TestHelper.DEFAULT_PORT, TestHelper.LOCAL_HOST, TestHelper.DEFAULT_WALLET_PORT, TestHelper.LOCAL_HOST)

    node_id = 1
    data_dir = Utils.getNodeDataDir(node_id)
    clean(keosd, data_dir)

    nodeos = Node(TestHelper.LOCAL_HOST, TestHelper.DEFAULT_PORT)
    nodeos_plugins = (" --plugin {0} --plugin {1} --plugin {2} --plugin {3} --plugin {4} --plugin {5} --plugin {6}"
                      " --plugin {7} --plugin {8} --plugin {9} --plugin {10} ").format("eosio::trace_api_plugin",
                                                                                       "eosio::test_control_api_plugin",
                                                                                       "eosio::test_control_plugin",
                                                                                       "eosio::net_plugin",
                                                                                       "eosio::net_api_plugin",
                                                                                       "eosio::producer_plugin",
                                                                                       "eosio::producer_api_plugin",
                                                                                       "eosio::chain_api_plugin",
                                                                                       "eosio::http_plugin",
                                                                                       "eosio::history_plugin",
                                                                                       "eosio::history_api_plugin")
    nodeos_flags = (" --data-dir={0} --trace-dir={0} --trace-no-abis --filter-on={1} --access-control-allow-origin={2} "
                    "--contracts-console --http-validate-host={3} --verbose-http-errors ").format(data_dir, "\"*\"", "\'*\'", "false")
    start_nodeos_cmd = ("{0} -p eosio {1} {2} ").format(Utils.EosServerPath, nodeos_plugins, nodeos_flags )
    createDataDir(data_dir)
    start(keosd, nodeos, start_nodeos_cmd, node_id)
    clean(keosd, data_dir)

if __name__ == "__main__":
    main()
