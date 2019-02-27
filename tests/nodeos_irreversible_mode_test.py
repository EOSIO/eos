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
numOfProducers = 4
totalNodes = 9

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

def removeReversibleBlks(nodeId):
   dataDir = Cluster.getDataDir(nodeId)
   reversibleBlks = os.path.join(dataDir, "blocks", "reversible")
   shutil.rmtree(reversibleBlks, ignore_errors=True)

def getHeadLibAndForkDbHead(node: Node):
   info = node.getInfo()
   assert info is not None, "Fail to retrieve info from the node, the node is currently having a problem"
   head = int(info["head_block_num"])
   lib = int(info["last_irreversible_block_num"])
   forkDbHead =  int(info["fork_db_head_block_num"])
   return head, lib, forkDbHead

# Around 30 seconds should be enough to advance lib for 4 producers
def waitForBlksProducedAndLibAdvanced():
   time.sleep(30)

# Ensure that the relaunched node received blks from producers, in other words head and lib is advancing
def ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest):
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   waitForBlksProducedAndLibAdvanced()
   headAfterWaiting, libAfterWaiting, forkDbHeadAfterWaiting = getHeadLibAndForkDbHead(nodeToTest)
   assert headAfterWaiting > head and libAfterWaiting > lib and forkDbHeadAfterWaiting > forkDbHead, \
      "Either Head ({} -> {})/ Lib ({} -> {})/ Fork Db Head ({} -> {}) is not advancing".format(head, headAfterWaiting, lib, libAfterWaiting, forkDbHead, forkDbHeadAfterWaiting)

# Confirm the head lib and fork db of irreversible mode
# Under any condition of irreversible mode:
# - forkDbHead > head == lib
# headLibAndForkDbHeadBeforeSwitchMode should be only passed IF production is disabled, otherwise it provides erroneous check
# When comparing with the the state before node is switched:
# - head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
def confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode=None):
   # In irreversible mode, head should be equal to lib and not equal to fork Db blk num
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   assert head == lib, "Head ({}) should be equal to lib ({})".format(head, lib)
   assert forkDbHead > head, "Fork db head ({}) should be larger than the head ({})".format(forkDbHead, head)

   if headLibAndForkDbHeadBeforeSwitchMode:
      headBeforeSwitchMode, libBeforeSwitchMode, forkDbHeadBeforeSwitchMode = headLibAndForkDbHeadBeforeSwitchMode
      assert head == libBeforeSwitchMode, "Head ({}) should be equal to lib before switch mode ({})".format(head, libBeforeSwitchMode)
      assert lib == libBeforeSwitchMode, "Lib ({}) should be equal to lib before switch mode ({})".format(lib, libBeforeSwitchMode)
      assert forkDbHead == headBeforeSwitchMode and forkDbHead == forkDbHeadBeforeSwitchMode, \
         "Fork db head ({}) should be equal to head before switch mode ({}) and fork db head before switch mode ({})".format(forkDbHead, headBeforeSwitchMode, forkDbHeadBeforeSwitchMode)

# Confirm the head lib and fork db of speculative mode
# Under any condition of irreversible mode:
# - forkDbHead == head > lib
# headLibAndForkDbHeadBeforeSwitchMode should be only passed IF production is disabled, otherwise it provides erroneous check
# When comparing with the the state before node is switched:
# - head == forkDbHeadBeforeSwitchMode == forkDbHead and lib == headBeforeSwitchMode == libBeforeSwitchMode
def confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode=None):
   # In speculative mode, head should be equal to lib and not equal to fork Db blk num
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   assert head > lib, "Head should be larger than lib (head: {}, lib: {})".format(head, lib)
   assert head == forkDbHead, "Head ({}) should be equal to fork db head ({})".format(head, forkDbHead)

   if headLibAndForkDbHeadBeforeSwitchMode:
      headBeforeSwitchMode, libBeforeSwitchMode, forkDbHeadBeforeSwitchMode = headLibAndForkDbHeadBeforeSwitchMode
      assert head == forkDbHeadBeforeSwitchMode, "Head ({}) should be equal to fork db head before switch mode ({})".format(head, forkDbHeadBeforeSwitchMode)
      assert lib == headBeforeSwitchMode and lib == libBeforeSwitchMode, \
         "Lib ({}) should be equal to head before switch mode ({}) and lib before switch mode ({})".format(lib, headBeforeSwitchMode, libBeforeSwitchMode)
      assert forkDbHead == forkDbHeadBeforeSwitchMode, \
         "Fork db head ({}) should be equal to fork db head before switch mode ({}) ".format(forkDbHead, forkDbHeadBeforeSwitchMode)

