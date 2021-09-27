#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper

###############################################################
# http_plugin_tests.py
#
# Integration tests for HTTP plugin
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit
cmdError=Utils.cmdError

args = TestHelper.parse_args({"-v","--clean-run", "--dump-error-details","--keep-logs"})
debug=args.v
killAll=args.clean_run
killEosInstances = True
keepLogs = args.keep_logs
dumpErrorDetails = dumpErrorDetails=args.dump_error_details


Utils.Debug=debug
https_port = 5555
cluster=Cluster(walletd=True)

testSuccessful=False

ClientName="cleos"
timeout = .5 * 12 * 2 + 60 # time for finalization with 1 producer + 60 seconds padding
Utils.setIrreversibleTimeout(timeout)

try:
    TestHelper.printSystemInfo("BEGIN")

    Print("Stand up cluster")
    # standup cluster with HTTPS enabled, but not configured
    # HTTP should still work
    extraArgs={ 0 : "--https-server-address 127.0.0.1:5555" }
    # specificExtraNodeosArgs=extraArgs

    if cluster.launch(dontBootstrap=True, loadSystemContract=False) is False:
        cmdError("launcher")
        errorExit("Failed to stand up eos cluster.")

    Print("Getting cluster info")
    cluster.getInfos()
    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, None, testSuccessful, killEosInstances, True, keepLogs, killAll, dumpErrorDetails)
        