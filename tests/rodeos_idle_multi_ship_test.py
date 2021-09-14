#!/usr/bin/env python3

from testUtils import Utils
from TestHelper import TestHelper
from rodeos_utils import RodeosCluster
from TestHelper import AppArgs
###############################################################
# rodeos_idle_multi_ship_test
# 
#   This test verifies launch of a cluster of several ship and rodeos nodes in idle state, rodeos receives (empty)
#   blocks from SHiP. Rodeos and SHiP connections are either Unix socket or TCP/IP.
#
###############################################################

Print=Utils.Print

extraArgs=AppArgs()
extraArgs.add_bool("--eos-vm-oc-enable", "Use OC for rodeos")

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"}, extraArgs)
enableOC=args.eos_vm_oc_enable

TestHelper.printSystemInfo("BEGIN")
testSuccessful=False
def launch_cluster(num_ships, num_rodeos, unix_socket_option, eos_vm_oc_enable=False):
    with RodeosCluster(args.dump_error_details,
            args.keep_logs,
            args.leave_running,
            args.clean_run, unix_socket_option,
            'test.filter', './tests/test_filter.wasm',
            eos_vm_oc_enable,
            num_rodeos,
            num_ships) as cluster:

        Print("Testing cluster of {} ship nodes and {} rodeos nodes connecting through {}"\
            .format(num_ships, num_rodeos, (lambda x: 'Unix Socket' if (x==True) else 'TCP')(unix_socket_option)))
        for i in range(num_rodeos):
            assert cluster.waitRodeosReady(i), "Rodeos failed to stand up for a cluster of {} ship node and {} rodeos node".format(num_ships, num_rodeos)

        # Big enough to have new blocks produced
        numBlocks=120
        assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks for a cluster of {} ship node and {} rodeos node"\
            .format(numBlocks, num_ships, num_rodeos)
        
        for i in range(num_rodeos):
            assert cluster.allBlocksReceived(numBlocks, i), "Rodeos #{} did not receive {} blocks in a cluster of {} ship node and {} rodeos node"\
            .format(i, numBlocks, num_ships, num_rodeos)

        cluster.setTestSuccessful(True)

# Test cases: (1 ships, 2 rodeos), (2 ships, 3 rodeos)
numSHiPs=[1, 2]
numRodeos=[2, 3]
NumTestCase=len(numSHiPs)
for i in [True, False]: # True means Unix-socket option, False means TCP/IP
    for j in range(NumTestCase):
        launch_cluster(numSHiPs[j], numRodeos[j], i, enableOC)

testSuccessful=True
errorCode = 0 if testSuccessful else 1
exit(errorCode)
