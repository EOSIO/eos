#!/usr/bin/env python3

# This script tests that the compiled binaries produce expected output in
# response to the `--help` option. It also contains a couple of additional
# CLI-related checks as well as test cases for CLI bugfixes.

import subprocess
import re


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

nodeos_help_test()

cleos_help_test(['--help'])
cleos_help_test(['system', '--help'])
cleos_help_test(['version', '--help'])
cleos_help_test(['wallet', '--help'])

cli11_bugfix_test()

cli11_optional_option_arg_test()
cleos_sign_test()
