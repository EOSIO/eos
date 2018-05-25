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
enucli = 'enucli --wallet-url http://localhost:6666 --url http://localhost:8000 '
enunode = 'enunode'
fastUnstakeSystem = './fast.refund/enumivo.system/enumivo.system.wasm'
logFile = open('test.log', 'a')

symbol = 'SYS'
maxUserKeys = 10            # Maximum user keys to import into wallet
minProducerStake = 20.0000  # Minimum producer CPU and BW stake
extraIssue = 10.0000        # Extra amount to issue to cover buying ram
limitUsers = 0              # Limit number of users if >0
limitProducers = 0          # Limit number of producers if >0
numVoters = 10              # Number of users which cast votes
numProducersToVoteFor = 20  # Number of producers each user votes for
numSenders = 10             # Number of users to transfer funds randomly
producerSyncDelay = 80      # Time (s) to sleep to allow producers to sync

enumivoPub = 'ENU8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr'
enumivoPvt = '5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p'
systemAccounts = [
    'enumivo.bpay',
    'enumivo.msig',
    'enumivo.name',
    'enumivo.ram',
    'enumivo.rfee',
    'enumivo.save',
    'enumivo.stk',
    'enumivo.coin',
    'enumivo.vpay',
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

def getOutput(args):
    print('test.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return proc.communicate()[0].decode('utf-8')

def getJsonOutput(args):
    print('test.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return json.loads(proc.communicate()[0])

def sleep(t):
    print('sleep', t, '...')
    time.sleep(t)
    print('resume')

def startWallet():
    run('rm -rf ' + walletDir)
    run('mkdir -p ' + walletDir)
    background('enuwallet --unlock-timeout %d --http-server-address 127.0.0.1:6666 --wallet-dir %s' % (unlockTimeout, walletDir))
    sleep(.4)
    run(enucli + 'wallet create')

def importKeys():
    run(enucli + 'wallet import ' + enumivoPvt)
    keys = {}
    for a in accounts:
        key = a['pvt']
        if not key in keys:
            if len(keys) >= maxUserKeys:
                break
            keys[key] = True
            run(enucli + 'wallet import ' + key)
    for i in range(firstProducer, firstProducer + numProducers):
        a = accounts[i]
        key = a['pvt']
        if not key in keys:
            keys[key] = True
            run(enucli + 'wallet import ' + key)

def startNode(nodeIndex, account):
    dir = nodesDir + ('%02d-' % nodeIndex) + account['name'] + '/'
    run('rm -rf ' + dir)
    run('mkdir -p ' + dir)
    otherOpts = ''.join(list(map(lambda i: '    --p2p-peer-address localhost:' + str(9000 + i), range(nodeIndex))))
    if not nodeIndex: otherOpts += (
        '    --plugin enumivo::history_plugin'
        '    --plugin enumivo::history_api_plugin'
    )
    cmd = (
        enunode +
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
        '    --plugin enumivo::http_plugin'
        '    --plugin enumivo::chain_api_plugin'
        '    --plugin enumivo::producer_plugin' +
        otherOpts)
    with open(dir + 'stderr', mode='w') as f:
        f.write(cmd + '\n\n')
    background(cmd + '    2>>' + dir + 'stderr')

def startProducers(b, e):
    for i in range(b, e):
        startNode(i - b + 1, accounts[i])

def createSystemAccounts():
    for a in systemAccounts:
        run(enucli + 'create account enumivo ' + a + ' ' + enumivoPub)

def intToCurrency(i):
    return '%d.%04d %s' % (i // 10000, i % 10000, symbol)

def fillStake(b, e):
    dist = numpy.random.pareto(1.161, e - b).tolist() # 1.161 = 80/20 rule
    dist.sort()
    dist.reverse()
    factor = 1_000_000_000 / sum(dist)
    total = 0
    for i in range(b, e):
        stake = round(factor * dist[i - b] * 10000 / 2)
        if i >= firstProducer and i < firstProducer + numProducers:
            stake = max(stake, round(minProducerStake * 10000))
        total += stake * 2
        accounts[i]['stake'] = stake
    return total + round(extraIssue * 10000)

def createStakedAccounts(b, e):
    for i in range(b, e):
        a = accounts[i]
        stake = intToCurrency(a['stake'])
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

def claimRewards():
    table = getJsonOutput(enucli + 'get table enumivo enumivo producers -l 100')
    for row in table['rows']:
        if row['unpaid_blocks'] and not row['last_claim_time']:
            run(enucli + 'system claimrewards ' + row['owner'])

def vote(b, e):
    for i in range(b, e):
        voter = accounts[i]['name']
        prods = random.sample(range(firstProducer, firstProducer + numProducers), numProducersToVoteFor)
        prods = ' '.join(map(lambda x: accounts[x]['name'], prods))
        retry(enucli + 'system voteproducer prods ' + voter + ' ' + prods)

def proxyVotes(b, e):
    vote(firstProducer, firstProducer + 1)
    proxy = accounts[firstProducer]['name']
    retry(enucli + 'system regproxy ' + proxy)
    sleep(1.0)
    for i in range(b, e):
        voter = accounts[i]['name']
        retry(enucli + 'system voteproducer proxy ' + voter + ' ' + proxy)

def updateAuth(account, permission, parent, controller):
    run(enucli + 'push action enumivo updateauth' + jsonArg({
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
    run(enucli + 'get account ' + account)

def sendUnstakedFunds(b, e):
    for i in range(b, e):
        a = accounts[i]
        run(enucli + 'transfer enumivo ' + a['name'] + ' "10.0000 ' + symbol + '"')

def randomTransfer(b, e, n):
    for i in range(n):
        for j in range(20):
            src = accounts[random.randint(b, e - 1)]['name']
            dest = src
            while dest == src:
                dest = accounts[random.randint(b, e - 1)]['name']
            run(enucli + 'transfer -f ' + src + ' ' + dest + ' "0.0001 ' + symbol + '"' + ' || true')
        sleep(.25)

def msigProposeReplaceSystem(proposer, proposalName):
    requestedPermissions = []
    for i in range(firstProducer, firstProducer + numProducers):
        requestedPermissions.append({'actor': accounts[i]['name'], 'permission': 'active'})
    trxPermissions = [{'actor': 'enumivo', 'permission': 'active'}]
    with open(fastUnstakeSystem, mode='rb') as f:
        setcode = {'account': 'enumivo', 'vmtype': 0, 'vmversion': 0, 'code': f.read().hex()}
    run(enucli + 'multisig propose ' + proposalName + jsonArg(requestedPermissions) + 
        jsonArg(trxPermissions) + 'enumivo setcode' + jsonArg(setcode) + ' -p ' + proposer)

def msigApproveReplaceSystem(proposer, proposalName):
    for i in range(firstProducer, firstProducer + numProducers):
        run(enucli + 'multisig approve ' + proposer + ' ' + proposalName +
            jsonArg({'actor': accounts[i]['name'], 'permission': 'active'}) +
            '-p ' + accounts[i]['name'])

def msigExecReplaceSystem(proposer, proposalName):
    retry(enucli + 'multisig exec ' + proposer + ' ' + proposalName + ' -p ' + proposer)

def msigReplaceSystem():
    run(enucli + 'push action enumivo buyrambytes' + jsonArg(['enumivo', accounts[0]['name'], 200000]) + '-p enumivo')
    sleep(1)
    msigProposeReplaceSystem(accounts[0]['name'], 'fast.unstake')
    sleep(1)
    msigApproveReplaceSystem(accounts[0]['name'], 'fast.unstake')
    msigExecReplaceSystem(accounts[0]['name'], 'fast.unstake')

def produceNewAccounts():
    with open('newusers', 'w') as f:
        for i in range(0, 3000):
            x = getOutput(enucli + 'create key')
            r = re.match('Private key: *([^ \n]*)\nPublic key: *([^ \n]*)', x, re.DOTALL | re.MULTILINE)
            name = 'user'
            for j in range(7, -1, -1):
                name += chr(ord('a') + ((i >> (j * 4)) & 15))
            print(i, name)
            f.write('        {"name":"%s", "pvt":"%s", "pub":"%s"},\n' % (name, r[1], r[2]))

logFile.write('\n\n' + '*' * 80 + '\n\n\n')
run('killall enuwallet enunode || true')
sleep(1.5)
startWallet()
importKeys()
startNode(0, {'name': 'enumivo', 'pvt': enumivoPvt, 'pub': enumivoPub})
sleep(1.5)
createSystemAccounts()
run(enucli + 'set contract enumivo.coin ' + contractsDir + 'enumivo.coin/')
run(enucli + 'set contract enumivo.msig ' + contractsDir + 'enumivo.msig/')
run(enucli + 'push action enumivo.coin create \'["enumivo", "10000000000.0000 %s"]\' -p enumivo.coin' % (symbol))
totalAllocation = fillStake(0, len(accounts))
run(enucli + 'push action enumivo.coin issue \'["enumivo", "%s", "memo"]\' -p enumivo' % intToCurrency(totalAllocation))
sleep(1)
retry(enucli + 'set contract enumivo ' + contractsDir + 'enumivo.system/')
sleep(1)
run(enucli + 'push action enumivo setpriv' + jsonArg(['enumivo.msig', 1]) + '-p enumivo@active')
createStakedAccounts(0, len(accounts))
regProducers(firstProducer, firstProducer + numProducers)
sleep(1)
listProducers()
startProducers(firstProducer, firstProducer + numProducers)
sleep(producerSyncDelay)
vote(0, 0 + numVoters)
sleep(1)
listProducers()
sleep(5)
claimRewards()
proxyVotes(0, 0 + numVoters)
resign('enumivo', 'enumivo.prods')
for a in systemAccounts:
    resign(a, 'enumivo')
# msigReplaceSystem()
run(enucli + 'push action enumivo.coin issue \'["enumivo", "%d.0000 %s", "memo"]\' -p enumivo' % ((len(accounts)) * 10, symbol))
sendUnstakedFunds(0, numSenders)
randomTransfer(0, numSenders, 8)
run('tail -n 60 ' + nodesDir + '00-enumivo/stderr')
