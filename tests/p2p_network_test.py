#!/usr/bin/env python3

import testUtils
import p2p_test_peers
import impaired_network
import lossy_network
import p2p_stress

import copy
import decimal
import argparse
import random
import re
import time

Remote = p2p_test_peers.P2PTestPeers

hosts=Remote.hosts
ports=Remote.ports

TEST_OUTPUT_DEFAULT="p2p_network_test_log.txt"
DEFAULT_HOST=hosts[0]
DEFAULT_PORT=ports[0]

parser = argparse.ArgumentParser(add_help=False)
Print=testUtils.Utils.Print
errorExit=testUtils.Utils.errorExit

# Override default help argument so that only --help (and not -h) can call help
parser.add_argument('-?', action='help', default=argparse.SUPPRESS,
                    help=argparse._('show this help message and exit'))
parser.add_argument("-o", "--output", type=str, help="output file", default=TEST_OUTPUT_DEFAULT)
parser.add_argument("--inita_prvt_key", type=str, help="Inita private key.",
                    default=testUtils.Cluster.initaAccount.ownerPrivateKey)
parser.add_argument("--initb_prvt_key", type=str, help="Initb private key.",
                    default=testUtils.Cluster.initbAccount.ownerPrivateKey)
parser.add_argument("--wallet_host", type=str, help="wallet host", default="localhost")
parser.add_argument("--wallet_port", type=int, help="wallet port", default=8899)
parser.add_argument("--impaired_network", help="test impaired network", action='store_true')
parser.add_argument("--lossy_network", help="test lossy network", action='store_true')
parser.add_argument("--stress_network", help="test load/stress network", action='store_true')
parser.add_argument("--not_kill_wallet", help="not killing walletd", action='store_true')

args = parser.parse_args()
testOutputFile=args.output
enableMongo=False
initaPrvtKey=args.inita_prvt_key
initbPrvtKey=args.initb_prvt_key

walletMgr=testUtils.WalletMgr(True, port=args.wallet_port, host=args.wallet_host)

if args.impaired_network:
    module = impaired_network.ImpairedNetwork()
elif args.lossy_network:
    module = lossy_network.LossyNetwork()
elif args.stress_network:
    module = p2p_stress.StressNetwork()
else:
    errorExit("one of impaired_network, lossy_network or stress_network must be set. Please also check peer configs in p2p_test_peers.py.")

cluster=testUtils.Cluster(walletd=True, enableMongo=enableMongo, initaPrvtKey=initaPrvtKey, initbPrvtKey=initbPrvtKey, walletHost=args.wallet_host, walletPort=args.wallet_port)

print("BEGIN")
print("TEST_OUTPUT: %s" % (testOutputFile))

print("number of hosts: %d, list of hosts:" % (len(hosts)))
for i in range(len(hosts)):
    Print("%s:%d" % (hosts[i], ports[i]))

init_str='{"nodes":['
for i in range(len(hosts)):
    if i > 0:
        init_str=init_str+','
    init_str=init_str+'{"host":"' + hosts[i] + '", "port":'+str(ports[i])+'}'
init_str=init_str+']}'

#Print('init nodes with json str %s',(init_str))
cluster.initializeNodesFromJson(init_str);

if args.not_kill_wallet == False:
    print('killing all wallets')
    walletMgr.killall()

walletMgr.cleanup()

print('creating account keys')
accounts=testUtils.Cluster.createAccountKeys(3)
if accounts is None:
    errorExit("FAILURE - create keys")
testeraAccount=accounts[0]
testeraAccount.name="testera"
currencyAccount=accounts[1]
currencyAccount.name="currency"
exchangeAccount=accounts[2]
exchangeAccount.name="exchange"

PRV_KEY1=testeraAccount.ownerPrivateKey
PUB_KEY1=testeraAccount.ownerPublicKey
PRV_KEY2=currencyAccount.ownerPrivateKey
PUB_KEY2=currencyAccount.ownerPublicKey
PRV_KEY3=exchangeAccount.activePrivateKey
PUB_KEY3=exchangeAccount.activePublicKey

