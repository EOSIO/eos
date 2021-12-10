#!/usr/bin/env python3

# This script tests that the compiled binaries produce expected output in
# response to the `--help` option. It also contains a couple of additional
# CLI-related checks as well as test cases for CLI bugfixes.

import subprocess
import re
import os
import time
import shutil
import signal


def nodeos_help_test():
    """Test that nodeos help contains option descriptions"""
    help_text = subprocess.check_output(["./programs/nodeos/nodeos", "--help"])

    assert(re.search(b'Application.*Options', help_text))
    assert(re.search(b'Options for .*_plugin', help_text))


def cleos_help_test(args):
    """Test that cleos help contains option and subcommand descriptions"""
    help_text = subprocess.check_output(["./programs/cleos/cleos"] + args)

    assert(b'Options:' in help_text)
    assert(b'Subcommands:' in help_text)


def cli11_bugfix_test():
    """Test that subcommand names can be used as option arguments"""
    completed_process = subprocess.run(
        ['./programs/cleos/cleos', '--no-auto-keosd', '-u', 'http://localhost:0/',
         'push', 'action', 'accout', 'action', '["data"]', '-p', 'wallet'],
        check=False,
        stderr=subprocess.PIPE)

    # The above command must fail because there is no server running
    # on localhost:0
    assert(completed_process.returncode != 0)

    # Make sure that the command failed because of the connection error,
    # not the command line parsing error.
    assert(b'Failed to connect to nodeos' in completed_process.stderr)


def cli11_optional_option_arg_test():
    """Test that options like --password can be specified without a value"""
    chain = 'cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f'
    key = '5Jgfqh3svgBZvCAQkcnUX8sKmVUkaUekYDGqFakm52Ttkc5MBA4'

    output = subprocess.check_output(['./programs/cleos/cleos', '--no-auto-keosd', 'sign',
                                      '-c', chain, '-k', '{}'],
                                     input=key.encode(),
                                     stderr=subprocess.DEVNULL)
    assert(b'signatures' in output)

    output = subprocess.check_output(['./programs/cleos/cleos', '--no-auto-keosd', 'sign',
                                      '-c', chain, '-k', key, '{}'])
    assert(b'signatures' in output)

def cleos_sign_test():
    """Test that sign can on both regular and packed transactions"""
    chain = 'cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f'
    key = '5Jgfqh3svgBZvCAQkcnUX8sKmVUkaUekYDGqFakm52Ttkc5MBA4'

    # regular trasaction
    trx = (
        '{'
        '"expiration": "2019-08-01T07:15:49",'
        '"ref_block_num": 34881,'
        '"ref_block_prefix": 2972818865,'
        '"max_net_usage_words": 0,'
        '"max_cpu_usage_ms": 0,'
        '"delay_sec": 0,'
        '"context_free_actions": [],'
        '"actions": [{'
            '"account": "eosio.token",'
            '"name": "transfer",'
            '"authorization": [{'
            '"actor": "eosio",'
            '"permission": "active"'
        '}'
        '],'
        '"data": "000000000000a6690000000000ea305501000000000000000453595300000000016d"'
       '}'
       '],'
        '"transaction_extensions": [],'
        '"context_free_data": []'
    '}')

    output = subprocess.check_output(['./programs/cleos/cleos', 'sign',
                                      '-c', chain, '-k', key, trx])
    # make sure it is signed
    assert(b'signatures' in output)
    # make sure fields are kept
    assert(b'"expiration": "2019-08-01T07:15:49"' in output)
    assert(b'"ref_block_num": 34881' in output)
    assert(b'"ref_block_prefix": 2972818865' in output)
    assert(b'"account": "eosio.token"' in output)
    assert(b'"name": "transfer"' in output)
    assert(b'"actor": "eosio"' in output)
    assert(b'"permission": "active"' in output)
    assert(b'"data": "000000000000a6690000000000ea305501000000000000000453595300000000016d"' in output)

    packed_trx = ' { "signatures": [], "compression": "none", "packed_context_free_data": "", "packed_trx": "a591425d4188b19d31b1000000000100a6823403ea3055000000572d3ccdcd010000000000ea305500000000a8ed323222000000000000a6690000000000ea305501000000000000000453595300000000016d00" } '

    # Test packed transaction is unpacked. Only with options --print-request and --public-key
    # the sign request is dumped to stderr.
    cmd = ['./programs/cleos/cleos', '--print-request', 'sign', '-c', chain, '--public-key', 'EOS8Dq1KosJ9PMn1vKQK3TbiihgfUiDBUsz471xaCE6eYUssPB1KY', packed_trx]
    outs=None
    errs=None
    try:
        popen=subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        outs,errs=popen.communicate()
        popen.wait()
    except subprocess.CalledProcessError as ex:
        print(ex.output)
    # make sure fields are unpacked
    assert(b'"expiration": "2019-08-01T07:15:49"' in errs)
    assert(b'"ref_block_num": 34881' in errs)
    assert(b'"ref_block_prefix": 2972818865' in errs)
    assert(b'"account": "eosio.token"' in errs)
    assert(b'"name": "transfer"' in errs)
    assert(b'"actor": "eosio"' in errs)
    assert(b'"permission": "active"' in errs)
    assert(b'"data": "000000000000a6690000000000ea305501000000000000000453595300000000016d"' in errs)

    # Test packed transaction is signed.
    output = subprocess.check_output(['./programs/cleos/cleos', 'sign',
                                      '-c', chain, '-k', key, packed_trx])
    # Make sure signatures not empty
    assert(b'signatures' in output)
    assert(b'"signatures": []' not in output)

