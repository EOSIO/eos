#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper
from testUtils import Account

import urllib.request
import re
import os
import time
import signal
import subprocess
import shutil


###############################################################
# nodeos_irreversible_mode_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
# -v --leave-running --clean-run
###############################################################

Print = Utils.Print
errorExit = Utils.errorExit
cmdError = Utils.cmdError
relaunchTimeout = 10
numOfProducers = 4
totalNodes = 10

# Parse command line arguments
args = TestHelper.parse_args({"-v","--clean-run","--dump-error-details","--leave-running","--keep-logs"})
Utils.Debug = args.v
killAll=args.clean_run
dumpErrorDetails=args.dump_error_details
dontKill=args.leave_running
killEosInstances=not dontKill
killWallet=not dontKill
keepLogs=args.keep_logs

# Setup cluster and it's wallet manager
walletMgr=WalletMgr(True)
cluster=Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

def makeSnapshot(nodeId):
  req = urllib.request.Request("http://127.0.0.1:{}/v1/producer/create_snapshot".format(8888 + int(nodeId)))
  urllib.request.urlopen(req)

def backupBlksDir(nodeId):
   dataDir = Utils.getNodeDataDir(nodeId)
   sourceDir = os.path.join(dataDir, "blocks")
   destinationDir = os.path.join(os.path.dirname(dataDir), os.path.basename(dataDir) + "-backup", "blocks")
   shutil.copytree(sourceDir, destinationDir)

def recoverBackedupBlksDir(nodeId):
   dataDir = Utils.getNodeDataDir(nodeId)
   # Delete existing one and copy backed up one
   existingBlocksDir = os.path.join(dataDir, "blocks")
   backedupBlocksDir = os.path.join(os.path.dirname(dataDir), os.path.basename(dataDir) + "-backup", "blocks")
   shutil.rmtree(existingBlocksDir, ignore_errors=True)
   shutil.copytree(backedupBlocksDir, existingBlocksDir)

def getLatestSnapshot(nodeId):
   snapshotDir = os.path.join(Utils.getNodeDataDir(nodeId), "snapshots")
   snapshotDirContents = os.listdir(snapshotDir)
   assert len(snapshotDirContents) > 0
   snapshotDirContents.sort()
   return os.path.join(snapshotDir, snapshotDirContents[-1])


def removeReversibleBlks(nodeId):
   dataDir = Utils.getNodeDataDir(nodeId)
   reversibleBlks = os.path.join(dataDir, "blocks", "reversible")
   shutil.rmtree(reversibleBlks, ignore_errors=True)

def removeState(nodeId):
   dataDir = Utils.getNodeDataDir(nodeId)
   state = os.path.join(dataDir, "state")
   shutil.rmtree(state, ignore_errors=True)

def getHeadLibAndForkDbHead(node: Node):
   info = node.getInfo()
   assert info is not None, "Fail to retrieve info from the node, the node is currently having a problem"
   head = int(info["head_block_num"])
   lib = int(info["last_irreversible_block_num"])
   forkDbHead =  int(info["fork_db_head_block_num"])
   return head, lib, forkDbHead

# Wait for some time until LIB advance
def waitForBlksProducedAndLibAdvanced():
   requiredConfirmation = int(2 / 3 * numOfProducers) + 1
   maxNumOfBlksReqToConfirmLib = (12 * requiredConfirmation - 1) * 2
   # Give 6 seconds buffer time
   bufferTime = 6
   timeToWait = maxNumOfBlksReqToConfirmLib / 2 + bufferTime
   time.sleep(timeToWait)

# Ensure that the relaunched node received blks from producers, in other words head and lib is advancing
def ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest):
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   waitForBlksProducedAndLibAdvanced()
   headAfterWaiting, libAfterWaiting, forkDbHeadAfterWaiting = getHeadLibAndForkDbHead(nodeToTest)
   assert headAfterWaiting > head and libAfterWaiting > lib and forkDbHeadAfterWaiting > forkDbHead, \
      "Either Head ({} -> {})/ Lib ({} -> {})/ Fork Db Head ({} -> {}) is not advancing".format(head, headAfterWaiting, lib, libAfterWaiting, forkDbHead, forkDbHeadAfterWaiting)

