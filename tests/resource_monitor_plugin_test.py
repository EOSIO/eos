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

stagingDir="rsmStaging"
dataDir=stagingDir+"/data"
configDir=stagingDir+"/etc"
traceDir=dataDir+"/traceDir"

loggingFile=configDir+"/logging.json"
stderrFile=dataDir + "/stderr.txt"

testNum=0

# We need debug level to get more information about nodeos process
logging="""{
  "includes": [],
  "appenders": [{
      "name": "stderr",
      "type": "console",
      "args": {
        "stream": "std_error",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    }
  ],
  "loggers": [{
      "name": "default",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    }
  ]
}"""

def cleanDirectories():
    os.path.exists(stagingDir) and shutil.rmtree(stagingDir)

def prepareDirectories():
    # Prepare own directories so we don't depend on others to make sure
    # tests are repeatable
    cleanDirectories()
    os.makedirs(stagingDir)
    os.makedirs(dataDir)
    os.makedirs(configDir)

    with open(loggingFile, "w") as textFile:
        print(logging,file=textFile)

def runNodeos(extraNodeosArgs, myTimeout):
    """Startup nodeos, wait for timeout (before forced shutdown) and collect output."""
    if debug: Print("Launching nodeos process.")
    cmd="programs/nodeos/nodeos --config-dir rsmStaging/etc -e -p eosio --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --plugin eosio::resource_monitor_plugin --data-dir " + dataDir + " "

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

    timeout=90  # Leave sufficient time such nodeos can start up fully in any platforms
    runNodeos(extraNodeosArgs, timeout)

    for msg in expectedMsgs:
        if not isMsgInStderrFile(msg):
            errorExit ("Log should have contained \"%s\"" % (expectedMsgs))

def testAll():
    testCommon("All arguments", "--resource-monitor-space-threshold=85 --resource-monitor-interval-seconds=5 --resource-monitor-not-shutdown-on-threshold-exceeded", ["threshold set to 85", "interval set to 5", "Shutdown flag when threshold exceeded set to false"])

    testCommon("No arguments supplied", "", ["interval set to 2", "threshold set to 90", "Shutdown flag when threshold exceeded set to true"])
    
    testCommon("Producer, Chain, State History and Trace Api with Resource Monitor", "--plugin eosio::state_history_plugin --state-history-dir=/tmp/state-history --disable-replay-opts --plugin eosio::trace_api_plugin --trace-dir=/tmp/trace --trace-no-abis", ["snapshots's file system to be monitored", "blocks's file system to be monitored", "state's file system to be monitored", "state-history's file system to be monitored", "trace's file system to be monitored"])

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
