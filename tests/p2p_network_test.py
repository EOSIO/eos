#!/usr/bin/env python3

import testUtils
import p2p_test_peers
import impaird_network
import lossy_network
import p2p_stress

import copy
import decimal
import argparse
import random
import re

Remote = p2p_test_peers.P2PTestPeers

hosts=Remote.hosts
ports=Remote.ports

TEST_OUTPUT_DEFAULT="p2p_network_test_log.txt"
DEFAULT_HOST=hosts[0]
DEFAULT_PORT=ports[0]

parser = argparse.ArgumentParser(add_help=False)
walletMgr=testUtils.WalletMgr(True)
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
parser.add_argument("--impaired_network", help="test impaired network", action='store_true')
parser.add_argument("--lossy_network", help="test lossy network", action='store_true')
parser.add_argument("--stress_network", help="test load/stress network", action='store_true')

args = parser.parse_args()
testOutputFile=args.output
enableMongo=False
initaPrvtKey=args.inita_prvt_key
initbPrvtKey=args.initb_prvt_key

if args.impaired_network is True:
    module = impaird_network.ImpairedNetwork()
elif args.lossy_network is True:
    module = lossy_network.LossyNetwork()
elif args.stress_network is True:
    module = p2p_stress.StressNetwork()
else:
    errorExit("one of impaird_network & lossy_network must be set. Please also check peer configs in p2p_test_peers.py.")

cluster=testUtils.Cluster(walletd=True, enableMongo=enableMongo, initaPrvtKey=initaPrvtKey, initbPrvtKey=initbPrvtKey)

print("BEGIN")
print("TEST_OUTPUT: %s" % (testOutputFile))

print("list of hosts:")
for i in range(len(hosts)):
    Print("%s:%d" % (hosts[i], ports[i]))

init_str='{"nodes":['
for i in range(len(hosts)):
    if i > 0:
        init_str=init_str+','
    init_str=init_str+'{"host":"' + hosts[i] + '", "port":'+str(ports[i])+'}'
init_str=init_str+']}'

Print('init nodes with json str %s',(init_str))
cluster.initializeNodesFromJson(init_str);

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

try:
    maxIndex = module.maxIndex()
    for cmdInd in range(maxIndex):
        transIdList = module.execute(cmdInd, node0, testeraAccount, eosio)
        if len(transIdList) == 0:
            errorExit("no transaction returned")
        Print("got %d transaction(s)" % (len(transIdList)))
        for i in range(len(hosts)):
            for j in range(len(transIdList)):
                trans = cluster.getNode(i).getTransaction(transIdList[j])
            if trans is None:
                errorExit("Failed to retrieve transId %s in host %s" % (transIdList[j], hosts[i]))
        Print("%d transaction(s) verified in %d hosts" % (len(transIdList), len(hosts)))
finally:
    Print("\nfinally: restore everything")
    module.on_exit()
exit(0)
