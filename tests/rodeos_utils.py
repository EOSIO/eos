#!/usr/bin/env python3

from testUtils import Utils
from datetime import datetime
from datetime import timedelta
import time
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import BlockType
from TestHelper import TestHelper
from TestHelper import AppArgs

import json
import os
import subprocess
import re
import shutil
import signal
import time
import sys

###############################################################
# rodeos_utils
# 
# This file contains common utilities for managing producer,
#   ship, and rodeos. It supports a cluster of one producer,
#   one SHiP, and multiple rodeos'.
#
###############################################################
class RodeosCluster(object):
    SHIP_HOST_PORT="127.0.0.1:9999"

    def __init__(self, dump_error_details, keep_logs, leave_running, clean_run, unix_socket, filterName, filterWasm, enableOC=False, numRodeos=1):
        self.cluster=Cluster(walletd=True)
        self.dumpErrorDetails=dump_error_details
        self.keepLogs=keep_logs
        self.walletMgr=WalletMgr(True, port=TestHelper.DEFAULT_WALLET_PORT)
        self.testSuccessful=False
        self.killAll=clean_run
        self.killEosInstances=not leave_running
        self.killWallet=not leave_running
        self.clean_run=clean_run
        self.unix_socket=unix_socket

        self.numRodeos=numRodeos
        self.rodeosDir=[]
        self.rodeos=[]
        self.rodeosStdout=[None] * numRodeos
        self.rodeosStderr=[None] * numRodeos
        self.wqlHostPort=[]
        self.wqlEndPoints=[]

        port=8880
        for i in range(numRodeos):
            self.rodeosDir.append(os.path.join(os.getcwd(), 'var/lib/rodeos' + str(i)))
            shutil.rmtree(self.rodeosDir[i], ignore_errors=True)
            os.makedirs(self.rodeosDir[i], exist_ok=True)
            self.wqlHostPort.append("127.0.0.1:" + str(port))
            self.wqlEndPoints.append("http://" + self.wqlHostPort[i] + "/")
            port+=i

        self.filterName = filterName
        self.filterWasm = filterWasm
        self.OCArg=["--eos-vm-oc-enable"] if enableOC else []

    def __enter__(self):
        self.cluster.setWalletMgr(self.walletMgr)
        self.cluster.killall(allInstances=self.clean_run)
        self.cluster.cleanup()
        Utils.Print("Stand up cluster")
        specificExtraNodeosArgs={}
        # non-producing nodes are at the end of the cluster's nodes, so reserving the last one for SHiP node
        self.producerNodeId=0
        self.shipNodeId=1
        specificExtraNodeosArgs[self.shipNodeId]="--plugin eosio::state_history_plugin --trace-history --chain-state-history --state-history-endpoint {} --disable-replay-opts --plugin eosio::net_api_plugin ".format(self.SHIP_HOST_PORT)

        if self.unix_socket:
            specificExtraNodeosArgs[shipNodeId] += "--state-history-unix-socket-path ship.sock"

        if self.cluster.launch(pnodes=1, totalNodes=2, totalProducers=1, useBiosBootFile=False, specificExtraNodeosArgs=specificExtraNodeosArgs) is False:
            Utils.cmdError("launcher")
            Utils.errorExit("Failed to stand up eos cluster.")

        self.shipNode = self.cluster.getNode(self.shipNodeId)
        self.prodNode = self.cluster.getNode(self.producerNodeId)

        #verify nodes are in sync and advancing
        self.cluster.waitOnClusterSync(blockAdvancing=5)
        Utils.Print("Cluster in Sync")

        # Shut down bios node such that the cluster contains only one producer,
        # which makes SHiP not fork
        self.cluster.biosNode.kill(signal.SIGTERM)

        for i in range(self.numRodeos):
            self.restartRodeos(i, clean=True)

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        TestHelper.shutdown(self.cluster, self.walletMgr, testSuccessful=self.testSuccessful, killEosInstances=self.killEosInstances, killWallet=self.killWallet, keepLogs=self.keepLogs, cleanRun=self.killAll, dumpErrorDetails=self.dumpErrorDetails)

        for i in range(self.numRodeos):
            if self.rodeos[i] is not None:
                self.rodeos[i].send_signal(signal.SIGTERM)
                self.rodeos[i].wait()
            if self.rodeosStdout[i] is not None:
                self.rodeosStdout[i].close()
            if self.rodeosStderr[i] is not None:
                self.rodeosStderr[i].close()
            if not self.keepLogs:
                shutil.rmtree(self.rodeosDir[i], ignore_errors=True)

    def relaunchNode(self, node: Node, chainArg="", relaunchAssertMessage="Fail to relaunch", clean=False):
        if clean:
            shutil.rmtree(Utils.getNodeDataDir(node.nodeId))
            os.makedirs(Utils.getNodeDataDir(node.nodeId))

        # skipGenesis=False starts the same chain
        isRelaunchSuccess=node.relaunch(chainArg=chainArg,                      timeout=10, skipGenesis=False, cachePopen=True)
        time.sleep(1) # Give a second to replay or resync if needed
        assert isRelaunchSuccess, relaunchAssertMessage
        return isRelaunchSuccess

    def restartProducer(self, clean):
        # need to reenable producing after restart
        self.relaunchNode(self.prodNode, chainArg="-e -p defproducera ", clean=clean)

    def stopProducer(self, killSignal):
        self.prodNode.kill(killSignal)

    def restartShip(self, clean):
        if self.unix_socket:
            arg += "--state-history-unix-socket-path ship.sock"
        self.relaunchNode(self.shipNode, clean=clean)

    def stopShip(self, killSignal):
        self.shipNode.kill(killSignal)

    def restartRodeos(self, rodeosId=0, clean=True):
        assert(rodeosId >= 0 and rodeosId < self.numRodeos)

        if clean:
            if self.rodeosStdout[rodeosId] is not None:
                self.rodeosStdout[rodeosId].close()
            if self.rodeosStderr[rodeosId] is not None:
                self.rodeosStderr[rodeosId].close()
            shutil.rmtree(self.rodeosDir[rodeosId], ignore_errors=True)
            os.makedirs(self.rodeosDir[rodeosId], exist_ok=True)
            self.rodeosStdout[rodeosId]=open(os.path.join(self.rodeosDir[rodeosId], "stdout.out"), "w")
            self.rodeosStderr[rodeosId]=open(os.path.join(self.rodeosDir[rodeosId], "stderr.out"), "w")

        self.rodeos.append(subprocess.Popen(['./programs/rodeos/rodeos', '--rdb-database', os.path.join(self.rodeosDir[rodeosId],'rocksdb'),
                                        '--clone-connect-to', self.SHIP_HOST_PORT, '--wql-listen', self.wqlHostPort[rodeosId], '--wql-threads', '8', '--filter-name', self.filterName , '--filter-wasm', self.filterWasm ] + self.OCArg,
                                       stdout=self.rodeosStdout[rodeosId],
                                       stderr=self.rodeosStderr[rodeosId]))

    # SIGINT to simulate CTRL-C
    def stopRodeos(self, killSignal=signal.SIGINT, rodeosId=0):
        assert(rodeosId >= 0 and rodeosId < self.numRodeos)
        if self.rodeos[rodeosId] is not None:
            self.rodeos[rodeosId].send_signal(killSignal)
            self.rodeos[rodeosId].wait()
            self.rodeos[rodeosId] = None

    def waitRodeosReady(self, rodeosId=0):
        assert(rodeosId >= 0 and rodeosId < self.numRodeos)
        return Utils.waitForTruth(lambda:  Utils.runCmdArrReturnStr(['curl', '-H', 'Accept: application/json', self.wqlEndPoints[rodeosId] + 'v1/chain/get_info'], silentErrors=True) != "" , timeout=30)

    def getBlock(self, blockNum, rodeosId=0):
        assert(rodeosId >= 0 and rodeosId < self.numRodeos)
        request_body = { "block_num_or_id": blockNum }
        return Utils.runCmdArrReturnJson(['curl', '-X', 'POST', '-H', 'Content-Type: application/json', '-H', 'Accept: application/json', self.wqlEndPoints[rodeosId] + 'v1/chain/get_block', '--data', json.dumps(request_body)])
        
    def getInfo(self, rodeosId=0):
        assert(rodeosId >= 0 and rodeosId < self.numRodeos)
        return Utils.runCmdArrReturnJson(['curl', '-H', 'Accept: application/json', self.wqlEndPoints[rodeosId] + 'v1/chain/get_info'])

    def produceBlocks(self, numBlocks):
        Utils.Print("Wait for Nodeos to produce {} blocks".format(numBlocks))
        return self.prodNode.waitForBlock(numBlocks, blockType=BlockType.lib)

    def allBlocksReceived(self, lastBlockNum, rodeosId=0):
        assert(rodeosId >= 0 and rodeosId < self.numRodeos)

        Utils.Print("Verifying {} blocks has received by rodeos #{}".format(lastBlockNum, rodeosId))
        headBlockNum=0
        numSecsSleep=0
        while headBlockNum < lastBlockNum:
            response = self.getInfo(rodeosId)
            assert 'head_block_num' in response, "Rodeos response does not contain head_block_num, response body = {}".format(json.dumps(response))
            headBlockNum = int(response['head_block_num'])
            Utils.Print("head_block_num {}".format(headBlockNum))
            if headBlockNum < lastBlockNum:
                if numSecsSleep >= 60:
                    Utils.Print("Rodeos did not receive block {} after {} seconds. Only block {} received".format(lastBlockNum, numSecsSleep, headBlockNum))
                    return False
                time.sleep(1)
                numSecsSleep+=1
        Utils.Print("{} blocks has received".format(lastBlockNum))
        
        # find the first block number
        firstBlockNum=0
        for i in range(1, lastBlockNum+1):
            response = self.getBlock(i, rodeosId)
            if "block_num" in response:
                firstBlockNum=response["block_num"]
                Utils.Print("firstBlockNum is {}".format(firstBlockNum))
                break
        assert firstBlockNum >= 1, "firstBlockNum not found"

        Utils.Print("Verifying blocks were not skipped")
        for blockNum in range(firstBlockNum, lastBlockNum+1):
            response = self.getBlock(blockNum, rodeosId)
            #Utils.Print("response body = {}".format(json.dumps(response)))
            if "block_num" in response:
                assert response["block_num"] == blockNum, "Rodeos responds with wrong block {0}, response body = {1}".format(i, json.dumps(response))
        Utils.Print("No blocks were skipped")

        return True

    def setTestSuccessful(self, testSuccessful):
        self.testSuccessful=testSuccessful