testeraAccount.activePrivateKey=currencyAccount.activePrivateKey=PRV_KEY3
testeraAccount.activePublicKey=currencyAccount.activePublicKey=PUB_KEY3

exchangeAccount.ownerPrivateKey=PRV_KEY2
exchangeAccount.ownerPublicKey=PUB_KEY2

print("Stand up walletd")
if walletMgr.launch() is False:
    cmdError("%s" % (WalletdName))
    errorExit("Failed to stand up eos walletd.")

testWalletName="test"
Print("Creating wallet \"%s\"." % (testWalletName))
testWallet=walletMgr.create(testWalletName)
if testWallet is None:
    cmdError("eos wallet create")
    errorExit("Failed to create wallet %s." % (testWalletName))

for account in accounts:
    Print("Importing keys for account %s into wallet %s." % (account.name, testWallet.name))
    if not walletMgr.importKey(account, testWallet):
        cmdError("%s wallet import" % (ClientName))
        errorExit("Failed to import key for account %s" % (account.name))

initaWalletName="inita"
Print("Creating wallet \"%s\"." % (initaWalletName))
initaWallet=walletMgr.create(initaWalletName)
if initaWallet is None:
    cmdError("eos wallet create")
    errorExit("Failed to create wallet %s." % (initaWalletName))

initaAccount=testUtils.Cluster.initaAccount
# initbAccount=testUtils.Cluster.initbAccount

Print("Importing keys for account %s into wallet %s." % (initaAccount.name, initaWallet.name))
if not walletMgr.importKey(initaAccount, initaWallet):
     cmdError("%s wallet import" % (ClientName))
     errorExit("Failed to import key for account %s" % (initaAccount.name))

node0=cluster.getNode(0)
if node0 is None:
    errorExit("cluster in bad state, received None node")

# eosio should have the same key as inita
eosio = copy.copy(initaAccount)
eosio.name = "eosio"

Print("Info of each node:")
for i in range(len(hosts)):
    node = cluster.getNode(0)
    cmd="%s %s get info" % (testUtils.Utils.EosClientPath, node.endpointArgs)
    trans = node.runCmdReturnJson(cmd)
    Print("host %s: %s" % (hosts[i], trans))


wastFile="contracts/eosio.system/eosio.system.wast"
abiFile="contracts/eosio.system/eosio.system.abi"
Print("\nPush system contract %s %s" % (wastFile, abiFile))
trans=node0.publishContract(eosio.name, wastFile, abiFile, waitForTransBlock=True)
if trans is None:
    Utils.errorExit("Failed to publish eosio.system.")
else:
    Print("transaction id %s" % (node0.getTransId(trans)))

try:
    maxIndex = module.maxIndex()
    for cmdInd in range(maxIndex):
        (transIdList, checkacct, expBal, errmsg) = module.execute(cmdInd, node0, testeraAccount, eosio)

        if len(transIdList) == 0 and len(checkacct) == 0:
            errorExit("failed to execute command in host %s:%s" % (hosts[0], errmsg))

        successhosts = []
        attempts = 2
        while attempts > 0 and len(successhosts) < len(hosts):
            attempts = attempts - 1
            for i in range(len(hosts)):
                host = hosts[i]
                if host in successhosts:
                    continue
                if len(checkacct) > 0:
                    actBal = cluster.getNode(i).getAccountBalance(checkacct)
                    if expBal == actBal:
                        Print("acct balance verified in host %s" % (host))
                    else:
                        Print("acct balance check failed in host %s, expect %d actual %d" % (host, expBal, actBal))
                okcount = 0
                failedcount = 0
                for j in range(len(transIdList)):
                    if cluster.getNode(i).getTransaction(transIdList[j]) is None:
                        failedcount = failedcount + 1
                    else:
                        okcount = okcount + 1
                Print("%d transaction(s) verified in host %s, %d transaction(s) failed" % (okcount, host, failedcount))
                if failedcount == 0:
                    successhosts.append(host)
        Print("%d host(s) passed, %d host(s) failed" % (len(successhosts), len(hosts) - len(successhosts)))
finally:
    Print("\nfinally: restore everything")
    module.on_exit()
exit(0)
