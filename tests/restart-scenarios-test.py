#!/usr/bin/env python3

import testUtils

import argparse
import random
import signal

###############################################################
# Test for different test scenarios.
# Nodes can be producing or non-producing.
# -p <producing nodes count>
# -c <chain strategy[replay|resync|none]>
# -s <topology>
# -d <delay between nodes startup>
# -v <verbose logging>
# --kill-sig <kill signal [term|kill]>
# --kill-count <nodeos instances to kill>
# --dont-kill <Leave cluster running after test finishes>
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################


DefaultKillPercent=25
Print=testUtils.Utils.Print

def errorExit(msg="", errorCode=1):
    Print("ERROR:", msg)
    exit(errorCode)

parser = argparse.ArgumentParser()
parser.add_argument("-p", type=int, help="producing nodes count", default=2)
parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
parser.add_argument("-s", type=str, help="topology", default="mesh")
parser.add_argument("-c", type=str, help="chain strategy[%s|%s|%s]" %
                    (testUtils.Utils.SyncResyncTag, testUtils.Utils.SyncReplayTag, testUtils.Utils.SyncNoneTag),
                    default=testUtils.Utils.SyncResyncTag)
parser.add_argument("--kill-sig", type=str, help="kill signal[%s|%s]" %
                    (testUtils.Utils.SigKillTag, testUtils.Utils.SigTermTag), default=testUtils.Utils.SigKillTag)
parser.add_argument("--kill-count", type=int, help="nodeos instances to kill", default=-1)
parser.add_argument("-v", help="verbose logging", action='store_true')
parser.add_argument("--dont-kill", help="Leave cluster running after test finishes", action='store_true')
parser.add_argument("--not-noon", help="This is not the Noon branch.", action='store_true')
parser.add_argument("--dump-error-details",
                    help="Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout",
                    action='store_true')
parser.add_argument("--keep-logs", help="Don't delete var/lib/node_* folders upon test completion",
                    action='store_true')

args = parser.parse_args()
pnodes=args.p
topo=args.s
delay=args.d
chainSyncStrategyStr=args.c
debug=args.v
total_nodes = pnodes
killCount=args.kill_count if args.kill_count > 0 else int(round((DefaultKillPercent/100.0)*total_nodes))
killSignal=args.kill_sig
killEosInstances= not args.dont_kill
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
amINoon=not args.not_noon

testUtils.Utils.Debug=debug

if not amINoon:
    testUtils.Utils.iAmNotNoon()

Print ("producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d, chain sync strategy: %s" % (
    pnodes, topo, delay, chainSyncStrategyStr))

cluster=testUtils.Cluster()
walletMgr=testUtils.WalletMgr(False)
cluster.killall()
cluster.cleanup()
random.seed(1) # Use a fixed seed for repeatability.
testSuccessful=False

try:
    cluster.setChainStrategy(chainSyncStrategyStr)
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

    Print ("Create wallet.")
    if not cluster.populateWallet(accountsCount, wallet):
        errorExit("Wallet initialization failed.")

    Print("Create accounts.")
    #if not cluster.createAccounts(wallet):
    if not cluster.createAccounts(testUtils.Cluster.initaAccount):
        errorExit("Accounts creation failed.")

    Print("Wait on cluster sync.")
    if not cluster.waitOnClusterSync():
        errorExit("Cluster sync wait failed.")

    Print("Spread funds and validate")
    if not cluster.spreadFundsAndValidate(10):
        errorExit("Failed to spread and validate funds.")

    Print("Wait on cluster sync.")
    if not cluster.waitOnClusterSync():
        errorExit("Cluster sync wait failed.")

    Print("Kill %d cluster node instances." % (killCount))
    if cluster.killSomeEosInstances(killCount, killSignal) is False:
        errorExit("Failed to kill Eos instances")
    Print("nodeos instances killed.")

    Print("Spread funds and validate")
    if not cluster.spreadFundsAndValidate(10):
        errorExit("Failed to spread and validate funds.")

    Print("Wait on cluster sync.")
    if not cluster.waitOnClusterSync():
        errorExit("Cluster sync wait failed.")

    Print ("Relaunch dead cluster nodes instances.")
    if cluster.relaunchEosInstances() is False:
        errorExit("Failed to relaunch Eos instances")
    Print("nodeos instances relaunched.")

    Print ("Resyncing cluster nodes.")
    if not cluster.waitOnClusterSync():
        errorExit("Cluster never synchronized")
    Print ("Cluster synched")

    Print("Spread funds and validate")
    if not cluster.spreadFundsAndValidate(10):
        errorExit("Failed to spread and validate funds.")

    Print("Wait on cluster sync.")
    if not cluster.waitOnClusterSync():
        errorExit("Cluster sync wait failed.")

    testSuccessful=True
finally:
    if not testSuccessful and dumpErrorDetails:
        cluster.dumpErrorDetails()
        walletMgr.dumpErrorDetails()
        Print("== Errors see above ==")

    if killEosInstances:
        Print("Shut down the cluster%s" % (" and cleanup." if (testSuccessful and not keepLogs) else "."))
        cluster.killall()
        walletMgr.killall()
        if testSuccessful and not keepLogs:
            Print("Cleanup cluster and wallet data.")
            cluster.cleanup()
            walletMgr.cleanup()
    pass

exit(0)
