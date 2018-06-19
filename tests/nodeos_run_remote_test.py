#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from TestHelper import TestHelper

import subprocess

###############################################################
# nodeos_run_remote_test
#  Tests remote capability of the nodeos_run_test. Test will setup cluster and pass nodes info to nodeos_run_test. E.g.
#  nodeos_run_remote_test.py -v --clean-run --dump-error-detail
###############################################################

Print=Utils.Print

def errorExit(msg="", errorCode=1):
    Print("ERROR:", msg)
    exit(errorCode)

args = TestHelper.parse_args({"--dump-error-details","-v","--leave-running","--only-bios","--clean-run"})
debug=args.v
dontKill=args.leave_running
dumpErrorDetails=args.dump_error_details
onlyBios=args.only_bios
killAll=args.clean_run

Utils.Debug=debug

killEosInstances=not dontKill
topo="mesh"
delay=1
prodCount=1 # producers per producer node
pnodes=1
total_nodes=pnodes
actualTest="tests/nodeos_run_test.py"
testSuccessful=False

cluster=Cluster()
try:
    Print("BEGIN")
    cluster.killall(allInstances=killAll)
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

    producerKeys=Cluster.parseClusterKeys(1)
    defproduceraPrvtKey=producerKeys["defproducera"]["private"]
    defproducerbPrvtKey=producerKeys["defproducerb"]["private"]

    cmd="%s --dont-launch --defproducera_prvt_key %s --defproducerb_prvt_key %s %s %s %s" % (actualTest, defproduceraPrvtKey, defproducerbPrvtKey, "-v" if debug else "", "--dont-kill" if dontKill else "", "--only-bios" if onlyBios else "")
    Print("Starting up %s test: %s" % ("nodeos", actualTest))
    Print("cmd: %s\n" % (cmd))
    if 0 != subprocess.call(cmd, shell=True):
        errorExit("failed to run cmd.")

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, None, testSuccessful, killEosInstances, False, False, killAll, dumpErrorDetails)

exit(0)
