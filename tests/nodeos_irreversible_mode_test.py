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


Print = Utils.Print
errorExit = Utils.errorExit
cmdError = Utils.cmdError
relaunchTimeout = 5

# Parse command line arguments
args = TestHelper.parse_args({"-v"})
Utils.Debug = args.v

# Setup cluster and it's wallet manager
walletMgr=WalletMgr(True)
cluster=Cluster(walletd=True)
cluster.setWalletMgr(walletMgr)

def removeReversibleBlks(nodeId):
   dataDir = Cluster.getDataDir(nodeId)
   reversibleBlks = os.path.join(dataDir, "blocks", "reversible")
   shutil.rmtree(reversibleBlks, ignore_errors=True)

def getHeadLibAndForkDbHead(node: Node):
   info = node.getInfo()
   head = int(info["head_block_num"])
   lib = int(info["last_irreversible_block_num"])
   forkDbHead =  int(info["fork_db_head_block_num"])
   return head, lib, forkDbHead

def waitForBlksProducedAndLibAdvanced():
   time.sleep(60)

def ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest):
   # Ensure that the relaunched node received blks from producers
   head, lib, _ = getHeadLibAndForkDbHead(nodeToTest)
   waitForBlksProducedAndLibAdvanced()
   headAfterWaiting, libAfterWaiting, _ = getHeadLibAndForkDbHead(nodeToTest)
   assert headAfterWaiting > head and libAfterWaiting > lib, "Head is not advancing"

# Confirm the head lib and fork db of irreversible mode
# Under any condition of irreversible mode: forkDbHead > head == lib
# headLibAndForkDbHeadBeforeSwitchMode should be only passed IF production is disabled, otherwise it provides erroneous check
# Comparing with the state before mode is switched: head == libBeforeSwitchMode and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
def confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode=None):
   # In irreversible mode, head should be equal to lib and not equal to fork Db blk num
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   assert head == lib, "Head ({}) should be equal to lib ({})".format(head, lib)
   assert forkDbHead > head, "Fork db head ({}) should be larger than the head ({})".format(forkDbHead, head)

   if headLibAndForkDbHeadBeforeSwitchMode:
      headBeforeSwitchMode, libBeforeSwitchMode, forkDbHeadBeforeSwitchMode = headLibAndForkDbHeadBeforeSwitchMode
      assert head == libBeforeSwitchMode, "Head ({}) should be equal to lib before switch mode ({})".format(head, libBeforeSwitchMode)
      assert forkDbHead == headBeforeSwitchMode, "Fork db head ({}) should be equal to head before switch mode ({})".format(forkDbHead, headBeforeSwitchMode)
      assert forkDbHead == forkDbHeadBeforeSwitchMode, "Fork db head ({}) should not be equal to fork db before switch mode ({})".format(forkDbHead, forkDbHeadBeforeSwitchMode)

# Confirm the head lib and fork db of speculative mode
# Under any condition of irreversible mode: forkDbHead == head > lib
# headLibAndForkDbHeadBeforeSwitchMode should be only passed IF production is disabled, otherwise it provides erroneous check
# Comparing with the state before mode is switched: head == forkDbHeadBeforeSwitchMode == forkDbHead and lib == libBeforeSwitchMode
def confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode=None):
   # In speculative mode, head should be equal to lib and not equal to fork Db blk num
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   assert head > lib, "Head should be larger than lib (head: {}, lib: {})".format(head, lib)
   assert head == forkDbHead, "Head ({}) should be equal to fork db head ({})".format(head, forkDbHead)

   if headLibAndForkDbHeadBeforeSwitchMode:
      _, libBeforeSwitchMode, forkDbHeadBeforeSwitchMode = headLibAndForkDbHeadBeforeSwitchMode
      assert head == forkDbHeadBeforeSwitchMode, "Head ({}) should be equal to fork db head before switch mode ({})".format(head, forkDbHeadBeforeSwitchMode)
      assert lib == libBeforeSwitchMode, "Lib ({}) should be equal to lib before switch mode ({})".format(lib, libBeforeSwitchMode)
      assert forkDbHead == forkDbHeadBeforeSwitchMode, "Fork db head ({}) should be equal to fork db before switch mode ({})".format(forkDbHead, forkDbHeadBeforeSwitchMode)

