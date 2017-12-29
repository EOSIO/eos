#!/usr/bin/python3

import testUtils

import argparse
import random
import signal

KILL_PERCENT=50
Print=testUtils.Utils.Print

def errorExit(msg="", errorCode=1):
    Print("ERROR:", msg)
    exit(errorCode)

parser = argparse.ArgumentParser()
parser.add_argument("-p", type=int, help="producing nodes count", default=2)
parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
parser.add_argument("-s", type=str, help="topology", default="mesh")
parser.add_argument("-c", type=str, help="chain strategy[%s|%s|%s]" % (
    testUtils.Utils.SyncResyncTag, testUtils.Utils.SyncReplayTag, testUtils.Utils.SyncNoneTag),
                    default=testUtils.Utils.SyncResyncTag)
parser.add_argument("-v", help="verbose", action='store_true')

args = parser.parse_args()
pnodes=args.p
topo=args.s
delay=args.d
chainSyncStrategyStr=args.c
debug=args.v
total_nodes = pnodes

testUtils.Utils.Debug=debug

Print ("producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d, chain sync strategy: %s" % (pnodes, topo, delay, chainSyncStrategyStr))

cluster=testUtils.Cluster()
cluster.killall()
cluster.cleanup()
random.seed(1) # Use a fixed seed for repeatability.

try:
    print("Stand up cluster")
    if cluster.launch(pnodes, total_nodes, topo, delay) is False:
        errorExit("Failed to stand up eos cluster.")
    
    Print ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not cluster.waitOnClusterBlockNumSync(3):
        errorExit("Cluster never stabilized")

    accountsCount=total_nodes
    Print ("Create wallet.")
    if not cluster.populateWallet(accountsCount):
        errorExit("Wallet initialization failed.")

    Print("Create accounts.")
    if not cluster.createAccounts():
        errorExit("Accounts creation failed.")

    Print("Spread funds and validate")
    if not cluster.spreadFundsAndValidate(10):
        errorExit("Failed to spread and validate funds.")

    killSignal=signal.SIGTERM
    if chainSyncStrategyStr == testUtils.Utils.SyncNoneTag:
        killSignal=signal.SIGKILL
    
    Print("Kill %d%% cluster node instances." % (KILL_PERCENT))
    if cluster.killSomeEosInstances(KILL_PERCENT, killSignal) is False:
        errorExit("Failed to kill Eos instances")
    Print("Eosd instances killed.")

    Print("Spread funds and validate")
    if not cluster.spreadFundsAndValidate(10):
        errorExit("Failed to spread and validate funds.")

    Print ("Relaunch dead cluster nodes instances.")
    if cluster.relaunchEosInstances() is False:
        errorExit("Failed to relaunch Eos instances")
    Print("Eosd instances relaunched.")

    Print ("Resyncing cluster nodes.")
    if not cluster.waitOnClusterSync():
        errorExit("Cluster never synchronized")
    Print ("Cluster synched")

    Print("Spread funds and validate")
    if not cluster.spreadFundsAndValidate(10):
        errorExit("Failed to spread and validate funds.")
    
finally:
    Print("Shut down the cluster and cleanup.")
    cluster.killall()
    cluster.cleanup()
    pass
    
exit(0)
