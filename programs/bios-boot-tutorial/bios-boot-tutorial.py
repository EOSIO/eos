#!/usr/bin/env python3

import json
import numpy
import os
import random
import re
import subprocess
import sys
import time

genesis = './genesis.json'
walletDir = os.path.abspath('./wallet/')
unlockTimeout = 99999999999
nodesDir = './nodes/'
contractsDir = '../../build/contracts/'
cleos = 'cleos --wallet-url http://localhost:6666 --url http://localhost:8000 '
nodeos = 'nodeos'
fastUnstakeSystem = './fast.refund/eosio.system/eosio.system.wasm'
logFile = open('test.log', 'a')

limitUsers = 0
limitProducers = 0
numProducersToVoteFor = 20
numVoters = 10
symbol = 'SYS'

eosioPub = 'EOS8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr'
eosioPvt = '5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p'
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

with open('accounts.json') as f:
    a = json.load(f)
    if limitUsers:
        del a['users'][limitUsers:]
    if limitProducers:
        del a['producers'][limitProducers:]
    firstProducer = len(a['users'])
    numProducers = len(a['producers'])
    accounts = a['users'] + a['producers']

maxClients = numProducers + 10

def jsonArg(a):
    return " '" + json.dumps(a) + "' "

def run(args):
    print('test.py:', args)
    logFile.write(args + '\n')
    if subprocess.call(args, shell=True):
        print('test.py: exiting because of error')
        sys.exit(1)

def retry(args):
    while True:
        print('test.py:', args)
        logFile.write(args + '\n')
        if subprocess.call(args, shell=True):
            print('*** Retry')
        else:
            break

def background(args):
    print('test.py:', args)
    logFile.write(args + '\n')
    return subprocess.Popen(args, shell=True)

