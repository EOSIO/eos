#!/usr/bin/env python3

from testUtils import Utils, Account
from Cluster import Cluster
from TestHelper import TestHelper
from WalletMgr import WalletMgr
from Node import Node

import signal
import json
import time
import os
import filecmp

###############################################################
# nodeos_chainbase_allocation_test
#
# Test snapshot creation and restarting from snapshot
#
###############################################################

# Parse command line arguments
args = TestHelper.parse_args({"-v","--clean-run","--dump-error-details","--leave-running","--keep-logs"})
Utils.Debug = args.v
killAll=args.clean_run
dumpErrorDetails=args.dump_error_details
dontKill=args.leave_running
killEosInstances=not dontKill
killWallet=not dontKill
keepLogs=args.keep_logs

walletMgr=WalletMgr(True)
cluster=Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

testSuccessful = False
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=killAll)
    cluster.cleanup()

    # The following is the list of chainbase objects that need to be verified:
    # - account_object (bootstrap)
    # - code_object (bootstrap)
    # - generated_transaction_object
    # - global_property_object
    # - key_value_object (bootstrap)
    # - protocol_state_object (bootstrap)
    # - permission_object (bootstrap)
    # The bootstrap process has created account_object and code_object (by uploading the bios contract),
    # key_value_object (token creation), protocol_state_object (preactivation feature), and permission_object
    # (automatically taken care by the automatically generated eosio account)
    assert cluster.launch(
        pnodes=1,
        prodCount=1,
        totalProducers=1,
        totalNodes=2,
        useBiosBootFile=False,
        loadSystemContract=False,
        specificExtraNodeosArgs={
            1:"--read-mode irreversible --plugin eosio::producer_api_plugin"})

    producerNodeId = 0
    irrNodeId = 1
    producerNode = cluster.getNode(producerNodeId)
    irrNode = cluster.getNode(irrNodeId)

    # Create delayed transaction to create "generated_transaction_object"
    cmd = "create account -j eosio sample EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\
         EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV --delay-sec 600 -p eosio"
    trans = producerNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans

    # Schedule a new producer to trigger new producer schedule for "global_property_object"
    newProducerAcc = Account("newprod")
    newProducerAcc.ownerPublicKey = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
    newProducerAcc.activePublicKey = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
    producerNode.createAccount(newProducerAcc, cluster.eosioAccount)

    setProdsStr = '{"schedule": ['
    setProdsStr += '{"producer_name":' + newProducerAcc.name + ',"block_signing_key":' + newProducerAcc.activePublicKey + '}'
    setProdsStr += ']}'
    cmd="push action -j eosio setprods '{}' -p eosio".format(setProdsStr)
    trans = producerNode.processCleosCmd(cmd, cmd, silentErrors=False)
    assert trans
    setProdsBlockNum = int(trans["processed"]["block_num"])

    # Wait until the block where set prods is executed become irreversible so the producer schedule
    def isSetProdsBlockNumIrr():
            return producerNode.getIrreversibleBlockNum() >= setProdsBlockNum
    Utils.waitForBool(isSetProdsBlockNumIrr, timeout=30, sleepTime=0.1)
    # Once it is irreversible, immediately pause the producer so the promoted producer schedule is not cleared
    producerNode.processCurlCmd("producer", "pause", "")

    producerNode.kill(signal.SIGTERM)

    # Create the snapshot and rename it to avoid name conflict later on
    res = irrNode.createSnapshot()
    beforeShutdownSnapshotPath = res["snapshot_name"]
    snapshotPathWithoutExt, snapshotExt = os.path.splitext(beforeShutdownSnapshotPath)
    os.rename(beforeShutdownSnapshotPath, snapshotPathWithoutExt + "_before_shutdown" + snapshotExt)

    # Restart irr node and ensure the snapshot is still identical
    irrNode.kill(signal.SIGTERM)
    isRelaunchSuccess = irrNode.relaunch(irrNodeId, "", timeout=5, cachePopen=True)
    assert isRelaunchSuccess, "Fail to relaunch"
    res = irrNode.createSnapshot()
    afterShutdownSnapshotPath = res["snapshot_name"]
    assert filecmp.cmp(beforeShutdownSnapshotPath, afterShutdownSnapshotPath), "snapshot is not identical"

    testSuccessful = True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
