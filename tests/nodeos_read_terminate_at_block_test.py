#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper
from testUtils import Account

import re
import os
import time
import signal
import subprocess
import shutil

###############################################################
# nodeos_read_terminate_at_block_test
#
# A few tests centered around read mode of irreversible, speculative and head with terminate-at-block set
#
###############################################################

Print = Utils.Print
errorExit = Utils.errorExit
cmdError = Utils.cmdError
relaunchTimeout = 10
numOfProducers = 4
totalNodes = 10

termAtFutureBlockNum = 30

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

# Wait for some time until LIB advance
def waitForBlksProducedAndLibAdvanced():
   requiredConfirmation = int(2 / 3 * numOfProducers) + 1
   maxNumOfBlksReqToConfirmLib = (12 * requiredConfirmation - 1) * 2
   # Give 6 seconds buffer time
   bufferTime = 6
   timeToWait = maxNumOfBlksReqToConfirmLib / 2 + bufferTime
   time.sleep(timeToWait)

# This function is special for "terminate-at-block" argument, if no this argument just use node.relaunch (see Node.py)
def nodeRelaunch(node: Node, chainArg=None, newChain=False, skipGenesis=True, timeout=Utils.systemWaitTimeout, addSwapFlags=None, cachePopen=False, nodeosPath=None):

    assert(node.pid is None)
    assert(node.killed)

    if Utils.Debug: Utils.Print("Launching node process, Id: {}".format(node.nodeId))

    cmdArr=[]
    splittedCmd=node.cmd.split()
    if nodeosPath: splittedCmd[0] = nodeosPath
    myCmd=" ".join(splittedCmd)
    toAddOrSwap=copy.deepcopy(addSwapFlags) if addSwapFlags is not None else {}
    if not newChain:
        skip=False
        swapValue=None
        for i in splittedCmd:
            Utils.Print("\"%s\"" % (i))
            if skip:
                skip=False
                continue
            if skipGenesis and ("--genesis-json" == i or "--genesis-timestamp" == i):
                skip=True
                continue

            if swapValue is None:
                cmdArr.append(i)
            else:
                cmdArr.append(swapValue)
                swapValue=None

            if i in toAddOrSwap:
                swapValue=toAddOrSwap[i]
                del toAddOrSwap[i]
        for k,v in toAddOrSwap.items():
            cmdArr.append(k)
            cmdArr.append(v)
        myCmd=" ".join(cmdArr)

    cmd=myCmd + ("" if chainArg is None else (" " + chainArg))
    node.launchCmd(cmd, cachePopen)

    def isNodeAlive():
        """wait for node to be responsive."""
        try:
            return True if node.checkPulse() else False
        except (TypeError) as _:
            pass
        return False

    isAlive=Utils.waitForTruth(isNodeAlive, timeout, sleepTime=1)
    if isAlive:
        Utils.Print("Node relaunch was successful.")
    else:
        Utils.Print("ERROR: Node relaunch Failed.")
        # Ensure the node process is really killed
        if node.popenProc:
            node.popenProc.send_signal(signal.SIGTERM)
            node.popenProc.wait()
        node.pid=None
        return False

    node.cmd=cmd
    node.killed=False
    return True

def relaunchNode(node: Node, chainArg="", addSwapFlags=None, relaunchAssertMessage="Fail to relaunch"):
   isRelaunchSuccess = node.relaunch(chainArg=chainArg, addSwapFlags=addSwapFlags, timeout=relaunchTimeout, cachePopen=True)
   time.sleep(1) # Give a second to replay or resync if needed
   assert isRelaunchSuccess, relaunchAssertMessage
   return isRelaunchSuccess

def relaunchTestNode(node: Node, chainArg="", addSwapFlags=None, relaunchAssertMessage="Fail to relaunch"):
   isRelaunchSuccess = nodeRelaunch(node=node, chainArg=chainArg, addSwapFlags=addSwapFlags, timeout=relaunchTimeout, cachePopen=True)
   time.sleep(1) # Give a second to replay or resync if needed
   assert isRelaunchSuccess, relaunchAssertMessage
   return isRelaunchSuccess

