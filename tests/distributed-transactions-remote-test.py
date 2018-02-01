#!/usr/bin/python3

import testUtils

import argparse
import subprocess

Print=testUtils.Utils.Print

def errorExit(msg="", errorCode=1):
    Print("ERROR:", msg)
    exit(errorCode)

pnodes=3
nodesFile="tests/sample-cluster-map.json"
parser = argparse.ArgumentParser()
parser.add_argument("-p", type=int, help="producing nodes count", default=pnodes)
parser.add_argument("-v", help="verbose", action='store_true')
parser.add_argument("--nodes-file", type=str, help="File containing nodes info in JSON format.", default=nodesFile)
parser.add_argument("--dump-error-details",
                    help="Upon error print tn_data_*/config.ini and tn_data_*/stderr.log to stdout",
                    action='store_true')

args = parser.parse_args()
pnodes=args.p
nodesFile=args.nodes_file
debug=args.v
dumpErrorDetails=args.dump_error_details

testUtils.Utils.Debug=debug

killEosInstances=True
topo="mesh"
delay=1
total_nodes=pnodes
actualTest="tests/distributed-transactions-test.py"
testSuccessful=False

cluster=testUtils.Cluster()

try:
    Print("BEGIN")
    cluster.killall()
    cluster.cleanup()

    Print ("producing nodes: %s, non-producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d" %
           (pnodes, total_nodes-pnodes, topo, delay))
    Print("Stand up cluster")
    if cluster.launch(pnodes, total_nodes, topo, delay) is False:
        errorExit("Failed to stand up eos cluster.")
    
    Print ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not cluster.waitOnClusterBlockNumSync(3):
        errorExit("Cluster never stabilized")

    cmd="%s --nodes-file %s %s" % (actualTest, nodesFile, "-v" if debug else "")
    Print("Starting up distributed transactions test: %s" % (actualTest))
    Print("cmd: %s\n" % (cmd))
    if 0 != subprocess.call(cmd, shell=True):
        errorExit("failed to run cmd.")

    testSuccessful=True
    Print("\nEND")
finally:
    if not testSuccessful and dumpErrorDetails:
        cluster.dumpErrorDetails()
        Print("== Errors see above ==")

    if killEosInstances:
        Print("Shut down the cluster and cleanup.")
        cluster.killall()
        cluster.cleanup()
    
exit(0)