# Confirm the head lib and fork db of irreversible mode
# Under any condition of irreversible mode:
# - forkDbHead >= head == lib
# headLibAndForkDbHeadBeforeSwitchMode should be only passed IF production is disabled, otherwise it provides erroneous check
# When comparing with the the state before node is switched:
# - head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
def confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode=None):
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   assert head == lib, "Head ({}) should be equal to lib ({})".format(head, lib)
   assert forkDbHead >= head, "Fork db head ({}) should be larger or equal to the head ({})".format(forkDbHead, head)

   if headLibAndForkDbHeadBeforeSwitchMode:
      headBeforeSwitchMode, libBeforeSwitchMode, forkDbHeadBeforeSwitchMode = headLibAndForkDbHeadBeforeSwitchMode
      assert head == libBeforeSwitchMode, "Head ({}) should be equal to lib before switch mode ({})".format(head, libBeforeSwitchMode)
      assert lib == libBeforeSwitchMode, "Lib ({}) should be equal to lib before switch mode ({})".format(lib, libBeforeSwitchMode)
      assert forkDbHead == headBeforeSwitchMode and forkDbHead == forkDbHeadBeforeSwitchMode, \
         "Fork db head ({}) should be equal to head before switch mode ({}) and fork db head before switch mode ({})".format(forkDbHead, headBeforeSwitchMode, forkDbHeadBeforeSwitchMode)

# Confirm the head lib and fork db of speculative mode
# Under any condition of speculative mode:
# - forkDbHead == head >= lib
# headLibAndForkDbHeadBeforeSwitchMode should be only passed IF production is disabled, otherwise it provides erroneous check
# When comparing with the the state before node is switched:
# - head == forkDbHeadBeforeSwitchMode == forkDbHead and lib == headBeforeSwitchMode == libBeforeSwitchMode
def confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode=None):
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   assert head >= lib, "Head should be larger or equal to lib (head: {}, lib: {})".format(head, lib)
   assert head == forkDbHead, "Head ({}) should be equal to fork db head ({})".format(head, forkDbHead)

   if headLibAndForkDbHeadBeforeSwitchMode:
      headBeforeSwitchMode, libBeforeSwitchMode, forkDbHeadBeforeSwitchMode = headLibAndForkDbHeadBeforeSwitchMode
      assert head == forkDbHeadBeforeSwitchMode, "Head ({}) should be equal to fork db head before switch mode ({})".format(head, forkDbHeadBeforeSwitchMode)
      assert lib == headBeforeSwitchMode and lib == libBeforeSwitchMode, \
         "Lib ({}) should be equal to head before switch mode ({}) and lib before switch mode ({})".format(lib, headBeforeSwitchMode, libBeforeSwitchMode)
      assert forkDbHead == forkDbHeadBeforeSwitchMode, \
         "Fork db head ({}) should be equal to fork db head before switch mode ({}) ".format(forkDbHead, forkDbHeadBeforeSwitchMode)

def relaunchNode(node: Node, nodeId, chainArg="", addOrSwapFlags=None, relaunchAssertMessage="Fail to relaunch"):
   isRelaunchSuccess = node.relaunch(nodeId, chainArg=chainArg, addOrSwapFlags=addOrSwapFlags, timeout=relaunchTimeout, cachePopen=True)
   time.sleep(1) # Give a second to replay or resync if needed
   assert isRelaunchSuccess, relaunchAssertMessage
   return isRelaunchSuccess

