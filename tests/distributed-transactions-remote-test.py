#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from TestHelper import TestHelper

import subprocess
import tempfile
import os

###############################################################
# distributed-transactions-remote-test
#  Tests remote capability of the distributed-transactions-test. Test will setup cluster and pass nodes info to distributed-transactions-test. E.g.
#  distributed-transactions-remote-test.py -v --clean-run --dump-error-detail
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit

args = TestHelper.parse_args({"-p","--dump-error-details","-v","--leave-running","--clean-run"})
pnodes=args.p
debug=args.v
dontKill=args.leave_running
dumpErrorDetails=args.dump_error_details
killAll=args.clean_run

Utils.Debug=debug

killEosInstances=not dontKill
topo="mesh"
delay=1
prodCount=1 # producers per producer node
total_nodes=pnodes+3
actualTest="tests/distributed-transactions-test.py"
testSuccessful=False

clusterMapJsonTemplate="""{
    "keys": {
        "defproduceraPrivateKey": "%s",
        "defproducerbPrivateKey": "%s"
    },
    "nodes": [
        {"port": 8888, "host": "localhost"},
        {"port": 8889, "host": "localhost"},
        {"port": 8890, "host": "localhost"}
    ]
}
"""

cluster=Cluster(walletd=True)

(fd, nodesFile) = tempfile.mkstemp()
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    Print ("producing nodes: %s, non-producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d" %
           (pnodes, total_nodes-pnodes, topo, delay))
    Print("Stand up cluster")
    if cluster.launch(pnodes=pnodes, totalNodes=total_nodes, prodCount=prodCount, topo=topo, delay=delay, dontKill=dontKill) is False:
        errorExit("Failed to stand up eos cluster.")

    Print ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not cluster.waitOnClusterBlockNumSync(3):
        errorExit("Cluster never stabilized")

    producerKeys=Cluster.parseClusterKeys(total_nodes)
    defproduceraPrvtKey=producerKeys["defproducera"]["private"]
    defproducerbPrvtKey=producerKeys["defproducerb"]["private"]

    clusterMapJson = clusterMapJsonTemplate % (defproduceraPrvtKey, defproducerbPrvtKey)

    tfile = os.fdopen(fd, "w")
    tfile.write(clusterMapJson)
    tfile.close()

    cmd="%s --nodes-file %s %s %s" % (actualTest, nodesFile, "-v" if debug else "", "--leave-running" if dontKill else "")
    Print("Starting up distributed transactions test: %s" % (actualTest))
    Print("cmd: %s\n" % (cmd))
    if 0 != subprocess.call(cmd, shell=True):
        errorExit("failed to run cmd.")

    testSuccessful=True
    Print("\nEND")
finally:
    os.remove(nodesFile)
    TestHelper.shutdown(cluster, None, testSuccessful, killEosInstances, False, False, killAll, dumpErrorDetails)

exit(0)
