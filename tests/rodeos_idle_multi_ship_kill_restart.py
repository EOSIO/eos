#!/usr/bin/env python3

from testUtils import Utils
from TestHelper import TestHelper
from rodeos_utils import RodeosCluster
import signal
from TestHelper import AppArgs

###############################################################
# rodeos_idle_multi_ship_kill_restart
# 
#   Test senario: first launch a cluster of 2 SHiPs and 2 Rodeos, verifies cluster is operating properly and it is stable.
#   Stop a rodeos node and verify the other rodeos is receiving blocks.
#   Restart rodeos and verify that rodeos receives blocks and has all the blocks 
#   Stop ShiP and verify that the other SHiP and rodeos node are operating properly.
#   Restart ShiP and verify that rodeos listening to its endpoint is receiving blocks.
#
###############################################################
Print=Utils.Print

extraArgs=AppArgs()
extraArgs.add_bool("--eos-vm-oc-enable", "Use OC for rodeos")
extraArgs.add_bool("--clean-restart", "Use for clean restart of SHiP and Rodeos")

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"}, extraArgs)
enableOC=args.eos_vm_oc_enable
cleanRestart=args.clean_restart

TestHelper.printSystemInfo("BEGIN")
testSuccessful=False

def launch_cluster(num_ships, num_rodeos, unix_socket, cleanRestart, killSignal, eos_vm_oc_enable=False):
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

        # Stop rodeosId=1
        rodeosKilledId=1
        SHiPIdHostRodeos=cluster.rodeosShipConnectionMap[rodeosKilledId]
        Print("Stopping rodeos #{} with {} signal".format(rodeosKilledId, (lambda x: 'SIGINT' if (x==signal.SIGINT) else 'SIGTERM')(killSignal)))
        cluster.stopRodeos(killSignal, rodeosKilledId)

        # Producing 10 more blocks
        currentBlockNum=cluster.prodNode.getHeadBlockNum()
        numBlocks= currentBlockNum + 10
        assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks for a cluster of {} ship node and {} rodeos node"\
            .format(numBlocks, num_ships, num_rodeos)

        # Verify that the other rodeos instances receiving blocks
        for i in range(num_rodeos):
            if i != rodeosKilledId:
                assert cluster.allBlocksReceived(numBlocks, i), "Rodeos #{} did not receive {} blocks after a rodeos node shutdown"\
                    .format(i, numBlocks, num_ships, num_rodeos)

        # Restarting rodeos        
        Print("{} restart of rodeos #{}".format((lambda x: 'Clean mode' if (x==True) else 'Non-clean mode')(cleanRestart), rodeosKilledId))
        cluster.restartRodeos(SHiPIdHostRodeos, rodeosKilledId, cleanRestart)     
        print("Wait for Rodeos #{} to get ready after a restart".format(rodeosKilledId))
        cluster.waitRodeosReady(rodeosKilledId)
        
        # Producing 10 more blocks after rodeos restart
        currentBlockNum=cluster.prodNode.getHeadBlockNum()
        numBlocks= currentBlockNum + 10
        assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks for a cluster of {} ship node and {} rodeos node"\
            .format(numBlocks, num_ships, num_rodeos)
        
        # verify that rodeos receives all blocks from start to now
        assert cluster.allBlocksReceived(numBlocks, rodeosKilledId), "Rodeos #{} did not receive {} blocks after the other rodeos node shutdown"\
                .format(rodeosKilledId, numBlocks, num_ships, num_rodeos)
        
        # Stop ShipId = 1
        shipKilledId=1
        Print("Stopping SHiP #{} with {} signal".format(shipKilledId, (lambda x: 'SIGINT' if (x==signal.SIGINT) else 'SIGTERM')(killSignal)))
        cluster.stopShip(killSignal, shipKilledId)

        # Producing 10 more blocks after ShiP stop
        currentBlockNum=cluster.prodNode.getHeadBlockNum()
        numBlocks= currentBlockNum + 10
        assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks for a cluster of {} ship node and {} rodeos node"\
            .format(numBlocks, num_ships, num_rodeos)

        # verify that other rodeos nodes receiveing blocks without interruption
        for i in cluster.shipNodeIdPortsNodes:
            if i != shipKilledId:
                for j in cluster.ShiprodeosConnectionMap[i]:
                    assert cluster.allBlocksReceived(numBlocks, j), "Rodeos #{} did not receive {} blocks after a rodeos node shutdown"\
                        .format(j, numBlocks, num_ships - 1, num_rodeos)

        # Restart Ship
        Print("Restart of SHiP #{}".format(shipKilledId))
        cluster.restartShip(cleanRestart, shipKilledId)
        newShipNode = cluster.cluster.getNode(shipKilledId)
        newShipNode.waitForLibToAdvance()

        # Producing 10 more blocks after ShiP restart
        currentBlockNum=cluster.prodNode.getHeadBlockNum()
        numBlocks= currentBlockNum + 10
        assert cluster.produceBlocks(numBlocks), "Nodeos failed to produce {} blocks for a cluster of {} ship node and {} rodeos node"\
            .format(numBlocks, num_ships, num_rodeos)

        # Verify that the rodeos node listening to the newly started Ship is receiving blocks.
        for j in cluster.ShiprodeosConnectionMap[shipKilledId]:
            assert cluster.allBlocksReceived(numBlocks, j), "Rodeos #{} did not receive {} blocks after a rodeos node shutdown"\
                    .format(j, numBlocks, num_ships, num_rodeos)

        cluster.setTestSuccessful(True)



# Test cases: (2 ships, 2 rodeos)
numSHiPs=[2]
numRodeos=[2]
NumTestCase=len(numSHiPs)
for i in [False, True]: # True means Unix-socket, False means TCP/IP
    for j in range(NumTestCase):
        for killSignal in [signal.SIGINT, signal.SIGTERM]: 
            launch_cluster(numSHiPs[j], numRodeos[j], i, cleanRestart, killSignal, enableOC)


testSuccessful=True
errorCode = 0 if testSuccessful else 1
exit(errorCode)
