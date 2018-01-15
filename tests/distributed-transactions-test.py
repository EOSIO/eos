#!/usr/bin/python3

import testUtils

import argparse
import random

Print=testUtils.Utils.Print

def errorExit(msg="", errorCode=1):
    Print("ERROR:", msg)
    exit(errorCode)

parser = argparse.ArgumentParser()
parser.add_argument("-p", type=int, help="producing nodes count", default=1)
parser.add_argument("-n", type=int, help="total nodes", default=0)
parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
parser.add_argument("-s", type=str, help="topology", default="star")
parser.add_argument("-v", help="verbose", action='store_true')

args = parser.parse_args()
pnodes=args.p
topo=args.s
delay=args.d
total_nodes = pnodes if args.n == 0 else args.n
debug=args.v

testUtils.Utils.Debug=debug

Print ("producing nodes:", pnodes, ", non-producing nodes: ", total_nodes-pnodes,
       ", topology:", topo, ", delay between nodes launch(seconds):", delay)

cluster=testUtils.Cluster()
walletMgr=testUtils.WalletMgr(False)
cluster.killall()
cluster.cleanup()
random.seed(1) # Use a fixed seed for repeatability.

try:
    cluster.setWalletMgr(walletMgr)
    Print("Stand up cluster")
    if cluster.launch(pnodes, total_nodes, topo, delay) is False:
        errorExit("Failed to stand up eos cluster.")
    
    Print ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not cluster.waitOnClusterBlockNumSync(3):
        errorExit("Cluster never stabilized")

    accountsCount=total_nodes
    walletName="MyWallet"
    Print("Creating wallet %s if one doesn't already exist." % walletName)
    wallet=walletMgr.create(walletName)
    if wallet is None:
        errorExit("Failed to create wallet %s" % (walletName))

    Print ("Populate wallet.")
    if not cluster.populateWallet(accountsCount, wallet):
        errorExit("Wallet initialization failed.")

    Print("Create accounts.")
    if not cluster.createAccounts(testUtils.Cluster.initaAccount):
        errorExit("Accounts creation failed.")

    Print("Spread funds and validate")
    if not cluster.spreadFundsAndValidate(10):
        errorExit("Failed to spread and validate funds.")

    print("Funds spread validated")
finally:
    Print("Shut down the cluster, wallet and cleanup.")
    cluster.killall()
    walletMgr.killall()
    cluster.cleanup()
    cluster.cleanup()
    pass
    
exit(0)