def getJsonOutput(args):
    print('test.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return json.loads(proc.communicate()[0])

def startWallet():
    run('rm -rf ' + walletDir)
    run('mkdir -p ' + walletDir)
    background('keosd --unlock-timeout %d --http-server-address 127.0.0.1:6666 --wallet-dir %s' % (unlockTimeout, walletDir))
    time.sleep(.4)
    run(cleos + 'wallet create')

def importKeys():
    run(cleos + 'wallet import ' + eosioPvt)
    keys = {}
    for a in accounts:
        key = a['pvt']
        if not key in keys:
            keys[key] = True
            run(cleos + 'wallet import ' + key)

def startNode(nodeIndex, account):
    dir = nodesDir + ('%02d-' % nodeIndex) + account['name'] + '/'
    run('rm -rf ' + dir)
    run('mkdir -p ' + dir)
    otherOpts = ''.join(list(map(lambda i: '    --p2p-peer-address localhost:' + str(9000 + i), range(nodeIndex))))
    if not nodeIndex: otherOpts += (
        '    --plugin eosio::history_plugin'
        '    --plugin eosio::history_api_plugin'
    )
    cmd = (
        nodeos +
        '    --max-transaction-time 200'
        '    --contracts-console'
        '    --genesis-json ' + genesis +
        '    --blocks-dir ' + os.path.abspath(dir) + '/blocks'
        '    --config-dir ' + os.path.abspath(dir) +
        '    --data-dir ' + os.path.abspath(dir) +
        '    --chain-state-db-size-mb 1024'
        '    --http-server-address 127.0.0.1:' + str(8000 + nodeIndex) +
        '    --p2p-listen-endpoint 127.0.0.1:' + str(9000 + nodeIndex) +
        '    --max-clients ' + str(maxClients) +
        '    --enable-stale-production'
        '    --producer-name ' + account['name'] +
        '    --private-key \'["' + account['pub'] + '","' + account['pvt'] + '"]\''
        '    --plugin eosio::http_plugin'
        '    --plugin eosio::chain_api_plugin'
        '    --plugin eosio::producer_plugin' +
        otherOpts)
    with open(dir + 'stderr', mode='w') as f:
        f.write(cmd + '\n\n')
    background(cmd + '    2>>' + dir + 'stderr')

def startProducers(b, e):
    for i in range(b, e):
        startNode(i - b + 1, accounts[i])

def createSystemAccounts():
    for a in systemAccounts:
        run(cleos + 'create account eosio ' + a + ' ' + eosioPub)

def intToCurrency(i):
    return '%d.%04d %s' % (i // 1000, i % 1000, symbol)

def fillStake(b, e):
    dist = numpy.random.pareto(1.161, e - b).tolist() # 1.161 = 80/20 rule
    dist.sort()
    dist.reverse()
    factor = 1_000_000_000 / sum(dist)
    total = 0
    for i in range(b, e):
        stake = round(factor * dist[i - b] * 1000 / 2)
        total += stake * 2
        accounts[i]['stake'] = stake
    return total

def createStakedAccounts(b, e):
    for i in range(b, e):
        a = accounts[i]
        stake = intToCurrency(a['stake'])
        run(cleos + 'system newaccount eosio --transfer ' + a['name'] + ' ' + a['pub'] + ' --stake-net "' + stake + '" --stake-cpu "' + stake + '"')

def regProducers(b, e):
    for i in range(b, e):
        a = accounts[i]
        retry(cleos + 'system regproducer ' + a['name'] + ' ' + a['pub'] + ' https://' + a['name'] + '.com' + '/' + a['pub'])

def listProducers():
    run(cleos + 'system listproducers')

def vote(b, e):
    for i in range(b, e):
        voter = accounts[i]['name']
        prods = random.sample(range(firstProducer, firstProducer + numProducers), numProducersToVoteFor)
        prods = ' '.join(map(lambda x: accounts[x]['name'], prods))
        retry(cleos + 'system voteproducer prods ' + voter + ' ' + prods)

def claimRewards():
    table = getJsonOutput(cleos + 'get table eosio eosio producers -l 100')
    for row in table['rows']:
        if row['unpaid_blocks'] and not row['last_claim_time']:
            run(cleos + 'system claimrewards ' + row['owner'])

def vote(b, e):
    for i in range(b, e):
        voter = accounts[i]['name']
        prods = random.sample(range(firstProducer, firstProducer + numProducers), numProducersToVoteFor)
        prods = ' '.join(map(lambda x: accounts[x]['name'], prods))
        retry(cleos + 'system voteproducer prods ' + voter + ' ' + prods)

def proxyVotes(b, e):
    vote(firstProducer, firstProducer + 1)
    proxy = accounts[firstProducer]['name']
    retry(cleos + 'system regproxy ' + proxy)
    time.sleep(1.0)
    for i in range(b, e):
        voter = accounts[i]['name']
        retry(cleos + 'system voteproducer proxy ' + voter + ' ' + proxy)

def updateAuth(account, permission, parent, controller):
    run(cleos + 'push action eosio updateauth' + jsonArg({
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
    time.sleep(1)
    run(cleos + 'get account ' + account)

def sendUnstakedFunds(b, e):
    for i in range(b, e):
        a = accounts[i]
        run(cleos + 'transfer eosio ' + a['name'] + ' "10.0000 ' + symbol + '"')

def randomTransfer(b, e, n):
    for i in range(n):
        for j in range(20):
            src = accounts[random.randint(b, e - 1)]['name']
            dest = src
            while dest == src:
                dest = accounts[random.randint(b, e - 1)]['name']
            run(cleos + 'transfer ' + src + ' ' + dest + ' "0.0001 ' + symbol + '"' + ' || true')
        time.sleep(.25)

def msigProposeReplaceSystem(proposer, proposalName):
    requestedPermissions = []
    for i in range(firstProducer, firstProducer + numProducers):
        requestedPermissions.append({'actor': accounts[i]['name'], 'permission': 'active'})
    trxPermissions = [{'actor': 'eosio', 'permission': 'active'}]
    with open(fastUnstakeSystem, mode='rb') as f:
        setcode = {'account': 'eosio', 'vmtype': 0, 'vmversion': 0, 'code': f.read().hex()}
    run(cleos + 'multisig propose ' + proposalName + jsonArg(requestedPermissions) + 
        jsonArg(trxPermissions) + 'eosio setcode' + jsonArg(setcode) + ' -p ' + proposer)

def msigApproveReplaceSystem(proposer, proposalName):
    for i in range(firstProducer, firstProducer + numProducers):
        run(cleos + 'multisig approve ' + proposer + ' ' + proposalName +
            jsonArg({'actor': accounts[i]['name'], 'permission': 'active'}) +
            '-p ' + accounts[i]['name'])

def msigExecReplaceSystem(proposer, proposalName):
    retry(cleos + 'multisig exec ' + proposer + ' ' + proposalName + ' -p ' + proposer)

def msigReplaceSystem():
    run(cleos + 'push action eosio buyrambytes' + jsonArg(['eosio', accounts[0]['name'], 200000]) + '-p eosio')
    time.sleep(1)
    msigProposeReplaceSystem(accounts[0]['name'], 'fast.unstake')
    time.sleep(1)
    msigApproveReplaceSystem(accounts[0]['name'], 'fast.unstake')
    msigExecReplaceSystem(accounts[0]['name'], 'fast.unstake')

logFile.write('\n\n' + '*' * 80 + '\n\n\n')
run('killall keosd nodeos || true')
time.sleep(1.5)
startWallet()
importKeys()
startNode(0, {'name': 'eosio', 'pvt': eosioPvt, 'pub': eosioPub})
time.sleep(1.5)
createSystemAccounts()
run(cleos + 'set contract eosio.token ' + contractsDir + 'eosio.token/')
run(cleos + 'set contract eosio.msig ' + contractsDir + 'eosio.msig/')
run(cleos + 'push action eosio.token create \'["eosio", "10000000000.0000 %s"]\' -p eosio.token' % (symbol))
totalAllocation = fillStake(0, len(accounts))
run(cleos + 'push action eosio.token issue \'["eosio", "%s", "memo"]\' -p eosio' % intToCurrency(totalAllocation))
time.sleep(1)
retry(cleos + 'set contract eosio ' + contractsDir + 'eosio.system/')
time.sleep(1)
run(cleos + 'push action eosio setpriv' + jsonArg(['eosio.msig', 1]) + '-p eosio@active')
createStakedAccounts(0, len(accounts))
regProducers(firstProducer, firstProducer + numProducers)
time.sleep(1)
listProducers()
startProducers(firstProducer, firstProducer + numProducers)
time.sleep(2)
vote(0, 0 + numVoters)
time.sleep(1)
listProducers()
time.sleep(5)
claimRewards()
proxyVotes(0, 0 + numVoters)
resign('eosio', 'eosio.prods')
for a in systemAccounts:
    resign('eosio.msig', 'eosio')
# msigReplaceSystem()
run(cleos + 'push action eosio.token issue \'["eosio", "%d.0000 %s", "memo"]\' -p eosio' % ((len(accounts)) * 10, symbol))
sendUnstakedFunds(0, len(accounts))
randomTransfer(0, len(accounts), 8)
run('tail -n 60 ' + nodesDir + '00-eosio/stderr')
