#!/usr/bin/env python3

import argparse
import json
import os
import random
import re
import subprocess
import sys
import time
import requests

args = None
logFile = None

unlockTimeout = 999999999
fastUnstakeSystem = './fast.refund/rem.system/rem.system.wasm'

systemAccounts = [
    'rem.bpay',
    'rem.msig',
    'rem.names',
    'rem.ram',
    'rem.ramfee',
    'rem.saving',
    'rem.stake',
    'rem.token',
    'rem.vpay',
    'rem.rex',
    'rem.swap',
    'rem.oracle',
    'rem.swap.bot',
    'rewards',
]

def jsonArg(a):
    return " '" + json.dumps(a) + "' "

def run(args: object) -> object:
    print('bios-boot-tutorial.py:', args)
    logFile.write(args + '\n')
    if subprocess.call(args, shell=True):
        print('bios-boot-tutorial.py: exiting because of error')
        sys.exit(1)

def retry(args):
    while True:
        print('bios-boot-tutorial.py:', args)
        logFile.write(args + '\n')
        if subprocess.call(args, shell=True):
            print('*** Retry')
        else:
            break

def background(args):
    print('bios-boot-tutorial.py:', args)
    logFile.write(args + '\n')
    return subprocess.Popen(args, shell=True)

def getOutput(args):
    print('bios-boot-tutorial.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return proc.communicate()[0].decode('utf-8')

def getJsonOutput(args):
    return json.loads(getOutput(args))

def sleep(t):
    print('sleep', t, '...')
    time.sleep(t)
    print('resume')

def startWallet():
    run('rm -rf ' + os.path.abspath(args.wallet_dir))
    run('mkdir -p ' + os.path.abspath(args.wallet_dir))
    background(args.remvault + ' --unlock-timeout %d --http-server-address 127.0.0.1:6666 --wallet-dir %s' % (unlockTimeout, os.path.abspath(args.wallet_dir)))
    sleep(.4)
    run(args.remcli + 'wallet create --to-console')

def importKeys():
    run(args.remcli + 'wallet import --private-key ' + args.private_key)
    keys = {}
    for a in accounts:
        key = a['pvt']
        if not key in keys:
            if len(keys) >= args.max_user_keys:
                break
            keys[key] = True
            run(args.remcli + 'wallet import --private-key ' + key)
    for i in range(firstProducer, firstProducer + numProducers):
        a = accounts[i]
        key = a['pvt']
        if not key in keys:
            keys[key] = True
            run(args.remcli + 'wallet import --private-key ' + key)

def startNode(nodeIndex, account):
    dir = args.nodes_dir + ('%02d-' % nodeIndex) + account['name'] + '/'
    run('rm -rf ' + dir)
    run('mkdir -p ' + dir)
    otherOpts = ''.join(list(map(lambda i: '    --p2p-peer-address localhost:' + str(9000 + i), range(nodeIndex))))
    if not nodeIndex: otherOpts += (
        '    --plugin eosio::history_plugin'
        '    --plugin eosio::history_api_plugin'
    )
    swap_and_oracle_opts = ''
    if account['name'] != 'rem':
        swap_and_oracle_opts = (
                '    --plugin eosio::eth_swap_plugin'
                '    --plugin eosio::rem_oracle_plugin'
                '    --oracle-authority ' + account['name'] + '@active' +
                '    --oracle-signing-key ' + account['pvt'] +
                '    --swap-signing-key ' + account['pvt'] +
                '    --swap-authority ' + account['name'] + '@active'
        )
    if args.cryptocompare_apikey:
        swap_and_oracle_opts += (
            '    --cryptocompare-apikey ' + args.cryptocompare_apikey
        )
    if args.eth_wss_provider:
        swap_and_oracle_opts += (
            '    --eth-wss-provider ' + args.eth_wss_provider
        )
    cmd = (
        args.remnode +
        '    --max-irreversible-block-age -1'
        '    --contracts-console'
        '    --genesis-json ' + os.path.abspath(args.genesis) +
        '    --blocks-dir ' + os.path.abspath(dir) + '/blocks'
        '    --config-dir ' + os.path.abspath(dir) +
        '    --data-dir ' + os.path.abspath(dir) +
        '    --chain-state-db-size-mb 1024'
        '    --http-server-address 127.0.0.1:' + str(8888 + nodeIndex) +
        '    --p2p-listen-endpoint 127.0.0.1:' + str(9000 + nodeIndex) +
        '    --max-clients ' + str(maxClients) +
        '    --p2p-max-nodes-per-host ' + str(maxClients) +
        '    --enable-stale-production'
        '    --producer-name ' + account['name'] +
        '    --private-key \'["' + account['pub'] + '","' + account['pvt'] + '"]\''
        '    --access-control-allow-origin=\'*\''
        '    --plugin eosio::http_plugin'
        '    --plugin eosio::chain_api_plugin'
        '    --plugin eosio::producer_api_plugin'
        '    --plugin eosio::producer_plugin' + swap_and_oracle_opts +
        otherOpts)
    with open(dir + 'stderr', mode='w') as f:
        f.write(cmd + '\n\n')
    background(cmd + '    2>>' + dir + 'stderr')
    sleep(2)
    run(f'curl -X POST http://127.0.0.1:{8888+nodeIndex}/v1/producer/schedule_protocol_feature_activations -d \'{{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}}\'')

def startProducers(b, e):
    for i in range(b, e):
        startNode(i - b + 1, accounts[i])

def createSystemAccounts():
    run(f'curl -X POST http://127.0.0.1:{args.http_port}/v1/producer/schedule_protocol_feature_activations -d \'{{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}}\'')

    for a in systemAccounts:
        run(args.remcli + 'create account rem ' + a + ' ' + args.public_key)

    run(args.remcli + 'set account permission rem.swap.bot bot ' + jsonArg({
        'threshold': 1,
        'keys': [{"key": args.rem_swap_bot_public_key, "weight": 1}],
    }) + "active")
    run(args.remcli + 'push action rem linkauth ' + jsonArg({
        'account': 'rem.swap.bot',
        'code': 'rem.swap',
        'type': 'finish',
        'requirement': 'bot',
    }) + " -p rem.swap.bot@active")
    run(args.remcli + 'push action rem linkauth ' + jsonArg({
        'account': 'rem.swap.bot',
        'code': 'rem.swap',
        'type': 'finishnewacc',
        'requirement': 'bot',
    }) + " -p rem.swap.bot@active")

def intToCurrency(i):
    return '%d.%04d %s' % (i // 10000, i % 10000, args.symbol)

def allocateFunds(b, e, total):
    funds = total // (e - b)
    for i in range(b, e):
        accounts[i]['funds'] = funds
    return total

def createStakedAccounts(b, e):
    stakePercent = args.stake_percent
    for i in range(b, e):
        a = accounts[i]
        funds = a['funds']
        stake = funds*stakePercent
        unstaked = funds - stake
        retry(args.remcli + 'system newaccount --transfer rem %s %s --stake "%s"   ' %
            (a['name'], a['pub'], intToCurrency(stake)))
        if unstaked:
            retry(args.remcli + 'transfer rem %s "%s"' % (a['name'], intToCurrency(unstaked)))

def regProducers(b, e):
    for i in range(b, e):
        a = accounts[i]
        retry(args.remcli + 'system regproducer ' + a['name'] + ' ' + a['pub'] + ' https://' + a['name'] + '.com' + '/' + a['pub'])

def listProducers():
    run(args.remcli + 'system listproducers')

def vote():
    for i in range(firstProducer, firstProducer + numProducers):
        retry(args.remcli + 'system voteproducer prods ' + accounts[i]['name'] + ' ' + accounts[i]['name'])

def claimRewards():
    table = getJsonOutput(args.remcli + 'get table rem rem producers -l 100')
    times = []
    for row in table['rows']:
        if row['unpaid_blocks'] and not row['last_claim_time']:
            times.append(getJsonOutput(args.remcli + 'system claimrewards -j ' + row['owner'])['processed']['elapsed'])
    print('Elapsed time for claimrewards:', times)

def proxyVotes(b, e):
    vote(firstProducer, firstProducer + 1)
    proxy = accounts[firstProducer]['name']
    retry(args.remcli + 'system regproxy ' + proxy)
    sleep(1.0)
    for i in range(firstProducer + 1, firstProducer + numProducers):
        voter = accounts[i]['name']
        retry(args.remcli + 'system voteproducer proxy ' + voter + ' ' + proxy)

def updateAuth(account, permission, parent, controller):
    run(args.remcli + 'push action rem updateauth' + jsonArg({
        'account': account,
        'permission': permission,
        'parent': parent,
        'auth': {
            'threshold': 1, 'keys': [], 'waits': [],
            'accounts': [{
                'weight': 1,
                'permission': {'actor': controller, 'permission': 'active'}
            }]
        }
    }) + '-p ' + account + '@' + permission)

def resign(account, controller):
    updateAuth(account, 'owner', '', controller)
    updateAuth(account, 'active', 'owner', controller)
    sleep(1)
    run(args.remcli + 'get account ' + account)

def randomTransfer(b, e):
    for j in range(20):
        src = accounts[random.randint(b, e - 1)]['name']
        dest = src
        while dest == src:
            dest = accounts[random.randint(b, e - 1)]['name']
        run(args.remcli + 'transfer -f ' + src + ' ' + dest + ' "0.0001 ' + args.symbol + '"' + ' || true')

def msigProposeReplaceSystem(proposer, proposalName):
    requestedPermissions = []
    for i in range(firstProducer, firstProducer + numProducers):
        requestedPermissions.append({'actor': accounts[i]['name'], 'permission': 'active'})
    trxPermissions = [{'actor': 'rem', 'permission': 'active'}]
    with open(fastUnstakeSystem, mode='rb') as f:
        setcode = {'account': 'rem', 'vmtype': 0, 'vmversion': 0, 'code': f.read().hex()}
    run(args.remcli + 'multisig propose ' + proposalName + jsonArg(requestedPermissions) +
        jsonArg(trxPermissions) + 'rem setcode' + jsonArg(setcode) + ' -p ' + proposer)

def msigApproveReplaceSystem(proposer, proposalName):
    for i in range(firstProducer, firstProducer + numProducers):
        run(args.remcli + 'multisig approve ' + proposer + ' ' + proposalName +
            jsonArg({'actor': accounts[i]['name'], 'permission': 'active'}) +
            '-p ' + accounts[i]['name'])

def msigExecReplaceSystem(proposer, proposalName):
    retry(args.remcli + 'multisig exec ' + proposer + ' ' + proposalName + ' -p ' + proposer)

def msigReplaceSystem():
    msigProposeReplaceSystem(accounts[0]['name'], 'fast.unstake')
    sleep(1)
    msigApproveReplaceSystem(accounts[0]['name'], 'fast.unstake')
    msigExecReplaceSystem(accounts[0]['name'], 'fast.unstake')

def produceNewAccounts():
    with open('newusers', 'w') as f:
        for i in range(120_000, 200_000):
            x = getOutput(args.remcli + 'create key --to-console')
            r = re.match('Private key: *([^ \n]*)\nPublic key: *([^ \n]*)', x, re.DOTALL | re.MULTILINE)
            name = 'user'
            for j in range(7, -1, -1):
                name += chr(ord('a') + ((i >> (j * 4)) & 15))
            print(i, name)
            f.write('        {"name":"%s", "pvt":"%s", "pub":"%s"},\n' % (name, r[1], r[2]))

def stepKillAll():
    run('killall remvault remnode || true')
    sleep(1.5)
def stepStartWallet():
    startWallet()
    importKeys()
def stepStartBoot():
    startNode(0, {'name': 'rem', 'pvt': args.private_key, 'pub': args.public_key})
    sleep(1.5)
def stepInstallSystemContracts():
    run(args.remcli + 'set contract rem.token ' + args.contracts_dir + '/rem.token/')
    run(args.remcli + 'set contract rem.msig ' + args.contracts_dir + '/rem.msig/')
    run(args.remcli + 'set contract rem.swap ' + args.contracts_dir + '/rem.swap/')
    run(args.remcli + 'set contract rem.oracle ' + args.contracts_dir + '/rem.oracle/')
def stepCreateTokens():
    max_supply = 1_000_000_000_0000
    run(args.remcli + 'push action rem.token create \'["rem.swap", "%s"]\' -p rem.token' % intToCurrency(max_supply))
    if len(accounts) != 0 and args.initial_supply != 0:
        allocateFunds(0, len(accounts), args.initial_supply)
    if args.initial_supply != 0:
        run(args.remcli + 'push action rem.token issue \'["rem.swap", "%s", "memo"]\' -p rem.swap' % intToCurrency(args.initial_supply))
    sleep(1)
def stepSetSystemContract():
    run(args.remcli + 'set contract rem ' + args.contracts_dir + '/rem.bios/ -p rem')
    retry(args.remcli + 'push action rem activate \'["f0af56d2c5a48d60a4a5b5c903edfb7db3a736a94ed589d0b797df33ff9d3e1d"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["2652f5f96006294109b3dd0bbde63693f55324af452b799ee137a81a905eed25"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["8ba52fe7a3956c5cd3a656a3174b931d3bb2abb45578befc59f283ecd816a405"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["ad9e3d8f650687709fd68f4b90b41f7d825a365b02c23a636cef88ac2ac00c43"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["68dcaa34c0517d19666e6b33add67351d8c5f69e999ca1e37931bc410a297428"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["e0fb64b1085cc5538970158d05a009c24e276fb94e1a0bf6a528b48fbc4ff526"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["ef43112c6543b88db2283a2e077278c315ae2c84719a8b25f25cc88565fbea99"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["4a90c00d55454dc5b059055ca213579c6ea856967712a56017487886a4d4cc0f"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["1a99a59d87e06e09ec5b028a9cbb7749b4a5ad8819004365d02dc4379a8b7241"]\' -p rem')
    retry(args.remcli + 'push action rem activate \'["4e7bf348da00a945489b2a681749eb56f5de00b900014e137ddae39f48f69d67"]\' -p rem')

    retry(args.remcli + 'set contract rem ' + args.contracts_dir + '/rem.system/')
    sleep(1)
    run(args.remcli + 'push action rem setpriv' + jsonArg(['rem.msig', 1]) + '-p rem@active')
def stepInitSystemContract():
    pass
def stepCreateStakedAccounts():
    createStakedAccounts(0, len(accounts))
def stepRegProducers():
    regProducers(firstProducer, firstProducer + numProducers)
    sleep(1)
    listProducers()
def stepStartProducers():
    startProducers(firstProducer, firstProducer + numProducers)
    sleep(args.producer_sync_delay)
def stepVote():
    vote()
    sleep(1)
    listProducers()
    sleep(5)
def stepResign():
    resign('rem', 'rem.prods')
    for a in systemAccounts:
        resign(a, 'rem')
def stepTransfer():
    while True:
        randomTransfer(0, args.num_senders)
def stepLog():
    run('tail -n 60 ' + args.nodes_dir + '00-rem/stderr')

# Command Line Arguments

parser = argparse.ArgumentParser()

commands = [
    ('k', 'kill',               stepKillAll,                True,    "Kill all remnode and remvault processes"),
    ('w', 'wallet',             stepStartWallet,            True,    "Start remvault, create wallet, fill with keys"),
    ('b', 'boot',               stepStartBoot,              True,    "Start boot node"),
    ('s', 'sys',                createSystemAccounts,       True,    "Create system accounts (rem.*)"),
    ('c', 'contracts',          stepInstallSystemContracts, True,    "Install system contracts (token, msig)"),
    ('t', 'tokens',             stepCreateTokens,           True,    "Create tokens"),
    ('S', 'sys-contract',       stepSetSystemContract,      True,    "Set system contract"),
    ('I', 'init-sys-contract',  stepInitSystemContract,     True,    "Initialiaze system contract"),
    ('T', 'stake',              stepCreateStakedAccounts,   True,    "Create staked accounts"),
    ('p', 'reg-prod',           stepRegProducers,           True,    "Register producers"),
    ('P', 'start-prod',         stepStartProducers,         True,    "Start producers"),
    ('v', 'vote',               stepVote,                   True,    "Vote for producers"),
    ('R', 'claim',              claimRewards,               True,    "Claim rewards"),
    ('q', 'resign',             stepResign,                 True,    "Resign rem"),
    ('m', 'msg-replace',        msigReplaceSystem,          False,   "Replace system contract using msig"),
    ('X', 'xfer',               stepTransfer,               False,   "Random transfer tokens (infinite loop)"),
    ('l', 'log',                stepLog,                    True,    "Show tail of node's log"),
]

parser.add_argument('--public-key', metavar='', help="EOSIO Public Key", default='EOS8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr', dest="public_key")
parser.add_argument('--private-key', metavar='', help="EOSIO Private Key", default='5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p', dest="private_key")
parser.add_argument('--remcli', metavar='', help="Cleos command", default='../../build/programs/remcli/remcli --wallet-url http://127.0.0.1:6666 ')
parser.add_argument('--remnode', metavar='', help="Path to remnode binary", default='../../build/programs/remnode/remnode')
parser.add_argument('--remvault', metavar='', help="Path to remvault binary", default='../../build/programs/remvault/remvault')
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default='../../build/contracts/contracts')
parser.add_argument('--nodes-dir', metavar='', help="Path to nodes directory", default='./nodes/')
parser.add_argument('--genesis', metavar='', help="Path to genesis.json", default="./genesis.json")
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')
parser.add_argument('--symbol', metavar='', help="The rem.system symbol", default='REM')
parser.add_argument('--user-limit', metavar='', help="Max number of users", type=int, default=0)
parser.add_argument('--max-user-keys', metavar='', help="Maximum user keys to import into wallet", type=int, default=0)
parser.add_argument('--stake-percent', metavar='', help="Stake percent for each account", type=float, default=0.5)
parser.add_argument('--producer-limit', metavar='', help="Maximum number of producers", type=int, default=0)
parser.add_argument('--producer-sync-delay', metavar='', help="Time (s) to sleep to allow producers to sync", type=int, default=15)
parser.add_argument('-a', '--all', action='store_true', help="Do everything marked with (*)")
parser.add_argument('-H', '--http-port', type=int, default=8888, metavar='', help='HTTP port for remcli')
parser.add_argument('-bs', '--bootstrap', action='store_true', help="Deploy all remprocol contracts and configure all tech accounts")
parser.add_argument('--cryptocompare-apikey', metavar='', help="Cryptocompare api key for reading REM token price", type=str, default="")
parser.add_argument('--eth-wss-provider', metavar='', help="A websocket link to Ethereum node", type=str, default="")
parser.add_argument('--rem-swap-bot-public-key', metavar='', help="A public key to authorize finish swap action", type=str, default="EOS7tAF35d4cLH1TwF6it5TkK2ezkpwpjJcTWJKAQVHsFon4YdGUp")
parser.add_argument('--initial-supply', metavar='', help="Initial supply of REM tokens", type=int, default=0)

for (flag, command, function, inAll, help) in commands:
    prefix = ''
    if inAll: prefix += '*'
    if prefix: help = '(' + prefix + ') ' + help
    if flag:
        parser.add_argument('-' + flag, '--' + command, action='store_true', help=help, dest=command)
    else:
        parser.add_argument('--' + command, action='store_true', help=help, dest=command)

args = parser.parse_args()

args.remcli += '--url http://127.0.0.1:%d ' % args.http_port

logFile = open(args.log_path, 'a')

logFile.write('\n\n' + '*' * 80 + '\n\n\n')

with open('accounts.json') as f:
    a = json.load(f)
    del a['users'][args.user_limit:]
    del a['producers'][args.producer_limit:]
    firstProducer = len(a['users'])
    numProducers = len(a['producers'])
    accounts = a['users'] + a['producers']

maxClients = numProducers + 10
commands = [
    ('k', 'kill',               stepKillAll,                True,    "Kill all remnode and remvault processes"),
    ('w', 'wallet',             stepStartWallet,            True,    "Start remvault, create wallet, fill with keys"),
    ('b', 'boot',               stepStartBoot,              True,    "Start boot node"),
    ('s', 'sys',                createSystemAccounts,       True,    "Create system accounts (rem.*)"),
    ('c', 'contracts',          stepInstallSystemContracts, True,    "Install system contracts (token, msig)"),
    ('t', 'tokens',             stepCreateTokens,           True,    "Create tokens"),
    ('S', 'sys-contract',       stepSetSystemContract,      True,    "Set system contract"),
    ('I', 'init-sys-contract',  stepInitSystemContract,     True,    "Initialiaze system contract"),
    ('T', 'stake',              stepCreateStakedAccounts,   True,    "Create staked accounts"),
    ('p', 'reg-prod',           stepRegProducers,           True,    "Register producers"),
    ('P', 'start-prod',         stepStartProducers,         True,    "Start producers"),
    ('v', 'vote',               stepVote,                   True,    "Vote for producers"),
    ('R', 'claim',              claimRewards,               True,    "Claim rewards"),
    ('q', 'resign',             stepResign,                 True,    "Resign rem"),
    ('m', 'msg-replace',        msigReplaceSystem,          False,   "Replace system contract using msig"),
    ('X', 'xfer',               stepTransfer,               False,   "Random transfer tokens (infinite loop)"),
    ('l', 'log',                stepLog,                    True,    "Show tail of node's log"),
]
haveCommand = False
for (flag, command, function, inAll, help) in commands:
    if getattr(args, command) or inAll and args.all:
        if function:
            haveCommand = True
            function()
    if args.bootstrap and command in ('sys', 'contracts', 'tokens', 'sys-contract', 'init-sys-contract', 'log',):
        if function:
            haveCommand = True
            function()
if not haveCommand:
    print('bios-boot-tutorial.py: Tell me what to do. -a does almost everything. -h shows options.')