def relaunchNode(node: Node, nodeId, chainArg="", addOrSwapFlags=None, relaunchAssertMessage="Fail to relaunch"):
   isRelaunchSuccess = node.relaunch(nodeId, chainArg=chainArg, addOrSwapFlags=addOrSwapFlags, timeout=relaunchTimeout)
   assert isRelaunchSuccess, relaunchAssertMessage
   return isRelaunchSuccess

# List to contain the test result message
testResultMsgs = []
try:
   TestHelper.printSystemInfo("BEGIN")
   cluster.killall(allInstances=True)
   cluster.cleanup()
   numOfProducers = 4
   totalNodes = 12
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
         8:"--read-mode irreversible"})

   producingNodeId = 0
   producingNode = cluster.getNode(producingNodeId)

   def stopBlkProduction():
      if not producingNode.killed:
         producingNode.kill(signal.SIGTERM)

   def resumeBlkProduction():
      if producingNode.killed:
         producingNode.relaunch(producingNodeId, "", timeout=relaunchTimeout)

   # Give some time for it to produce, so lib is advancing
   waitForBlksProducedAndLibAdvanced()

   # Kill all nodes, so we can test all node in isolated environment
   for clusterNode in cluster.nodes:
      clusterNode.kill(signal.SIGTERM)
   cluster.biosNode.kill(signal.SIGTERM)

   # Wrapper function to execute test
   # This wrapper function will resurrect the node to be tested, and shut it down by the end of the test
   def executeTest(nodeIdOfNodeToTest, runTestScenario):
      try:
         nodeToTest = cluster.getNode(nodeIdOfNodeToTest)
         # Resurrect killed node to be tested
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, relaunchAssertMessage="Fail to relaunch before running test scenario")
         # Run test scenario
         runTestScenario(nodeIdOfNodeToTest, nodeToTest)
         # Kill node after use
         if not nodeToTest.killed: nodeToTest.kill(signal.SIGTERM)
         testResultMsgs.append("!!!TEST CASE #{} ({}) IS SUCCESSFUL".format(nodeIdOfNodeToTest, runTestScenario.__name__))
      except Exception as e:
         testResultMsgs.append("!!!BUG IS CONFIRMED ON TEST CASE #{} ({}): {}".format(nodeIdOfNodeToTest, runTestScenario.__name__, e))

   # 1st test case: Replay in irreversible mode with reversible blks
   # Expectation: Node replays and launches successfully
   # Current Bug: duplicate blk added error
   def replayInIrrModeWithRevBlks(nodeIdOfNodeToTest, nodeToTest):
      # Kill node and replay in irreversible mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")
      # Confirm state
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)


   # 2nd test case: Replay in irreversible mode without reversible blks
   # Expectation: Node replays and launches successfully with lib == head == libBeforeSwitchMode
   # Current Bug: last_irreversible_blk != the real lib (e.g. if lib is 1000, it replays up to 1000 saying head is 1000 and lib is 999)
   def replayInIrrModeWithoutRevBlksAndCompareState(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)
      # Shut node, remove reversible blks and relaunch
      nodeToTest.kill(signal.SIGTERM)
      removeReversibleBlks(nodeIdOfNodeToTest)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")
      # Confirm state
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 3rd test case: Switch mode speculative -> irreversible without replay and production disabled
   # Expectation: Node switches mode successfully
   def switchSpecToIrrMode(nodeIdOfNodeToTest, nodeToTest):
      # Relaunch in irreversible mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible")
      # Confirm state
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)

   # 4th test case: Switch mode irreversible -> speculative without replay and production disabled
   # Expectation: Node switches mode successfully
   def switchIrrToSpecMode(nodeIdOfNodeToTest, nodeToTest):
      # Relaunch in speculative mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, addOrSwapFlags={"--read-mode": "speculative"})
      # Confirm state
      confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest)

   # 5th test case: Switch mode irreversible -> speculative without replay and production enabled
   # Expectation: Node switches mode successfully and receives next blk from the producer
   # Current Bug: Fail to switch to irreversible mode, blk_validate_exception next blk in the future will be thrown
   def switchSpecToIrrModeWithProdEnabled(nodeIdOfNodeToTest, nodeToTest):
      try:
         # Resume blk production
         resumeBlkProduction()
         # Kill and relaunch in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait for some blks to be produced and lib advance
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible")
         # Ensure that the relaunched node received blks from producers
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         # Confirm state
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         # Stop blk production
         stopBlkProduction()

   # 6th test case: Switch mode irreversible -> speculative without replay and production enabled
   # Expectation: Node switches mode successfully and receives next blk from the producer
   # Current Bug: Node switches mode successfully, however, it fails to establish connection with the producing node
   def switchIrrToSpecModeWithProdEnabled(nodeIdOfNodeToTest, nodeToTest):
      try:
         # Resume blk production
         resumeBlkProduction()
         # Kill and relaunch in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait for some blks to be produced and lib advance)
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, addOrSwapFlags={"--read-mode": "speculative"})
         # Ensure that the relaunched node received blks from producers
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         # Confirm state
         confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest)
      finally:
         # Stop blk production
         stopBlkProduction()

   # 7th test case: Switch mode speculative -> irreversible and compare the state before shutdown
   # Expectation: Node switch mode successfully and head == libBeforeSwitchMode and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
   def switchSpecToIrrModeAndCompareState(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)
      # Kill and relaunch in irreversible mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible")
      # Confirm state
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 8th test case: Switch mode irreversible -> speculative and compare the state before shutdown
   # Expectation: Node switch mode successfully and head == forkDbHeadBeforeSwitchMode == forkDbHead and lib == libBeforeSwitchMode
   def switchIrrToSpecModeAndCompareState(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)
      # Kill and relaunch in speculative mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, addOrSwapFlags={"--read-mode": "speculative"})
      # Confirm state
      confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 9th test case: Replay in irreversible mode with reversible blks while production is enabled
   # Expectation: Node replays and launches successfully and the head and lib should be advancing
   # Current Bug: duplicate blk added error
   def replayInIrrModeWithRevBlksAndProdEnabled(nodeIdOfNodeToTest, nodeToTest):
      try:
         resumeBlkProduction()
         # Kill node and replay in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")
         # Ensure that the relaunched node received blks from producers
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         # Confirm state
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         stopBlkProduction()

   # 10th test case: Replay in irreversible mode without reversible blks while production is enabled
   # Expectation: Node replays and launches successfully and the head and lib should be advancing
   def replayInIrrModeWithoutRevBlksAndProdEnabled(nodeIdOfNodeToTest, nodeToTest):
      try:
         resumeBlkProduction()
         # Kill node and replay in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         # Remove rev blks
         removeReversibleBlks(nodeIdOfNodeToTest)
         waitForBlksProducedAndLibAdvanced() # Wait
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")
         # Ensure that the relaunched node received blks from producers
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         # Confirm state
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         stopBlkProduction()

   # 11th test case: Replay in irreversible mode with reversible blks and compare the state before switch mode
   # Expectation: Node replays and launches successfully and (head == libBeforeShutdown and lib == libBeforeShutdown and forkDbHead == forkDbHeadBeforeShutdown)
   # Current Bug: duplicate blk added error (similar to 1st test case)
   # Once bug in 1st test case is fixed, this can be merged with 1st test case
   def replayInIrrModeWithRevBlksAndCompareState(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)
      # Kill node and replay in irreversible mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")
      # Confirm state
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # Start executing test cases here
   executeTest(1, replayInIrrModeWithRevBlks)
   executeTest(2, replayInIrrModeWithoutRevBlksAndCompareState)
   executeTest(3, switchSpecToIrrMode)
   executeTest(4, switchIrrToSpecMode)
   executeTest(5, switchSpecToIrrModeWithProdEnabled)
   executeTest(6, switchIrrToSpecModeWithProdEnabled)
   executeTest(7, switchSpecToIrrModeAndCompareState)
   executeTest(8, switchIrrToSpecModeAndCompareState)
   executeTest(9, replayInIrrModeWithRevBlksAndProdEnabled)
   executeTest(10, replayInIrrModeWithoutRevBlksAndProdEnabled)
   executeTest(11, replayInIrrModeWithRevBlksAndCompareState)

finally:
   # TestHelper.shutdown(cluster, walletMgr)
   # Print test result
   for msg in testResultMsgs: Print(msg)

exit(0)