def processCleosCommand(cmd):
    outs = None
    errs = None
    try:
        popen=subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        outs, errs = popen.communicate()
        popen.wait()
    except subprocess.CalledProcessError as ex:
        print(ex.output)
    return outs, errs

def cleos_abi_file_test():
    """Test option --abi-file """
    token_abi_path = os.path.abspath(os.getcwd() + '/../unittests/contracts/eosio.token/eosio.token.abi')
    system_abi_path = os.path.abspath(os.getcwd() + '/../unittests/contracts/eosio.system/eosio.system.abi')
    token_abi_file_arg = 'eosio.token' + ':' + token_abi_path
    system_abi_file_arg = 'eosio' + ':' + system_abi_path

    # no option --abi-file
    account = 'eosio.token'
    action = 'transfer'
    unpacked_action_data = '{"from":"aaa","to":"bbb","quantity":"10.0000 SYS","memo":"hello"}'
    # use URL http://127.0.0.1:12345 to make sure cleos not to connect to any running nodeos
    cmd = ['./programs/cleos/cleos', '-u', 'http://127.0.0.1:12345', 'convert', 'pack_action_data', account, action, unpacked_action_data]
    outs, errs = processCleosCommand(cmd)
    assert(b'Failed to connect to nodeos' in errs)

    # invalid option --abi-file
    invalid_abi_arg = 'eosio.token' + ' ' + token_abi_path
    cmd = ['./programs/cleos/cleos', '-u', 'http://127.0.0.1:12345', '--abi-file', invalid_abi_arg, 'convert', 'pack_action_data', account, action, unpacked_action_data]
    outs, errs = processCleosCommand(cmd)
    assert(b'please specify --abi-file in form of <contract name>:<abi file path>.' in errs)

    # pack token transfer data
    account = 'eosio.token'
    action = 'transfer'
    unpacked_action_data = '{"from":"aaa","to":"bbb","quantity":"10.0000 SYS","memo":"hello"}'
    packed_action_data = '0000000000008c31000000000000ce39a08601000000000004535953000000000568656c6c6f'
    cmd = ['./programs/cleos/cleos', '-u','http://127.0.0.1:12345', '--abi-file', token_abi_file_arg, 'convert', 'pack_action_data', account, action, unpacked_action_data]
    outs, errs = processCleosCommand(cmd)
    actual = outs.strip()
    assert(actual.decode('utf-8') == packed_action_data)

    # unpack token transfer data
    cmd = ['./programs/cleos/cleos', '-u','http://127.0.0.1:12345', '--abi-file', token_abi_file_arg, 'convert', 'unpack_action_data', account, action, packed_action_data]
    outs, errs = processCleosCommand(cmd)
    assert(b'"from": "aaa"' in outs)
    assert(b'"to": "bbb"' in outs)
    assert(b'"quantity": "10.0000 SYS"' in outs)
    assert(b'"memo": "hello"' in outs)

    # pack account create data
    account = 'eosio'
    action = 'newaccount'

    unpacked_action_data = """{
        "creator": "eosio",
        "name": "bob",
        "owner": {
          "threshold": 1,
          "keys": [{
              "key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
              "weight": 1
            }
          ],
          "accounts": [],
          "waits": []
        },
        "active": {
          "threshold": 1,
          "keys": [{
              "key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
              "weight": 1
            }
          ],
          "accounts": [],
          "waits": []
        }
    }"""

    cmd = ['./programs/cleos/cleos', '-u','http://127.0.0.1:12345', '--abi-file', system_abi_file_arg, 'convert', 'pack_action_data', account, action, unpacked_action_data]
    packed_action_data = '0000000000ea30550000000000000e3d01000000010002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf0100000001000000010002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf01000000'
    outs, errs = processCleosCommand(cmd)
    actual = outs.strip()
    assert(actual.decode('utf-8') == packed_action_data)

    # unpack account create data
    cmd = ['./programs/cleos/cleos', '-u','http://127.0.0.1:12345', '--abi-file', system_abi_file_arg, 'convert', 'unpack_action_data', account, action, packed_action_data]
    outs, errs = processCleosCommand(cmd)
    assert(b'"creator": "eosio"' in outs)
    assert(b'"name": "bob"' in outs)

    # pack transaction
    unpacked_trx = """{
        "expiration": "2021-07-18T04:21:14",
        "ref_block_num": 494,
        "ref_block_prefix": 2118878731,
        "max_net_usage_words": 0,
        "max_cpu_usage_ms": 0,
        "delay_sec": 0,
        "context_free_actions": [],
        "actions": [{
            "account": "eosio",
            "name": "newaccount",
            "authorization": [{
                "actor": "eosio",
                "permission": "active"
                }
            ],
            "data": {
                "creator": "eosio",
                "name": "bob",
                "owner": {
                "threshold": 1,
                "keys": [{
                    "key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                    "weight": 1
                    }
                ],
                "accounts": [],
                "waits": []
                },
                "active": {
                "threshold": 1,
                "keys": [{
                    "key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                    "weight": 1
                    }
                ],
                "accounts": [],
                "waits": []
                }
            },
            "hex_data": "0000000000ea30550000000000000e3d01000000010002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf0100000001000000010002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf01000000"
            },
            {
            "account": "eosio.token",
            "name": "transfer",
            "authorization": [{
                "actor": "aaa",
                "permission": "active"
                }
            ],
            "data": {
                "from": "aaa",
                "to": "bbb",
                "quantity": "10.0000 SYS",
                "memo": "hello"
            },
            "hex_data": "0000000000008c31000000000000ce39a08601000000000004535953000000000568656c6c6f"
            }
        ],
        "signatures": [
            "SIG_K1_K3LfbB7ZV2DNBu67iSn3yUMseTdiwoT49gAcwSZVT1QTvGXVHjkcvKqhentCW4FJngZJ1H9gBRSWgo9UPiWEXWHyKpXNCZ"
        ],
        "context_free_data": []
    }"""

    expected_output = b'3aacf360ee010b864b7e00000000020000000000ea305500409e9a2264b89a010000000000ea305500000000a8ed3232660000000000ea30550000000000000e3d01000000010002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf0100000001000000010002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf0100000000a6823403ea3055000000572d3ccdcd010000000000008c3100000000a8ed3232260000000000008c31000000000000ce39a08601000000000004535953000000000568656c6c6f00'
    cmd = ['./programs/cleos/cleos', '-u','http://127.0.0.1:12345', '--abi-file', system_abi_file_arg, token_abi_file_arg, 'convert', 'pack_transaction', '--pack-action-data', unpacked_trx]
    outs, errs = processCleosCommand(cmd)
    assert(expected_output in outs)

    # unpack transaction
    packed_trx = """{
        "signatures": [
            "SIG_K1_K3LfbB7ZV2DNBu67iSn3yUMseTdiwoT49gAcwSZVT1QTvGXVHjkcvKqhentCW4FJngZJ1H9gBRSWgo9UPiWEXWHyKpXNCZ"
        ],
        "compression": "none",
        "packed_context_free_data": "",
        "packed_trx": "3aacf360ee010b864b7e00000000020000000000ea305500409e9a2264b89a010000000000ea305500000000a8ed3232660000000000ea30550000000000000e3d01000000010002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf0100000001000000010002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf0100000000a6823403ea3055000000572d3ccdcd010000000000008c3100000000a8ed3232260000000000008c31000000000000ce39a08601000000000004535953000000000568656c6c6f00"
    }"""
    cmd = ['./programs/cleos/cleos', '-u','http://127.0.0.1:12345', '--abi-file', system_abi_file_arg, token_abi_file_arg, 'convert', 'unpack_transaction', '--unpack-action-data', packed_trx]
    outs, errs = processCleosCommand(cmd)
    assert(b'"creator": "eosio"' in outs)
    assert(b'"name": "bob"' in outs)

    assert(b'"from": "aaa"' in outs)
    assert(b'"to": "bbb"' in outs)
    assert(b'"quantity": "10.0000 SYS"' in outs)
    assert(b'"memo": "hello"' in outs)

    # push action token transfer with option `--abi-file`
    try:
        data_dir = "./taf_data"
        cmd = "./programs/nodeos/nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::chain_plugin --plugin eosio::http_plugin --access-control-allow-origin=* --http-validate-host=false " + "--data-dir " + data_dir
        pNodeos = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        time.sleep(10)

        PUBLIC_KEY  = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
        PRIVATE_KEY = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"

        wallet_name = "taf_wallet"
        cmd = "./programs/cleos/cleos wallet create --name " + wallet_name + " --to-console"
        pWallet = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        cmd = "./programs/cleos/cleos wallet import --name " + wallet_name + " --private-key " + PRIVATE_KEY
        processCleosCommand(cmd.split())

        accounts = ["eosio.token", "alice", "bob"]
        for account in accounts:
            cmd = "./programs/cleos/cleos get account -j " + account
            outs, errs = processCleosCommand(cmd.split())
            str = '"account_name"' + ': ' + '"' + account + '"'
            if str not in outs.decode('utf-8'):
                cmd = "./programs/cleos/cleos -u http://127.0.0.1:8888 create account eosio " + account + " " + PUBLIC_KEY
                outs, errs = processCleosCommand(cmd.split())

        cmd = "./programs/cleos/cleos set contract eosio.token " + os.path.abspath(os.getcwd() + "/../unittests/contracts/eosio.token") + " " + "--abi " + "eosio.token.abi -p eosio.token@active"
        processCleosCommand(cmd.split())
        cmd = ['./programs/cleos/cleos', 'push', 'action', 'eosio.token', 'create', '[ "alice", "1000000000.0000 SYS" ]', '-p', 'eosio.token']
        processCleosCommand(cmd)
        cmd = ['./programs/cleos/cleos', 'push', 'action', 'eosio.token', 'issue', '[ "alice", "100.0000 SYS", "memo" ]', '-p', 'alice']
        processCleosCommand(cmd)

        # make a malicious abi file by switching 'from' and 'to' in eosio.token.abi
        malicious_token_abi_path = os.path.abspath(os.getcwd() + '/../unittests/contracts/eosio.token/malicious.eosio.token.abi')
        shutil.copyfile(token_abi_path, malicious_token_abi_path)
        replaces = [["from", "malicious"], ["to", "from"], ["malicious", "to"]]
        for replace in replaces:
            with open(malicious_token_abi_path, 'r+') as f:
                abi = f.read()
                abi = re.sub(replace[0], replace[1], abi)
                f.seek(0)
                f.write(abi)
                f.truncate()

        # set the malicious abi
        cmd = "./programs/cleos/cleos set abi eosio.token " + malicious_token_abi_path
        processCleosCommand(cmd.split())

        # option '--abi-file' makes token still be transferred from alice to bob after setting the malicious abi
        cmd = ['./programs/cleos/cleos', '-u', 'http://127.0.0.1:8888', '--print-request', '--abi-file', token_abi_file_arg, 'push', 'action', 'eosio.token', 'transfer', '{ "from":"alice", "to":"bob", "quantity":"25.0000 SYS", "memo":"m" }', '-p', 'alice']
        outs, errs = processCleosCommand(cmd)
        assert(b'"/v1/chain/get_raw_abi"' not in errs)
        cmd = "./programs/cleos/cleos get currency balance eosio.token alice SYS"
        outs, errs = processCleosCommand(cmd.split())
        assert(outs.strip().decode('utf-8') == "75.0000 SYS")

    finally:

        if pNodeos.pid:
            os.kill(pNodeos.pid, signal.SIGKILL)
        shutil.rmtree(data_dir)

        if pWallet.pid:
            os.kill(pWallet.pid, signal.SIGKILL)
        subprocess.call(("pkill -9 keosd").split())
        wallet_file = os.path.expanduser('~') + "/eosio-wallet/" + wallet_name + ".wallet"
        if os.path.exists(wallet_file):
            os.remove(wallet_file)

        if os.path.exists(malicious_token_abi_path):
            os.remove(malicious_token_abi_path)

nodeos_help_test()

cleos_help_test(['--help'])
cleos_help_test(['system', '--help'])
cleos_help_test(['version', '--help'])
cleos_help_test(['wallet', '--help'])

cli11_bugfix_test()

cli11_optional_option_arg_test()
cleos_sign_test()
cleos_abi_file_test()
