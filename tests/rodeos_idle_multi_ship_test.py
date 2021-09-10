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
# rodeos_idle_multi_ship_test
# 
#   This test verifies launch of a cluster of several ship and rodeos nodes in idle state, rodeos receives (empty)
#   blocks from SHiP. Rodeos and SHiP connections are either Unix socket or TCP/IP.
#
###############################################################

Print=Utils.Print

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"})

TestHelper.printSystemInfo("BEGIN")
testSuccessful=False
def launch_cluster(num_ships, num_rodeos, unix_socket, eos_vm_oc_enable=False):
    with RodeosCluster(args.dump_error_details,
            args.keep_logs,
            args.leave_running,
            args.clean_run, unix_socket,
            'test.filter', './tests/test_filter.wasm',
            eos_vm_oc_enable,
            num_rodeos,
            num_ships) as cluster:

        Print("Testing cluster of {} ship nodes and {} rodeos nodes connecting through {}"\
            .format(num_ships, num_rodeos, (lambda x: 'Unix Socket' if (x==True) else 'TCP')(unix_socket)))
        assert cluster.waitRodeosReady(), "Rodeos failed to stand up for a cluster of {} ship node and {} rodeos node".format(num_ships, num_rodeos)

        # Big enough to have new blocks produced
        numBlocks=120
        assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks for a cluster of {} ship node and {} rodeos node"\
            .format(numBlocks, num_ships, num_rodeos)
        
        for i in range(num_rodeos):
            assert cluster.allBlocksReceived(numBlocks, i), "Rodeos #{} did not receive {} blocks in a cluster of {} ship node and {} rodeos node"\
            .format(i, numBlocks, num_ships, num_rodeos)

        cluster.setTestSuccessful(True)

# Test cases: (1 ship, 1 rodeos), (2 ships, 2 rodeos), (2 ships, 3 rodeos), (3 ships, 2 rodeos)
numSHiPs=[1, 2, 2, 3]
numRodeos=[1, 2, 3, 2]
NumTestCase=len(numSHiPs)
for i in [True, False]:
    for j in range(NumTestCase):
        launch_cluster(numSHiPs[j], numRodeos[j], i)

testSuccessful=True
errorCode = 0 if testSuccessful else 1
exit(errorCode)
