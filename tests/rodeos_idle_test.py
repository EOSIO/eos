#!/usr/bin/env python3

from testUtils import Utils
from datetime import datetime
from datetime import timedelta
import time
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper
from TestHelper import AppArgs
from rodeos_utils import RodeosCluster

import json
import os
import re
import shutil
import signal
import sys

###############################################################
# rodeos_idle_test
# 
# This test verifies in idle state, rodeos receives (empty)
#   blocks from SHiP and the number of RocksDB SST files does
#   not grow uncontrolledly.
#
###############################################################

Print=Utils.Print

appArgs=AppArgs()
appArgs.add(flag="--num-rodeos", type=int, help="How many rodeos' should be started", default=1)
appArgs.add_bool(flag="--unix-socket", help="Run ship over unix socket")
appArgs.add_bool("--eos-vm-oc-enable", "Use OC for rodeos")

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"}, applicationSpecificArgs=appArgs)

testSuccessful=False
TestHelper.printSystemInfo("BEGIN")

with RodeosCluster(args.dump_error_details,
        args.keep_logs,
        args.leave_running,
        args.clean_run, args.unix_socket,
        'test.filter', './tests/test_filter.wasm',
        args.eos_vm_oc_enable,
        args.num_rodeos) as cluster:

    assert cluster.waitRodeosReady(), "Rodeos failed to stand up"

    # Big enough to have new blocks produced
    numBlocks=120
    assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks".format(numBlocks)
    assert cluster.allBlocksReceived(numBlocks), "Rodeos did not receive {} blocks".format(numBlocks)

    testSuccessful=True
    cluster.setTestSuccessful(testSuccessful)
    
errorCode = 0 if testSuccessful else 1
exit(errorCode)
