#!/usr/bin/env python3
import json
import os
import shutil
import time
import unittest

from testUtils import Utils
from TestHelper import TestHelper
from Node import Node
from WalletMgr import WalletMgr

class PluginHttpTest(unittest.TestCase):
    sleep_ms = 5
    base_cmd_str = ("curl http://%s:%s/v1/") % (TestHelper.LOCAL_HOST, TestHelper.DEFAULT_PORT)
    keosd = WalletMgr(True, TestHelper.DEFAULT_PORT, TestHelper.LOCAL_HOST, TestHelper.DEFAULT_WALLET_PORT, TestHelper.LOCAL_HOST)
    nodeos = Node(TestHelper.LOCAL_HOST, TestHelper.DEFAULT_PORT)
    node_id = 1
    data_dir = Utils.getNodeDataDir(node_id)
    http_post_str = " -X POST -d "
    http_post_invalid_param = " '{invalid}' "

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
        time.sleep(self.sleep_ms)

    # start keosd and nodeos
    def startEnv(self) :
        self.createDataDir(self)
        self.keosd.launch()
        nodeos_plugins = (" --plugin %s --plugin %s --plugin %s --plugin %s --plugin %s --plugin %s --plugin %s"
                          " --plugin %s --plugin %s --plugin %s --plugin %s ") % ( "eosio::trace_api_plugin",
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
        nodeos_flags = (" --data-dir=%s --trace-dir=%s --trace-no-abis --filter-on=%s --access-control-allow-origin=%s "
                        "--contracts-console --http-validate-host=%s --verbose-http-errors ") % (self.data_dir, self.data_dir, "\"*\"", "\'*\'", "false")
        start_nodeos_cmd = ("%s -e -p eosio %s %s ") % (Utils.EosServerPath, nodeos_plugins, nodeos_flags)
        self.nodeos.launchCmd(start_nodeos_cmd, self.node_id)
        time.sleep(self.sleep_ms)

    # test all chain api
    def test_ChainApi(self) :
        cmd_base = self.base_cmd_str + "chain/"

        # get_info without parameter
        default_cmd = cmd_base + "get_info"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertIn("server_version", ret_json)
        # get_info with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)

        # get_activated_protocol_features without parameter
        default_cmd = cmd_base + "get_activated_protocol_features"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        # get_activated_protocol_features with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        # get_activated_protocol_features with valid parameter
        valid_cmd = ("%s%s '{%s,%s,%s,%s,%s}'") % ( default_cmd,
                                                    self.http_post_str,
                                                    "\"lower_bound\":1",
                                                    "\"upper_bound\":1000",
                                                    "\"limit\":10",
                                                    "\"search_by_block_num\":true",
                                                    "\"reverse\":true")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(type(ret_json["activated_protocol_features"]), list)

        # get_block with empty parameter
        default_cmd = cmd_base + "get_block"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        # get_block with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        # get_block with valid parameter
        valid_cmd = default_cmd + self.http_post_str + "'{\"block_num_or_id\":1}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["block_num"], 1)

        # get_block_info with empty parameter
        default_cmd = cmd_base + "get_block_info"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        # get_block_info with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        # get_block_info with valid parameter
        valid_cmd = default_cmd + self.http_post_str + "'{\"block_num\":1}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["block_num"], 1)

        # get_block_header_state with empty parameter
        default_cmd = cmd_base + "get_block_header_state"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_block_header_state with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_block_header_state with valid parameter, the irreversible is not available, unknown block number
        valid_cmd = default_cmd + self.http_post_str + "'{\"block_num_or_id\":1}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3100002)

        # get_account with empty parameter
        default_cmd = cmd_base + "get_account"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_account with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_account with valid parameter
        valid_cmd = default_cmd + self.http_post_str + "'{\"account_name\":\"default\"}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_code with empty parameter
        default_cmd = cmd_base + "get_code"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_code with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_code with valid parameter
        valid_cmd = default_cmd + self.http_post_str + "'{\"account_name\":\"default\"}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_code_hash with empty parameter
        default_cmd = cmd_base + "get_code_hash"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_code_hash with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_code_hash with valid parameter
        valid_cmd = default_cmd + self.http_post_str + "'{\"account_name\":\"default\"}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_abi with empty parameter
        default_cmd = cmd_base + "get_abi"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_abi with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_abi with valid parameter
        valid_cmd = default_cmd + self.http_post_str + "'{\"account_name\":\"default\"}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_raw_code_and_abi with empty parameter
        default_cmd = cmd_base + "get_raw_code_and_abi"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_raw_code_and_abi with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_raw_code_and_abi with valid parameter
        valid_cmd = default_cmd + self.http_post_str + "'{\"account_name\":\"default\"}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_raw_abi with empty parameter
        default_cmd = cmd_base + "get_raw_abi"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_raw_abi with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_raw_abi with valid parameter
        valid_cmd = default_cmd + self.http_post_str + "'{\"account_name\":\"default\"}'"
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_table_rows with empty parameter
        default_cmd = cmd_base + "get_table_rows"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_table_rows with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_table_rows with valid parameter
        valid_cmd = ("%s%s '{%s,%s,%s,%s,%s,%s,%s,%s}'") % (  default_cmd,
                                                              self.http_post_str,
                                                              "\"json\":true",
                                                              "\"code\":\"cancancan345\"",
                                                              "\"scope\":\"cancancan345\"",
                                                              "\"table\":\"vote\"",
                                                              "\"index_position\":2",
                                                              "\"key_type\":\"i128\"",
                                                              "\"lower_bound\":\"0x0000000000000000D0F2A472A8EB6A57\"",
                                                              "\"upper_bound\":\"0xFFFFFFFFFFFFFFFFD0F2A472A8EB6A57\"")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_table_by_scope with empty parameter
        default_cmd = cmd_base + "get_table_by_scope"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_table_by_scope with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_table_by_scope with valid parameter
        valid_cmd = ("%s%s '{%s,%s,%s,%s,%s}'") % (   default_cmd,
                                                      self.http_post_str,
                                                      "\"code\":\"cancancan345\"",
                                                      "\"table\":\"vote\"",
                                                      "\"index_position\":2",
                                                      "\"lower_bound\":\"0x0000000000000000D0F2A472A8EB6A57\"",
                                                      "\"upper_bound\":\"0xFFFFFFFFFFFFFFFFD0F2A472A8EB6A57\"")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_currency_balance with empty parameter
        default_cmd = cmd_base + "get_currency_balance"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_currency_balance with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_currency_balance with valid parameter
        valid_cmd = default_cmd + self.http_post_str + ("'{\"code\":\"eosio.token\", \"account\":\"unknown\"}'")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_currency_stats with empty parameter
        default_cmd = cmd_base + "get_currency_stats"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_currency_stats with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_currency_stats with valid parameter
        valid_cmd = default_cmd + self.http_post_str + ("'{\"code\":\"eosio.token\",\"symbol\":\"SYS\"}'")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_producers with empty parameter
        default_cmd = cmd_base + "get_producers"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_producers with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_producers with valid parameter
        valid_cmd = default_cmd + self.http_post_str + ("'{\"json\":true,\"lower_bound\":\"\"}'")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(type(ret_json["rows"]), list)

        # get_producer_schedule with empty parameter
        default_cmd = cmd_base + "get_producer_schedule"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(type(ret_json["active"]), dict)
        # get_producer_schedule with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)

        # get_scheduled_transactions with empty parameter
        default_cmd = cmd_base + "get_scheduled_transactions"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_scheduled_transactions with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_scheduled_transactions with valid parameter
        valid_cmd = default_cmd + self.http_post_str + ("'{\"json\":true,\"lower_bound\":\"\"}'")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(type(ret_json["transactions"]), list)

        # abi_json_to_bin with empty parameter
        default_cmd = cmd_base + "abi_json_to_bin"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # abi_json_to_bin with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # abi_json_to_bin with valid parameter
        valid_cmd = ("%s%s '{%s,%s,%s}'") % (default_cmd,
                                             self.http_post_str,
                                             "\"code\":\"eosio.token\"",
                                             "\"action\":\"issue\"",
                                             "\"args\":{\"to\":\"eosio.token\", \"quantity\":\"1.0000\%20EOS\",\"memo\":\"m\"}")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # abi_bin_to_json with empty parameter
        default_cmd = cmd_base + "abi_bin_to_json"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # abi_bin_to_json with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # abi_bin_to_json with valid parameter
        valid_cmd = ("%s%s '{%s,%s,%s}'") % (default_cmd,
                                             self.http_post_str,
                                             "\"code\":\"eosio.token\"",
                                             "\"action\":\"issue\"",
                                             "\"args\":\"ee6fff5a5c02c55b6304000000000100a6823403ea3055000000572d3ccdcd0100000000007015d600000000a8ed32322a00000000007015d6000000005c95b1ca102700000000000004454f53000000000968656c6c6f206d616e00\"")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_required_keys with empty p4arameter
        default_cmd = cmd_base + "get_required_keys"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_required_keys with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_required_keys with valid parameter
        valid_cmd = ("%s%s '{\"transaction\":{%s,%s,%s,%s,%s,%s,%s},\"available_keys\":[%s,%s,%s]}'") % \
                    (default_cmd,
                     self.http_post_str,
                     "\"ref_block_num\":\"100\"",
                     "\"ref_block_prefix\": \"137469861\"",
                     "\"expiration\": \"2020-09-25T06:28:49\"",
                     "\"scope\":[\"initb\", \"initc\"]",
                     "\"actions\": [{\"code\": \"currency\",\"type\":\"transfer\",\"recipients\": [\"initb\", \"initc\"],\"authorization\": [{\"account\": \"initb\", \"permission\": \"active\"}],\"data\":\"000000000041934b000000008041934be803000000000000\"}]",
                     "\"signatures\": []",
                     "\"authorizations\": []",
                     "\"EOS4toFS3YXEQCkuuw1aqDLrtHim86Gz9u3hBdcBw5KNPZcursVHq\"",
                     "\"EOS7d9A3uLe6As66jzN8j44TXJUqJSK3bFjjEEqR4oTvNAB3iM9SA\"",
                     "\"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\"")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # get_transaction_id with empty p4arameter
        default_cmd = cmd_base + "get_transaction_id"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_transaction_id with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # get_transaction_id with valid parameter
        valid_cmd = ("%s%s '{%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s}'") % \
                    (default_cmd,
                     self.http_post_str,
                     "\"expiration\":\"2020-08-01T07:15:49\"",
                     "\"ref_block_num\": 34881",
                     "\"ref_block_prefix\":2972818865",
                     "\"max_net_usage_words\":0",
                     "\"max_cpu_usage_ms\":0",
                     "\"delay_sec\":0",
                     "\"context_free_actions\":[]",
                     "\"actions\":[{\"account\":\"eosio.token\",\"name\": \"transfer\",\"authorization\": [{\"actor\": \"han\",\"permission\": \"active\"}],\"data\": \"000000000000a6690000000000ea305501000000000000000453595300000000016d\"}]",
                     "\"transaction_extensions\": []",
                     "\"signatures\": [\"SIG_K1_KeqfqiZu1GwUxQb7jzK9Fdks6HFaVBQ9AJtCZZj56eG9qGgvVMVtx8EerBdnzrhFoX437sgwtojf2gfz6S516Ty7c22oEp\"]",
                     "\"context_free_data\": []")
        ret_str = Utils.runCmdReturnStr(valid_cmd)
        self.assertEqual(ret_str, "\"0be762a6406bab15530e87f21e02d1c58e77944ee55779a76f4112e3b65cac48\"")

        # push_block with empty parameter
        default_cmd = cmd_base + "push_block"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # push_block with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # push_block with valid parameter
        valid_cmd = default_cmd + self.http_post_str + ("'{\"block\":\"signed_block\"}'")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(len(ret_json), 0)

        # push_transaction with empty parameter
        default_cmd = cmd_base + "push_transaction"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # push_transaction with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # push_transaction with valid parameter
        valid_cmd = ("%s%s '{%s, %s, %s, %s}'") % (default_cmd,
                                                   self.http_post_str,
                                                   "\"signatures\":[\"SIG_K1_KeqfqiZu1GwUxQb7jzK9Fdks6HFaVBQ9AJtCZZj56eG9qGgvVMVtx8EerBdnzrhFoX437sgwtojf2gfz6S516Ty7c22oEp\"]",
                                                   "\"compression\": true",
                                                   "\"packed_context_free_data\": \"context_free_data\"",
                                                   "\"packed_trx\": \"packed_trx\"")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

        # push_transactions with empty parameter
        default_cmd = cmd_base + "push_transactions"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # push_transactions with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # push_transactions with valid parameter
        valid_cmd = ("%s%s '[{%s, %s, %s, %s}]'") % (default_cmd,
                                                   self.http_post_str,
                                                   "\"signatures\":[\"SIG_K1_KeqfqiZu1GwUxQb7jzK9Fdks6HFaVBQ9AJtCZZj56eG9qGgvVMVtx8EerBdnzrhFoX437sgwtojf2gfz6S516Ty7c22oEp\"]",
                                                   "\"compression\": true",
                                                   "\"packed_context_free_data\": \"context_free_data\"",
                                                   "\"packed_trx\": \"packed_trx\"")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertIn("transaction_id", ret_json[0])

        # send_transaction with empty parameter
        default_cmd = cmd_base + "send_transaction"
        ret_json = Utils.runCmdReturnJson(default_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # send_transaction with invalid parameter
        invalid_cmd = default_cmd + self.http_post_str + self.http_post_invalid_param
        ret_json = Utils.runCmdReturnJson(invalid_cmd)
        self.assertEqual(ret_json["code"], 400)
        self.assertEqual(ret_json["error"]["code"], 3200006)
        # send_transaction with valid parameter
        valid_cmd = ("%s%s '{%s, %s, %s, %s}'") % (default_cmd,
                                                   self.http_post_str,
                                                   "\"signatures\":[\"SIG_K1_KeqfqiZu1GwUxQb7jzK9Fdks6HFaVBQ9AJtCZZj56eG9qGgvVMVtx8EerBdnzrhFoX437sgwtojf2gfz6S516Ty7c22oEp\"]",
                                                   "\"compression\": true",
                                                   "\"packed_context_free_data\": \"context_free_data\"",
                                                   "\"packed_trx\": \"packed_trx\"")
        ret_json = Utils.runCmdReturnJson(valid_cmd)
        self.assertEqual(ret_json["code"], 500)

    # test all net api
    def test_NetApi(self) :
        cmd_base = self.base_cmd_str + "net/"

    # test all producer api
    def test_ProducerApi(self) :
        cmd_base = self.base_cmd_str + "producer/"

    # test all wallet api
    def test_WalletApi(self) :
        cmd_base = self.base_cmd_str + "wallet/"

    # test all trace api
    def test_TraceApi(self) :
        cmd_base = self.base_cmd_str + "trace_api/"

    @classmethod
    def setUpClass(self):
        self.cleanEnv(self)
        self.startEnv(self)

    @classmethod
    def tearDownClass(self):
        self.cleanEnv(self)

if __name__ == "__main__":
    unittest.main()
