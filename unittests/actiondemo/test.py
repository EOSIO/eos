#!/usr/bin/env python3

import argparse
import json

import os

import subprocess

import time

args = None
logFile = None

unlockTimeout = 999999

systemAccounts = [
    'eosio.bpay',
    'eosio.msig',
    'eosio.names',
    'eosio.ram',
    'eosio.ramfee',
    'eosio.saving',
    'eosio.stake',
    'eosio.token',
    'eosio.vpay',
]


def jsonArg(a):
    return " '" + json.dumps(a) + "' "

def run(args):
    print('testtool.py:', args)
    logFile.write(args + '\n')
    if subprocess.call(args, shell=True):
        print('testtool.py: exiting because of error')
        #sys.exit(1)

def retry(args):
    while True:
        print('testtool.py:', args)
        logFile.write(args + '\n')
        if subprocess.call(args, shell=True):
            print('*** Retry')
        else:
            break

def background(args):
    print('testtool.py:', args)
    logFile.write(args + '\n')
    return subprocess.Popen(args, shell=True)

def getOutput(args):
    print('testtool.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return proc.communicate()[0].decode('utf-8')

def getJsonOutput(args):
    print('testtool.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return json.loads(proc.communicate()[0].decode('utf-8'))

def sleep(t):
    print('sleep', t, '...')
    time.sleep(t)
    print('resume')

def startWallet():
    run('rm -rf ' + os.path.abspath(args.wallet_dir))
    run('mkdir -p ' + os.path.abspath(args.wallet_dir))
    background(args.keosd + ' --unlock-timeout %d --http-server-address 127.0.0.1:6666 --wallet-dir %s' % (unlockTimeout, os.path.abspath(args.wallet_dir)))
    sleep(4)
    run(args.cleos + 'wallet create --file ./unlock.key ' )

def importKeys():
    run(args.cleos + 'wallet import --private-key ' + args.private_key)

# def createStakedAccounts(b, e):
#     for i in range(b, e):
#         a = accounts[i]
#         stake = 100
#         run(args.cleos + 'system newaccount eosio --transfer ' + a['name'] + ' ' + a['pub'] + ' --stake-net "' + stake + '" --stake-cpu "' + stake + '"')


def stepStartWallet():
    startWallet()
    importKeys()
    # run('rm -rf  ~/.local/share/eosio/nodeos/data ')
    run("rm -rf  ./data/*")
    background(args.nodeos + ' -e -p eosio --blocks-dir ./data/block/ --genesis-json %s  --config-dir ./ --data-dir ./data/  --plugin eosio::http_plugin --plugin eosio::chain_api_plugin --plugin eosio::producer_plugin --plugin eosio::history_api_plugin> eos.log 2>&1 &' % args.genesis)
    run("rm -rf  ./data2/*")
    background(args.nodeos + '  --blocks-dir ./data2/block/ --genesis-json %s  --data-dir ./data2/  --config-dir ./  --p2p-peer-address 127.0.0.1:9876  --http-server-address 0.0.0.0:8001 --p2p-listen-endpoint 0.0.0.0:9001 --plugin eosio::http_plugin --plugin eosio::chain_api_plugin --plugin eosio::producer_plugin --plugin eosio::history_api_plugin > eos2.log 2>&1 &' % args.genesis)
    sleep(30)


def createAccounts():
    for a in systemAccounts:
        run(args.cleos + 'create account eosio ' + a + ' ' + args.public_key)
    run(args.cleos + 'set contract eosio.token ' + args.contracts_dir + 'eosio.token/')
    run(args.cleos + 'set contract eosio.msig ' + args.contracts_dir + 'eosio.msig/')
    run(args.cleos + 'push action eosio.token create \'["eosio", "10000000000.0000 %s"]\' -p eosio.token' % (args.symbol))
    run(args.cleos + 'push action eosio.token issue \'["eosio", "%s %s", "memo"]\' -p eosio' % ("1000000.0000", args.symbol))
    retry(args.cleos + 'set contract eosio ' + args.contracts_dir + 'eosio.system/ -p eosio')
    sleep(1)
    run(args.cleos + 'push action eosio setpriv' + jsonArg(['eosio.msig', 1]) + '-p eosio@active')

    for a in accounts:
        run(args.cleos + 'system newaccount --stake-net "10.0000 %s" --stake-cpu "10.0000 %s" --buy-ram-kbytes 80 eosio ' %(args.symbol,args.symbol) + a + ' ' + args.public_key)

    run(args.cleos + 'system newaccount --stake-net "10.0000 %s" --stake-cpu "10.0000 %s" --buy-ram-kbytes 80 eosio '%(args.symbol,args.symbol)  + 'cochaintoken' + ' ' + args.public_key)

    run(args.cleos + 'system buyram eosio %s -k 80000 -p eosio ' % args.contract )
    run(args.cleos + 'system  delegatebw eosio %s "1000.0000 SYS" "1000.0000 SYS"'% args.contract )

    run(args.cleos + 'system buyram eosio %s -k 80000 -p eosio ' % args.contract2 )
    run(args.cleos + 'system  delegatebw eosio %s "1000.0000 SYS" "1000.0000 SYS"'% args.contract2 )

#     stepIssueToken()
#
#
# def stepIssueToken():
#     run(args.cleos + 'push action eosio.token issue \'["eosio", "%s %s", "memo"]\' -p eosio' % ("1000000.0000", args.symbol))
#     for i in accounts:
#         run(args.cleos + 'push action eosio.token issue \'["%s", "%s %s", "memo"]\' -p eosio' % (i, "1000000.0000", args.symbol))
#
#     sleep(1)


def stepKillAll():
    run('killall keosd nodeos || true')
    sleep(1.5)
# Command Line Arguments

def stepInitCaee():
    print ("===========================    set contract caee   ===========================" )
    run(args.cleos + 'set contract %s ../actiondemo' %args.contract )
    run(args.cleos + 'set contract %s ../actiondemo' %args.contract2 )
    run(args.cleos + 'set account permission %s active \'{"threshold": 1,"keys": [{"key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","weight": 1}],"accounts": [{"permission":{"actor":"%s","permission":"eosio.code"},"weight":1}]}\' ' % (args.contract,args.contract))
    print ("sleep 5")


def stepClear():
    print ("===========================    set contract clear   ===========================" )
    run(args.cleos + 'push action %s clear "[]" -p %s ' %(args.contract, args.contract))
    run(args.cleos + 'get table %s %s seedobjs'  %(args.contract, args.contract) )
    run(args.cleos + 'push action %s clear "[]" -p %s ' %(args.contract2, args.contract2))
    run(args.cleos + 'get table %s %s seedobjs'  %(args.contract2, args.contract2) )
    print ("sleep 5")


def stepGenerate():
    print ("===========================    set contract stepGenerate   ===========================" )
    # run(args.cleos + 'push action %s generate \'[{"loop":1, "num":1}]\' -p %s ' %(args.contract, args.contract))
    run(args.cleos + 'push action %s inlineact \'[{"payer":"%s", "in":"%s"}]\' -p %s ' %(args.contract,args.contract,args.contract2, args.contract))
    run(args.cleos + 'get table %s %s seedobjs'  %(args.contract, args.contract) )
    run(args.cleos + 'get table %s %s seedobjs'  %(args.contract2, args.contract2) )
    print ("sleep 5")


parser = argparse.ArgumentParser()

commands = [
    ('k', 'kill',         stepKillAll,            True,    ""),
    ('w', 'wallet',         stepStartWallet,            True,    "Start keosd, create wallet"),
    ('s', 'sys',            createAccounts,             True,    "Create all accounts"),
    ('i', 'init',      stepInitCaee,                      True,         "stepInitCaee"),
    ('c', 'clear',      stepClear,                      True,         "stepInitCaee"),
    ('g', 'generate',      stepGenerate,                True,         "stepInitCaee"),
]

parser.add_argument('--public-key', metavar='', help="EOSIO Public Key", default='EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV', dest="public_key")
parser.add_argument('--private-Key', metavar='', help="EOSIO Private Key", default='5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3', dest="private_key")
parser.add_argument('--cleos', metavar='', help="Cleos command", default='../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 ')
parser.add_argument('--nodeos', metavar='', help="Path to nodeos binary", default='../../build/programs/nodeos/nodeos ')
parser.add_argument('--keosd', metavar='', help="Path to keosd binary", default='../../build/programs/keosd/keosd ')
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default='../../build/contracts/')
parser.add_argument('--nodes-dir', metavar='', help="Path to nodes diretodctory", default='./')
parser.add_argument('--genesis', metavar='', help="Path to genesis.json", default="./genesis.json")
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')
# parser.add_argument('--symbol', metavar='', help="The eosio.system symbol", default='SYS')
parser.add_argument('-a', '--all', action='store_true', help="Do everything marked with (*)")
#parser.add_argument('-H', '--http-port', type=int, default=8888, metavar='', help='HTTP port for cleos')

for (flag, command, function, inAll, help) in commands:
    prefix = ''
    if inAll: prefix += '*'
    if prefix: help = '(' + prefix + ') ' + help
    if flag:
        parser.add_argument('-' + flag, '--' + command, action='store_true', help=help, dest=command)
    else:
        parser.add_argument('--' + command, action='store_true', help=help, dest=command)

args = parser.parse_args()

args.cleos += '--url http://127.0.0.1:8888 '
args.symbol = 'SYS'
args.contract = 'caee'
args.contract2 = 'caee2'


accnum = 26
accounts = []
# for i in range(97,97+accnum):
#     accounts.append("user%c"% chr(i))
# accounts.append("payman")
accounts.append(args.contract)
accounts.append(args.contract2)

logFile = open(args.log_path, 'a')
logFile.write('\n\n' + '*' * 80 + '\n\n\n')

haveCommand = False
for (flag, command, function, inAll, help) in commands:
    if getattr(args, command) or inAll and args.all:
        if function:
            haveCommand = True
            function()
if not haveCommand:
    print('testtool.py: Tell me what to do. -a does almost everything. -h shows options.')