# List to contain the test result message
testResultMsgs = []
testSuccessful = False
try:
   # Kill any existing instances and launch cluster
   TestHelper.printSystemInfo("BEGIN")
   cluster.killall(allInstances=killAll)
   cluster.cleanup()
   cluster.launch(
      prodCount=numOfProducers,
      totalProducers=numOfProducers,
      totalNodes=totalNodes,
      pnodes=1,
      useBiosBootFile=False,
      topo="mesh",
      specificExtraNodeosArgs={
         0:"--enable-stale-production",
         4:"--read-mode irreversible",
         6:"--read-mode irreversible",
         9:"--plugin eosio::producer_api_plugin"})

   producingNodeId = 0
   producingNode = cluster.getNode(producingNodeId)

   def stopProdNode():
      if not producingNode.killed:
         producingNode.kill(signal.SIGTERM)

   def startProdNode():
      if producingNode.killed:
         relaunchNode(producingNode, producingNodeId)

   # Give some time for it to produce, so lib is advancing
   waitForBlksProducedAndLibAdvanced()

   # Kill all nodes, so we can test all node in isolated environment
   for clusterNode in cluster.nodes:
      clusterNode.kill(signal.SIGTERM)
   cluster.biosNode.kill(signal.SIGTERM)

   # Wrapper function to execute test
   # This wrapper function will resurrect the node to be tested, and shut it down by the end of the test
   def executeTest(nodeIdOfNodeToTest, runTestScenario):
      testResult = False
      try:
         # Relaunch killed node so it can be used for the test
         nodeToTest = cluster.getNode(nodeIdOfNodeToTest)
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, relaunchAssertMessage="Fail to relaunch before running test scenario")

         # Run test scenario
         runTestScenario(nodeIdOfNodeToTest, nodeToTest)
         testResultMsgs.append("!!!TEST CASE #{} ({}) IS SUCCESSFUL".format(nodeIdOfNodeToTest, runTestScenario.__name__))
         testResult = True
      except Exception as e:
         testResultMsgs.append("!!!BUG IS CONFIRMED ON TEST CASE #{} ({}): {}".format(nodeIdOfNodeToTest, runTestScenario.__name__, e))
      finally:
         # Kill node after use
         if not nodeToTest.killed: nodeToTest.kill(signal.SIGTERM)
      return testResult

   # 1st test case: Replay in irreversible mode with reversible blks
   # Expectation: Node replays and launches successfully and forkdb head, head, and lib matches the irreversible mode expectation
   def replayInIrrModeWithRevBlks(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)

      # Kill node and replay in irreversible mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")

      # Confirm state
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 2nd test case: Replay in irreversible mode without reversible blks
   # Expectation: Node replays and launches successfully and forkdb head, head, and lib matches the irreversible mode expectation
   def replayInIrrModeWithoutRevBlks(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)

      # Shut down node, remove reversible blks and relaunch
      nodeToTest.kill(signal.SIGTERM)
      removeReversibleBlks(nodeIdOfNodeToTest)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")

      # Ensure the node condition is as expected after relaunch
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 3rd test case: Switch mode speculative -> irreversible without replay
   # Expectation: Node switches mode successfully and forkdb head, head, and lib matches the irreversible mode expectation
   def switchSpecToIrrMode(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)

      # Kill and relaunch in irreversible mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible")

      # Ensure the node condition is as expected after relaunch
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 4th test case: Switch mode irreversible -> speculative without replay
   # Expectation: Node switches mode successfully and forkdb head, head, and lib matches the speculative mode expectation
   def switchIrrToSpecMode(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)

      # Kill and relaunch in speculative mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, addOrSwapFlags={"--read-mode": "speculative"})

      # Ensure the node condition is as expected after relaunch
      confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 5th test case: Switch mode speculative -> irreversible without replay and connected to producing node
   # Expectation: Node switches mode successfully
   #              and the head and lib should be advancing after some blocks produced
   #              and forkdb head, head, and lib matches the irreversible mode expectation
   def switchSpecToIrrModeWithConnectedToProdNode(nodeIdOfNodeToTest, nodeToTest):
      try:
         startProdNode()

         # Kill and relaunch in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait for some blks to be produced and lib advance
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible")

         # Ensure the node condition is as expected after relaunch
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         stopProdNode()

   # 6th test case: Switch mode irreversible -> speculative without replay and connected to producing node
   # Expectation: Node switches mode successfully
   #              and the head and lib should be advancing after some blocks produced
   #              and forkdb head, head, and lib matches the speculative mode expectation
   def switchIrrToSpecModeWithConnectedToProdNode(nodeIdOfNodeToTest, nodeToTest):
      try:
         startProdNode()

         # Kill and relaunch in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait for some blks to be produced and lib advance)
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, addOrSwapFlags={"--read-mode": "speculative"})

         # Ensure the node condition is as expected after relaunch
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest)
      finally:
         stopProdNode()

   # 7th test case: Replay in irreversible mode with reversible blks while connected to producing node
   # Expectation: Node replays and launches successfully
   #              and the head and lib should be advancing after some blocks produced
   #              and forkdb head, head, and lib matches the irreversible mode expectation
   def replayInIrrModeWithRevBlksAndConnectedToProdNode(nodeIdOfNodeToTest, nodeToTest):
      try:
         startProdNode()
         # Kill node and replay in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")

         # Ensure the node condition is as expected after relaunch
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         stopProdNode()

   # 8th test case: Replay in irreversible mode without reversible blks while connected to producing node
   # Expectation: Node replays and launches successfully
   #              and the head and lib should be advancing after some blocks produced
   #              and forkdb head, head, and lib matches the irreversible mode expectation
   def replayInIrrModeWithoutRevBlksAndConnectedToProdNode(nodeIdOfNodeToTest, nodeToTest):
      try:
         startProdNode()

         # Kill node, remove rev blks and then replay in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         removeReversibleBlks(nodeIdOfNodeToTest)
         waitForBlksProducedAndLibAdvanced() # Wait
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")

         # Ensure the node condition is as expected after relaunch
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         stopProdNode()

   # 9th test case: Switch to speculative mode while using irreversible mode snapshots and using backed up speculative blocks
   # Expectation: Node replays and launches successfully
   #              and the head and lib should be advancing after some blocks produced
   #              and forkdb head, head, and lib should stay the same after relaunch
   def switchToSpecModeWithIrrModeSnapshot(nodeIdOfNodeToTest, nodeToTest):
      try:
         # Kill node and backup blocks directory of speculative mode
         headLibAndForkDbHeadBeforeShutdown = getHeadLibAndForkDbHead(nodeToTest)
         nodeToTest.kill(signal.SIGTERM)
         backupBlksDir(nodeIdOfNodeToTest)

         # Relaunch in irreversible mode and create the snapshot
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible")
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
         makeSnapshot(nodeIdOfNodeToTest)
         nodeToTest.kill(signal.SIGTERM)

         # Start from clean data dir, recover back up blocks, and then relaunch with irreversible snapshot
         removeState(nodeIdOfNodeToTest)
         recoverBackedupBlksDir(nodeIdOfNodeToTest) # this function will delete the existing blocks dir first
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --snapshot {}".format(getLatestSnapshot(nodeIdOfNodeToTest)), addOrSwapFlags={"--read-mode": "speculative"})
         confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest)
         # Ensure it automatically replays "reversible blocks", i.e. head lib and fork db should be the same
         headLibAndForkDbHeadAfterRelaunch = getHeadLibAndForkDbHead(nodeToTest)
         assert headLibAndForkDbHeadBeforeShutdown == headLibAndForkDbHeadAfterRelaunch, \
            "Head, Lib, and Fork Db after relaunch is different {} vs {}".format(headLibAndForkDbHeadBeforeShutdown, headLibAndForkDbHeadAfterRelaunch)

         # Start production and wait until lib advance, ensure everything is alright
         startProdNode()
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)

         # Note the head, lib and fork db head
         stopProdNode()
         headLibAndForkDbHeadBeforeShutdown = getHeadLibAndForkDbHead(nodeToTest)
         nodeToTest.kill(signal.SIGTERM)

         # Relaunch the node again (using the same snapshot)
         # This time ensure it automatically replays both "irreversible blocks" and "reversible blocks", i.e. the end result should be the same as before shutdown
         removeState(nodeIdOfNodeToTest)
         relaunchNode(nodeToTest, nodeIdOfNodeToTest)
         headLibAndForkDbHeadAfterRelaunch = getHeadLibAndForkDbHead(nodeToTest)
         assert headLibAndForkDbHeadBeforeShutdown == headLibAndForkDbHeadAfterRelaunch, \
            "Head, Lib, and Fork Db after relaunch is different {} vs {}".format(headLibAndForkDbHeadBeforeShutdown, headLibAndForkDbHeadAfterRelaunch)
      finally:
         stopProdNode()

   # Start executing test cases here
   testResults = []
   testResults.append( executeTest(1, replayInIrrModeWithRevBlks) )
   testResults.append( executeTest(2, replayInIrrModeWithoutRevBlks) )
   testResults.append( executeTest(3, switchSpecToIrrMode) )
   testResults.append( executeTest(4, switchIrrToSpecMode) )
   testResults.append( executeTest(5, switchSpecToIrrModeWithConnectedToProdNode) )
   testResults.append( executeTest(6, switchIrrToSpecModeWithConnectedToProdNode) )
   testResults.append( executeTest(7, replayInIrrModeWithRevBlksAndConnectedToProdNode) )
   testResults.append( executeTest(8, replayInIrrModeWithoutRevBlksAndConnectedToProdNode) )
   testResults.append( executeTest(9, switchToSpecModeWithIrrModeSnapshot) )

   testSuccessful = all(testResults)
finally:
   TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)
   # Print test result
   for msg in testResultMsgs: Print(msg)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