def relaunchNode(node: Node, nodeId, chainArg="", addOrSwapFlags=None, relaunchAssertMessage="Fail to relaunch"):
   isRelaunchSuccess = node.relaunch(nodeId, chainArg=chainArg, addOrSwapFlags=addOrSwapFlags, timeout=relaunchTimeout)
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
         6:"--read-mode irreversible"})

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
         # Relaunch killed node so it can be used for the test
         nodeToTest = cluster.getNode(nodeIdOfNodeToTest)
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
   #              with head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
   # Current Bug: duplicate blk added error
   def replayInIrrModeWithRevBlks(nodeIdOfNodeToTest, nodeToTest):
      # Kill node and replay in irreversible mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")

      # Confirm state
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)

   # 2nd test case: Replay in irreversible mode without reversible blks
   # Expectation: Node replays and launches successfully
   #              with head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
   # Current Bug: lib != libBeforeSwitchMode
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
   # Expectation: Node switches mode successfully
   #              with head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
   # Current Bug: head != lib
   def switchSpecToIrrMode(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)

      # Kill and relaunch in irreversible mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible")

      # Ensure the node condition is as expected after relaunch
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 4th test case: Switch mode irreversible -> speculative without replay
   # Expectation: Node switches mode successfully
   #              with head == forkDbHeadBeforeSwitchMode == forkDbHead and lib == headBeforeSwitchMode == libBeforeSwitchMode
   # Current Bug: head != forkDbHead and head != forkDbHeadBeforeSwitchMode and lib != libBeforeSwitchMode
   def switchIrrToSpecMode(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)

      # Kill and relaunch in speculative mode
      nodeToTest.kill(signal.SIGTERM)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, addOrSwapFlags={"--read-mode": "speculative"})

      # Ensure the node condition is as expected after relaunch
      confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode)

   # 5th test case: Switch mode irreversible -> speculative without replay and production enabled
   # Expectation: Node switches mode successfully
   #              and the head and lib should be advancing after some blocks produced
   #              with head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
   # Current Bug: Fail to switch to irreversible mode, blk_validate_exception next blk in the future will be thrown
   def switchSpecToIrrModeWithProdEnabled(nodeIdOfNodeToTest, nodeToTest):
      try:
         resumeBlkProduction()

         # Kill and relaunch in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait for some blks to be produced and lib advance
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible")

         # Ensure the node condition is as expected after relaunch
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         stopBlkProduction()

   # 6th test case: Switch mode irreversible -> speculative without replay and production enabled
   # Expectation: Node switches mode successfully
   #              and the head and lib should be advancing after some blocks produced
   #              with head == forkDbHeadBeforeSwitchMode == forkDbHead and lib == headBeforeSwitchMode == libBeforeSwitchMode
   # Current Bug: Node switches mode successfully, however, it fails to establish connection with the producing node
   def switchIrrToSpecModeWithProdEnabled(nodeIdOfNodeToTest, nodeToTest):
      try:
         resumeBlkProduction()

         # Kill and relaunch in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait for some blks to be produced and lib advance)
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, addOrSwapFlags={"--read-mode": "speculative"})

         # Ensure the node condition is as expected after relaunch
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         confirmHeadLibAndForkDbHeadOfSpecMode(nodeToTest)
      finally:
         stopBlkProduction()

   # 7th test case: Replay in irreversible mode with reversible blks while production is enabled
   # Expectation: Node replays and launches successfully
   #              and the head and lib should be advancing after some blocks produced
   #              with head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
   # Current Bug: duplicate blk added error
   def replayInIrrModeWithRevBlksAndProdEnabled(nodeIdOfNodeToTest, nodeToTest):
      try:
         resumeBlkProduction()
         # Kill node and replay in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")

         # Ensure the node condition is as expected after relaunch
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         stopBlkProduction()

   # 8th test case: Replay in irreversible mode without reversible blks while production is enabled
   # Expectation: Node replays and launches successfully
   #              and the head and lib should be advancing after some blocks produced
   #              with head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
   # Current Bug: Nothing
   def replayInIrrModeWithoutRevBlksAndProdEnabled(nodeIdOfNodeToTest, nodeToTest):
      try:
         resumeBlkProduction()

         # Kill node, remove rev blks and then replay in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         removeReversibleBlks(nodeIdOfNodeToTest)
         waitForBlksProducedAndLibAdvanced() # Wait
         relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")

         # Ensure the node condition is as expected after relaunch
         ensureHeadLibAndForkDbHeadIsAdvancing(nodeToTest)
         confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest)
      finally:
         stopBlkProduction()


   # Start executing test cases here
   executeTest(1, replayInIrrModeWithRevBlks)
   executeTest(2, replayInIrrModeWithoutRevBlks)
   executeTest(3, switchSpecToIrrMode)
   executeTest(4, switchIrrToSpecMode)
   executeTest(5, switchSpecToIrrModeWithProdEnabled)
   executeTest(6, switchIrrToSpecModeWithProdEnabled)
   executeTest(7, replayInIrrModeWithRevBlksAndProdEnabled)
   executeTest(8, replayInIrrModeWithoutRevBlksAndProdEnabled)

   testSuccessful = True
finally:
   TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)
   # Print test result
   for msg in testResultMsgs: Print(msg)

exit(0)
