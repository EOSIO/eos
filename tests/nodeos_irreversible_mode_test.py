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
# - forkDbHead >= head == lib
# headLibAndForkDbHeadBeforeSwitchMode should be only passed IF production is disabled, otherwise it provides erroneous check
# When comparing with the the state before node is switched:
# - head == libBeforeSwitchMode == lib and forkDbHead == headBeforeSwitchMode == forkDbHeadBeforeSwitchMode
def confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode=None, isReversibleBlocksDeleted=False):
   head, lib, forkDbHead = getHeadLibAndForkDbHead(nodeToTest)
   assert head == lib, "Head ({}) should be equal to lib ({})".format(head, lib)
   assert forkDbHead >= head, "Fork db head ({}) should be larger or equal to the head ({})".format(forkDbHead, head)

   if headLibAndForkDbHeadBeforeSwitchMode:
      headBeforeSwitchMode, libBeforeSwitchMode, forkDbHeadBeforeSwitchMode = headLibAndForkDbHeadBeforeSwitchMode
      assert head == libBeforeSwitchMode, "Head ({}) should be equal to lib before switch mode ({})".format(head, libBeforeSwitchMode)
      assert lib == libBeforeSwitchMode, "Lib ({}) should be equal to lib before switch mode ({})".format(lib, libBeforeSwitchMode)
      # Different case when reversible blocks are deleted
      if not isReversibleBlocksDeleted:
         assert forkDbHead == headBeforeSwitchMode and forkDbHead == forkDbHeadBeforeSwitchMode, \
            "Fork db head ({}) should be equal to head before switch mode ({}) and fork db head before switch mode ({})".format(forkDbHead, headBeforeSwitchMode, forkDbHeadBeforeSwitchMode)
      else:
         assert forkDbHead == libBeforeSwitchMode, "Fork db head ({}) should be equal to lib before switch mode ({}) when there's no reversible blocks".format(forkDbHead, libBeforeSwitchMode)

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

         # Kill node after use
         if not nodeToTest.killed: nodeToTest.kill(signal.SIGTERM)
         testResultMsgs.append("!!!TEST CASE #{} ({}) IS SUCCESSFUL".format(nodeIdOfNodeToTest, runTestScenario.__name__))
         testResult = True
      except Exception as e:
         testResultMsgs.append("!!!BUG IS CONFIRMED ON TEST CASE #{} ({}): {}".format(nodeIdOfNodeToTest, runTestScenario.__name__, e))
      return testResult

   # 1st test case: Replay in irreversible mode with reversible blks
   # Expectation: Node replays and launches successfully and forkdb head, head, and lib matches the irreversible mode expectation
   # Current Bug: duplicate blk added error
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
   # Current Bug: lib != libBeforeSwitchMode
   def replayInIrrModeWithoutRevBlks(nodeIdOfNodeToTest, nodeToTest):
      # Track head blk num and lib before shutdown
      headLibAndForkDbHeadBeforeSwitchMode = getHeadLibAndForkDbHead(nodeToTest)

      # Shut down node, remove reversible blks and relaunch
      nodeToTest.kill(signal.SIGTERM)
      removeReversibleBlks(nodeIdOfNodeToTest)
      relaunchNode(nodeToTest, nodeIdOfNodeToTest, chainArg=" --read-mode irreversible --replay")

      # Ensure the node condition is as expected after relaunch
      confirmHeadLibAndForkDbHeadOfIrrMode(nodeToTest, headLibAndForkDbHeadBeforeSwitchMode, True)

   # 3rd test case: Switch mode speculative -> irreversible without replay
   # Expectation: Node switches mode successfully and forkdb head, head, and lib matches the irreversible mode expectation
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
   # Expectation: Node switches mode successfully and forkdb head, head, and lib matches the speculative mode expectation
   # Current Bug: head != forkDbHead and head != forkDbHeadBeforeSwitchMode and lib != libBeforeSwitchMode
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
   # Current Bug: Fail to switch to irreversible mode, blk_validate_exception next blk in the future will be thrown
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
   # Current Bug: Node switches mode successfully, however, it fails to establish connection with the producing node
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
   # Current Bug: duplicate blk added error
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
   # Current Bug: Nothing
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


   # Start executing test cases here
   testResult1 = executeTest(1, replayInIrrModeWithRevBlks)
   testResult2 = executeTest(2, replayInIrrModeWithoutRevBlks)
   testResult3 = executeTest(3, switchSpecToIrrMode)
   testResult4 = executeTest(4, switchIrrToSpecMode)
   testResult5 = executeTest(5, switchSpecToIrrModeWithConnectedToProdNode)
   testResult6 = executeTest(6, switchIrrToSpecModeWithConnectedToProdNode)
   testResult7 = executeTest(7, replayInIrrModeWithRevBlksAndConnectedToProdNode)
   testResult8 = executeTest(8, replayInIrrModeWithoutRevBlksAndConnectedToProdNode)

   testSuccessful = testResult1 and testResult2 and testResult3 and testResult4 and testResult5 and testResult6 and testResult7 and testResult8
finally:
   TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)
   # Print test result
   for msg in testResultMsgs: Print(msg)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
