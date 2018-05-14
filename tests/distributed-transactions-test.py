#!/usr/bin/env python3

import testUtils

import argparse
import random

Print=testUtils.Utils.Print

def errorExit(msg="", errorCode=1):
    Print("ERROR:", msg)
    exit(errorCode)

seed=1

parser = argparse.ArgumentParser()
parser.add_argument("-p", type=int, help="producing nodes count", default=1)
parser.add_argument("-n", type=int, help="total nodes", default=0)
parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
parser.add_argument("-s", type=str, help="topology", default="mesh")
parser.add_argument("-v", help="verbose", action='store_true')
parser.add_argument("--nodes-file", type=str, help="File containing nodes info in JSON format.")
parser.add_argument("--seed", type=int, help="random seed", default=seed)
parser.add_argument("--dont-kill", help="Leave cluster running after test finishes", action='store_true')
parser.add_argument("--dump-error-details",
                    help="Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout",
                    action='store_true')

args = parser.parse_args()
pnodes=args.p
topo=args.s
delay=args.d
total_nodes = pnodes if args.n == 0 else args.n
debug=args.v
nodesFile=args.nodes_file
seed=args.seed
dontKill=args.dont_kill
dumpErrorDetails=args.dump_error_details

killWallet=not dontKill
killEosInstances=not dontKill
if nodesFile is not None:
    killEosInstances=False

testUtils.Utils.Debug=debug
testSuccessful=False

random.seed(seed) # Use a fixed seed for repeatability.
cluster=testUtils.Cluster(walletd=True)
walletMgr=testUtils.WalletMgr(True)

try:
    cluster.setWalletMgr(walletMgr)

    if nodesFile is not None:
        jsonStr=None
        with open(nodesFile, "r") as f:
            jsonStr=f.read()
        if not cluster.initializeNodesFromJson(jsonStr):
            errorExit("Failed to initilize nodes from Json string.")
        total_nodes=len(cluster.getNodes())
    else:
        cluster.killall()
        cluster.cleanup()
        walletMgr.killall()
        walletMgr.cleanup()

        Print ("producing nodes: %s, non-producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d" %
               (pnodes, total_nodes-pnodes, topo, delay))

        Print("Stand up cluster")
        if cluster.launch(pnodes, total_nodes, topo=topo, delay=delay) is False:
            errorExit("Failed to stand up eos cluster.")

        #exit(0)
        Print ("Wait for Cluster stabilization")
        # wait for cluster to start producing blocks
        if not cluster.waitOnClusterBlockNumSync(3):
            errorExit("Cluster never stabilized")

    #exit(0)

    Print("Stand up EOS wallet keosd")
    if walletMgr.launch() is False:
        errorExit("Failed to stand up keosd.")

    accountsCount=total_nodes
    walletName="MyWallet-%d" % (random.randrange(10000))
    Print("Creating wallet %s if one doesn't already exist." % walletName)
    wallet=walletMgr.create(walletName)
    if wallet is None:
        errorExit("Failed to create wallet %s" % (walletName))

    Print ("Populate wallet with %d accounts." % (accountsCount))
    if not cluster.populateWallet(accountsCount, wallet):
        errorExit("Wallet initialization failed.")

    initaAccount=cluster.initaAccount
    initbAccount=cluster.initbAccount
    eosioAccount=cluster.eosioAccount

    # TBD: get account is currently failing. Enable when ready
    # Print("Create accounts.")
    # if not cluster.createAccounts(eosioAccount):
    #     errorExit("Accounts creation failed.")

    # TBD: Known issue (Issue 2043) that 'get currency balance' doesn't return balance.
    #  Uncomment when functional
    # Print("Spread funds and validate")
    # if not cluster.spreadFundsAndValidate(10):
    #     errorExit("Failed to spread and validate funds.")

    # print("Funds spread validated")
    
    testSuccessful=True
finally:
    if not testSuccessful and dumpErrorDetails:
        cluster.dumpErrorDetails()
        Print("== Errors see above ==")

    if killEosInstances:
        Print("Shut down the cluster and cleanup.")
        cluster.killall()
        cluster.cleanup()
    if killWallet:
        Print("Shut down the wallet and cleanup.")
        walletMgr.killall()
        walletMgr.cleanup()
    pass

exit(0)
