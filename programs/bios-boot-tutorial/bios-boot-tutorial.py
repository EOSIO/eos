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
cleos = '../../build/programs/cleos/cleos --wallet-url http://localhost:6666 --url http://localhost:8000 '
nodeos = '../../build/programs/nodeos/nodeos'
logFile = open('test.log', 'a')

with open('accounts.json') as f:
    accounts = json.load(f)

symbol = 'SYS'
numSysAccounts = 3
firstRegAccount = numSysAccounts
numProducers = 30
firstProducer = len(accounts) - numProducers
numProducersToVoteFor = 20
numVoters = 10
maxClients = numProducers + 1

def run(args):
    print('test.py:', args)
    logFile.write(args + '\n')
    if subprocess.call(args, shell=True):
        print('test.py: exiting because of error')
        sys.exit(1)

def retry(args):
    while True:
        try:
            run(args)
            break
        except:
            print('*** Retry')
            pass

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
    keys = {}
    for i in range(2, len(accounts)):
        key = accounts[i]['pvt']
        if not key in keys:
            keys[key] = True
            run(cleos + 'wallet import ' + key)

def startProducers(b, e):
    for i in range(b, e):
        account = accounts[i]
        dir = nodesDir + account['name'] + '/'
        run('rm -rf ' + dir)
        run('mkdir -p ' + dir)
        otherProds = [j for j in range(firstProducer, firstProducer + numProducers) if j < i]
        if i:
            otherProds.append(0)
        otherOpts = ''.join(list(map(lambda i: '    --p2p-peer-address localhost:' + str(9000 + i), otherProds)))
        if not i: otherOpts += (
            '    --plugin eosio::history_plugin'
            '    --plugin eosio::history_api_plugin'
        )
        portOffset = 0 if not i else i - firstProducer + 1
        cmd = (
            nodeos +
            '    --genesis-json ' + genesis +
            '    --block-log-dir ' + dir + 'blocks'
            '    --config-dir ' + dir +
            '    --data-dir ' + dir +
            '    --shared-memory-size-mb 1024'
            '    --http-server-address 127.0.0.1:' + str(8000 + portOffset) +
            '    --p2p-listen-endpoint 127.0.0.1:' + str(9000 + portOffset) +
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

def createAccounts(b, e):
    for i in range(b, e):
        a = accounts[i]
        run(cleos + 'create account eosio ' + a['name'] + ' ' + a['pub'] + ' ' + a['pub'])

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

logFile.write('\n\n' + '*' * 80 + '\n\n\n')
run('killall keosd nodeos || true')
time.sleep(1.5)
startWallet()
importKeys()
startProducers(0, 1)
time.sleep(1.5)
createAccounts(1, numSysAccounts)
run(cleos + 'set contract eosio.token ' + contractsDir + 'eosio.token/')
run(cleos + 'set contract eosio.msig ' + contractsDir + 'eosio.msig/')
run(cleos + 'push action eosio.token create \'["eosio", "10000000000.0000 %s"]\' -p eosio.token' % (symbol))
totalAllocation = fillStake(firstRegAccount, len(accounts))
run(cleos + 'push action eosio.token issue \'["eosio", "%s", "memo"]\' -p eosio' % intToCurrency(totalAllocation))
time.sleep(1)
retry(cleos + 'set contract eosio ' + contractsDir + 'eosio.system/')
time.sleep(1)
createStakedAccounts(firstRegAccount, len(accounts))
regProducers(firstProducer, firstProducer + numProducers)
time.sleep(1)
listProducers()
startProducers(firstProducer, firstProducer + numProducers)
time.sleep(2)
vote(firstRegAccount, firstRegAccount + numVoters)
time.sleep(1)
listProducers()
time.sleep(5)
claimRewards()
proxyVotes(firstRegAccount, firstRegAccount + numVoters)
run(cleos + 'push action eosio.token issue \'["eosio", "%d.0000 %s", "memo"]\' -p eosio' % ((len(accounts) - firstRegAccount) * 10, symbol))
sendUnstakedFunds(firstRegAccount, len(accounts))
randomTransfer(firstRegAccount, len(accounts), 8)
run('tail -n 60 ' + nodesDir + 'eosio/stderr')
