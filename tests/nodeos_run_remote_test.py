#!/usr/bin/env python3

import testUtils

import argparse
import subprocess

Print=testUtils.Utils.Print

def errorExit(msg="", errorCode=1):
    Print("ERROR:", msg)
    exit(errorCode)

parser = argparse.ArgumentParser()
parser.add_argument("-v", help="verbose", action='store_true')
parser.add_argument("--dont-kill", help="Leave cluster running after test finishes", action='store_true')
parser.add_argument("--only-bios", help="Limit testing to bios node.", action='store_true')
parser.add_argument("--dump-error-details",
                    help="Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout",
                    action='store_true')

args = parser.parse_args()
debug=args.v
dontKill=args.dont_kill
dumpErrorDetails=args.dump_error_details
onlyBios=args.only_bios

testUtils.Utils.Debug=debug

killEosInstances=not dontKill
topo="mesh"
delay=1
prodCount=1 # producers per producer node
pnodes=1
total_nodes=pnodes
actualTest="tests/nodeos_run_test.py"
testSuccessful=False

cluster=testUtils.Cluster()
try:
    Print("BEGIN")
    cluster.killall()
    cluster.cleanup()

    Print ("producing nodes: %s, non-producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d" %
           (pnodes, total_nodes-pnodes, topo, delay))
    Print("Stand up cluster")
    if cluster.launch(pnodes, total_nodes, prodCount, topo, delay, onlyBios=onlyBios, dontKill=dontKill) is False:
        errorExit("Failed to stand up eos cluster.")

    Print ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not cluster.waitOnClusterBlockNumSync(3):
        errorExit("Cluster never stabilized")

    producerKeys=testUtils.Cluster.parseClusterKeys(1)
    initaPrvtKey=producerKeys["inita"]["private"]
    initbPrvtKey=producerKeys["initb"]["private"]

    cmd="%s --dont-launch --inita_prvt_key %s --initb_prvt_key %s %s %s %s" % (actualTest, initaPrvtKey, initbPrvtKey, "-v" if debug else "", "--dont-kill" if dontKill else "", "--only-bios" if onlyBios else "")
    Print("Starting up %s test: %s" % ("nodeos", actualTest))
    Print("cmd: %s\n" % (cmd))
    if 0 != subprocess.call(cmd, shell=True):
        errorExit("failed to run cmd.")

    testSuccessful=True
finally:
    if testSuccessful:
        Print("Test succeeded.")
    else:
        Print("Test failed.")

    if not testSuccessful and dumpErrorDetails:
        cluster.dumpErrorDetails()
        Print("== Errors see above ==")

    if killEosInstances:
        Print("Shut down the cluster and cleanup.")
        cluster.killall()
        cluster.cleanup()

exit(0)
