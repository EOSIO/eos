#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from TestHelper import TestHelper

import random
import subprocess
import signal
import os
import shutil
import re

Print=Utils.Print
errorExit=Utils.errorExit

stagingDir="staging"
dataDir=stagingDir+"/data"
stderrFile=dataDir + "/stderr.txt"

testNum=0

def cleanDirectories():
    os.path.exists(stagingDir) and shutil.rmtree(stagingDir)

def prepareDirectories():
    # Prepare own directories so we don't depend on others to make sure
    # tests are repeatable
    cleanDirectories()
    os.makedirs(stagingDir)
    os.makedirs(dataDir)

def runNodeos(extraNodeosArgs, myTimeout):
    """Startup nodeos, wait for timeout (before forced shutdown) and collect output."""
    if debug: Print("Launching nodeos process.")
    cmd="programs/nodeos/nodeos -e -p eosio --plugin eosio::chain_api_plugin --plugin eosio::state_history_plugin --disable-replay-opts --data-dir " + dataDir + " "

    cmd=cmd + extraNodeosArgs;
    if debug: Print("cmd: %s" % (cmd))
    with open(stderrFile, 'w') as serr:
        proc=subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=serr)

        try:
            proc.communicate(timeout=myTimeout)
        except (subprocess.TimeoutExpired) as _:
            if debug: Print("Timed out\n")
            proc.send_signal(signal.SIGKILL)

def isMsgInStderrFile(msg):
    msgFound=False
    with open(stderrFile) as errFile:
        for line in errFile:
            if msg in line:
                msgFound=True
                break
    return msgFound

def testCommon(title, extraNodeosArgs, expectedMsgs):
    global testNum
    testNum+=1
    Print("Test %d: %s" % (testNum, title))

    prepareDirectories()

    timeout=120  # Leave sufficient time such nodeos can start up fully in any platforms
    runNodeos(extraNodeosArgs, timeout)

    for msg in expectedMsgs:
        if not isMsgInStderrFile(msg):
            errorExit ("Log should have contained \"%s\"" % (expectedMsgs))

def testAll():
    # State hisory plugin only. nodeos should run normally, producing blocks
    testCommon("state history plugin only", "", ["Produced block"])
    
    # Both state history plugin and history plugin enabled with
    # state-history-allow-legacy-history set. Nodeos would keep running
    testCommon("Both plugins enabled with state-history-allow-legacy-history flag", "--plugin eosio::history_plugin --state-history-allow-legacy-history-plugin", ["Legacy history plugin is running; this will cause random crashes", "With state-history-allow-legacy-history-plugin set, proceed at your own risk", "Produced block"])
    
    # Both state history plugin and history plugin enabled without
    # state-history-allow-legacy-history set. Nodeos aborts.
    testCommon("Both plugins enabled without state-history-allow-legacy-history flag", "--plugin eosio::history_plugin", ["Legacy history plugin is running; this will cause random crashes", "With state-history-allow-legacy-history-plugin not set, shutting down", "nodeos successfully exiting"])

args = TestHelper.parse_args({"--keep-logs","--dump-error-details","-v","--leave-running","--clean-run"})
debug=args.v
pnodes=1
topo="mesh"
delay=1
chainSyncStrategyStr=Utils.SyncResyncTag
total_nodes = pnodes
killCount=1
killSignal=Utils.SigKillTag

killEosInstances= not args.leave_running
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
killAll=args.clean_run

seed=1
Utils.Debug=debug
testSuccessful=False

cluster=Cluster(walletd=True)

try:
    TestHelper.printSystemInfo("BEGIN")

    cluster.setChainStrategy(chainSyncStrategyStr)

    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    if cluster.launch(pnodes=pnodes, totalNodes=total_nodes, topo=topo,delay=delay, dontBootstrap=True) is False:
        errorExit("Failed to stand up eos cluster.")
    cluster.killall(allInstances=killAll)

    testAll()

    testSuccessful=True
finally:
    if debug: Print("Cleanup in finally block.")
    cleanDirectories()
    TestHelper.shutdown(cluster, None, testSuccessful, killEosInstances, False, keepLogs, killAll, dumpErrorDetails)

if debug: Print("Exiting test, exit value 0.")
exit(0)
