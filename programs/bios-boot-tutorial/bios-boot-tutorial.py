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
nodesDir = './nodes/'
contractsDir = '../../build/contracts/'
enucli = '../../build/programs/enucli/enucli --wallet-url http://localhost:6666 --url http://localhost:8000 '
enunode = '../../build/programs/enunode/enunode'
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

def startWallet():
    run('rm -rf ' + walletDir)
    run('mkdir -p ' + walletDir)
    background('enuwallet --http-server-address 127.0.0.1:6666 --wallet-dir ' + walletDir)
    time.sleep(.4)
    run(enucli + 'wallet create')
    keys = {}
    for i in range(2, len(accounts)):
        key = accounts[i]['pvt']
        if not key in keys:
            keys[key] = True
            run(enucli + 'wallet import ' + key)

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
            '    --plugin enumivo::history_plugin'
            '    --plugin enumivo::history_api_plugin'
        )
        portOffset = 0 if not i else i - firstProducer + 1
        cmd = (
            enunode +
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
            '    --plugin enumivo::http_plugin'
            '    --plugin enumivo::chain_api_plugin'
            '    --plugin enumivo::producer_plugin' +
            otherOpts)
        with open(dir + 'stderr', mode='w') as f:
            f.write(cmd + '\n\n')
        background(cmd + '    2>>' + dir + 'stderr')

def createAccounts(b, e):
    for i in range(b, e):
        a = accounts[i]
        run(enucli + 'create account enumivo ' + a['name'] + ' ' + a['pub'] + ' ' + a['pub'])

def createStakedAccounts(b, e):
    dist = numpy.random.pareto(1.161, e - b).tolist() # 1.161 = 80/20 rule
    dist.sort()
    dist.reverse()
    factor = 1_000_000_000 / sum(dist)
    for i in range(b, e):
        a = accounts[i]
        stake = '%.4f %s' % (factor * dist[i - b] / 2, symbol)
        run(enucli + 'system newaccount enumivo --transfer ' + a['name'] + ' ' + a['pub'] + ' --stake-net "' + stake + '" --stake-cpu "' + stake + '"')

def regProducers(b, e):
    for i in range(b, e):
        a = accounts[i]
        retry(enucli + 'system regproducer ' + a['name'] + ' ' + a['pub'] + ' https://' + a['name'] + '.com' + '/' + a['pub'])

def listProducers():
    run(enucli + 'system listproducers')

def vote(b, e):
    for i in range(b, e):
        voter = accounts[i]['name']
        prods = random.sample(range(firstProducer, firstProducer + numProducers), numProducersToVoteFor)
        prods = ' '.join(map(lambda x: accounts[x]['name'], prods))
        retry(enucli + 'system voteproducer prods ' + voter + ' ' + prods)

logFile.write('\n\n' + '*' * 80 + '\n\n\n')
run('killall enuwallet enunode || true')
time.sleep(1.5)
startWallet()
startProducers(0, 1)
time.sleep(1.5)
createAccounts(1, numSysAccounts)
run(enucli + 'set contract enumivo.coin ' + contractsDir + 'enumivo.coin/')
run(enucli + 'set contract enumivo.msig ' + contractsDir + 'enumivo.msig/')
run(enucli + 'push action enumivo.coin create \'["enumivo", "10000000000.0000 %s", 0, 0, 0]\' -p enumivo.coin' % (symbol))
run(enucli + 'push action enumivo.coin issue \'["enumivo", "1000000000.0000 %s", "memo"]\' -p enumivo' % (symbol))
time.sleep(1)
retry(enucli + 'set contract enumivo ' + contractsDir + 'enumivo.system/')
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
run('tail -n 60 ' + nodesDir + 'enumivo/stderr')