# List to contain the test result message
testResultMsgs = []
testSuccessful = True
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
         relaunchNode(producingNode)

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
      resultDesc = None
      try:
         # Relaunch killed node so it can be used for the test
         nodeToTest = cluster.getNode(nodeIdOfNodeToTest)
         Utils.Print("Re-launch node #{} to excute test scenario: {}".format(nodeIdOfNodeToTest, runTestScenario.__name__))
         relaunchNode(nodeToTest, relaunchAssertMessage="Fail to relaunch before running test scenario")

         # Run test scenario
         runTestScenario(nodeIdOfNodeToTest, nodeToTest)
         resultDesc = "!!!TEST CASE #{} ({}) IS SUCCESSFUL".format(nodeIdOfNodeToTest, runTestScenario.__name__)
         testResult = True
      except Exception as e:
         resultDesc = "!!!BUG IS CONFIRMED ON TEST CASE #{} ({}): {}".format(nodeIdOfNodeToTest, runTestScenario.__name__, e)
      finally:
         Utils.Print(resultDesc)
         testResultMsgs.append(resultDesc)
         # Kill node after use
         if not nodeToTest.killed: nodeToTest.kill(signal.SIGTERM)
      return testResult


   # 1th test case: Replay in irreversible mode with reversible blks while connected to producing node
   # Expectation: Node replays and launches successfully
   #              and the head and lib should be no less than termAtBlockNum
   def replayInIrrModeWithRevBlksAndConnectedToProdNodeTermAtBlock(nodeIdOfNodeToTest, nodeToTest):
      try:
         startProdNode()
         # Kill node and replay in irreversible mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait
         infoprod = producingNode.getInfo()
         libprod = int(infoprod["last_irreversible_block_num"])
         termAtBlockNum = libprod + termAtFutureBlockNum
        
         relaunchTestNode(nodeToTest, chainArg=" --read-mode irreversible  --terminate-at-block=" + str(termAtBlockNum))
         Utils.Print("Test node begin recevie from producing node and is expected to stop at or little bigger than block number " + str(termAtBlockNum) )

         while True :
             info = nodeToTest.getInfo()
             if info is not None:
                 head = int(info["head_block_num"])
                 lib = int(info["last_irreversible_block_num"])
                 forkDbHead =  int(info["fork_db_head_block_num"])
                 Utils.Print("head=" + str(head)  + ", lib=" + str(lib) + ", terminate-at-block=", str(termAtBlockNum))
                 Utils.Print(info)
             else:
                 break
         assert head == lib, "Head ({}) should be equal to lib ({})".format(head, lib)
         assert head >= termAtBlockNum, "Head ({}) should no less than termAtBlockNum ({})".format(head, termAtBlockNum)
         assert forkDbHead >= head, "Fork db head ({}) should be larger or equal to the head ({})".format(forkDbHead, head)
      finally:
         stopProdNode()
         Utils.Print("End replayInIrrModeWithRevBlksAndConnectedToProdNodeTermAtBlock")

   # 2th test case: Replay in speculative mode with reversible blks while connected to producing node
   # Expectation: Node replays and launches successfully
   #              and the head and lib should be no less than termAtBlockNum
   def replayInSpecModeWithRevBlksAndConnectedToProdNodeTermAtBlock(nodeIdOfNodeToTest, nodeToTest):
      try:
         startProdNode()
         # Kill node and replay in speculative  mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait
         infoprod = producingNode.getInfo()
         headprod = int(infoprod["head_block_num"])
         termAtBlockNum = headprod + termAtFutureBlockNum
        
         relaunchTestNode(nodeToTest, chainArg=" --read-mode speculative  --terminate-at-block=" + str(termAtBlockNum))
         Utils.Print("Test node begin recevie from producing node and is expected to stop at or little bigger than block number " + str(termAtBlockNum) )

         while True :
             info = nodeToTest.getInfo()
             if info is not None:
                 head = int(info["head_block_num"])
                 lib = int(info["last_irreversible_block_num"])
                 forkDbHead =  int(info["fork_db_head_block_num"])
                 Utils.Print("head=" + str(head)  + ", lib=" + str(lib) + ", terminate-at-block=", str(termAtBlockNum))
                 Utils.Print(info)
             else:
                 break
         assert head >= termAtBlockNum, "Head ({}) should be no less than termAtBlockNum ({})".format(head, termAtBlockNum)
         assert forkDbHead >= head, "Fork db head ({}) should be larger or equal to the head ({})".format(forkDbHead, head)
      finally:
         stopProdNode()
         Utils.Print("End replayInSpecModeWithRevBlksAndConnectedToProdNodeTermAtBlock")

   # 3th test case: Replay in head mode with reversible blks while connected to producing node
   # Expectation: Node replays and launches successfully
   #              and the head and lib should be no less than termAtBlockNum
   def replayInHeadModeWithRevBlksAndConnectedToProdNodeTermAtBlock(nodeIdOfNodeToTest, nodeToTest):
      try:
         startProdNode()
         # Kill node and replay in head mode
         nodeToTest.kill(signal.SIGTERM)
         waitForBlksProducedAndLibAdvanced() # Wait
         infoprod = producingNode.getInfo()
         headprod = int(infoprod["head_block_num"])
         termAtBlockNum = headprod + termAtFutureBlockNum
        
         relaunchTestNode(nodeToTest, chainArg=" --read-mode head --terminate-at-block=" + str(termAtBlockNum))
         Utils.Print("Test node begin recevie from producing node and is expected to stop at or little bigger than block number " + str(termAtBlockNum) )

         while True :
             info = nodeToTest.getInfo()
             if info is not None:
                 head = int(info["head_block_num"])
                 lib = int(info["last_irreversible_block_num"])
                 forkDbHead =  int(info["fork_db_head_block_num"])
                 Utils.Print("head=" + str(head)  + ", lib=" + str(lib) + ", terminate-at-block=", str(termAtBlockNum))
                 Utils.Print(info)
             else:
                 break
         assert head >= termAtBlockNum, "Head ({}) should be no less than termAtBlockNum ({})".format(head, termAtBlockNum)
         assert forkDbHead >= head, "Fork db head ({}) should be larger or equal to the head ({})".format(forkDbHead, head)
      finally:
         stopProdNode()
         Utils.Print("End replayInHeadModeWithRevBlksAndConnectedToProdNodeTermAtBlock")


   # Start executing test cases here
   Utils.Print("Script Begin .............................")
   testSuccessful = testSuccessful and executeTest(1, replayInIrrModeWithRevBlksAndConnectedToProdNodeTermAtBlock)
   testSuccessful = testSuccessful and executeTest(2, replayInSpecModeWithRevBlksAndConnectedToProdNodeTermAtBlock)
   testSuccessful = testSuccessful and executeTest(3, replayInHeadModeWithRevBlksAndConnectedToProdNodeTermAtBlock)
   Utils.Print("Script End ................................")


finally:
   TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)
   # Print test result
   for msg in testResultMsgs:
      Print(msg)
   if not testSuccessful and len(testResultMsgs) < 9:
      Print("Subsequent tests were not run after failing test scenario.")

exitCode = 0 if testSuccessful else 1
exit(exitCode)
