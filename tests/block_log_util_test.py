#!/usr/bin/env python3

from testUtils import Utils
from testUtils import BlockLogAction
import time
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import BlockType
import os
import signal
import subprocess
from TestHelper import AppArgs
from TestHelper import TestHelper

###############################################################
# block_log_util_test
#  Test verifies that the blockLogUtil is still compatible with nodeos
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit

from core_symbol import CORE_SYMBOL

def verifyBlockLog(expected_block_num, trimmedBlockLog):
    firstBlockNum = expected_block_num
    for block in trimmedBlockLog:
        assert 'block_num' in block, print("ERROR: eosio-blocklog didn't return block output")
        block_num = block['block_num']
        assert block_num == expected_block_num
        expected_block_num += 1
    Print("Block_log contiguous from block number %d to %d" % (firstBlockNum, expected_block_num - 1))


appArgs=AppArgs()
args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"})
Utils.Debug=args.v
pnodes=2
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
prodCount=2
killAll=args.clean_run
walletPort=TestHelper.DEFAULT_WALLET_PORT
totalNodes=pnodes+1

walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill

WalletdName=Utils.EosWalletName
ClientName="cleos"

try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.setWalletMgr(walletMgr)

    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    Print("Stand up cluster")
    if cluster.launch(prodCount=prodCount, onlyBios=False, pnodes=pnodes, totalNodes=totalNodes, totalProducers=pnodes*prodCount, useBiosBootFile=False) is False:
        Utils.errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    biosNode=cluster.biosNode
    node0=cluster.getNode(0)
    node1=cluster.getNode(1)

    blockNum=100
    Print("Wait till we at least get to block %d" % (blockNum))
    node0.waitForBlock(blockNum, blockType=BlockType.lib)
    info=node0.getInfo(exitOnError=True)
    headBlockNum=info["head_block_num"]
    lib=info["last_irreversible_block_num"]

    Print("Kill the node we want to verify its block log")
    node0.kill(signal.SIGTERM)

    Print("Wait for node0's head block to become irreversible")
    node1.waitForBlock(headBlockNum, blockType=BlockType.lib)
    infoAfter=node1.getInfo(exitOnError=True)
    headBlockNumAfter=infoAfter["head_block_num"]

    def checkBlockLog(blockLog, blockNumsToFind, firstBlockNum=1):
        foundBlockNums=[]
        nextBlockNum=firstBlockNum
        previous=0
        nextIndex=0
        for block in blockLog:
            blockNum=block["block_num"]
            if nextBlockNum!=blockNum:
                Utils.errorExit("BlockLog should progress to the next block number, expected block number %d but got %d" % (nextBlockNum, blockNum))
            if nextIndex<len(blockNumsToFind) and blockNum==blockNumsToFind[nextIndex]:
                foundBlockNums.append(True)
                nextIndex+=1
            nextBlockNum+=1
        while nextIndex<len(blockNumsToFind):
            foundBlockNums.append(False)
            if nextIndex<len(blockNumsToFind)-1:
                assert blockNumsToFind[nextIndex+1] > blockNumsToFind[nextIndex], "expects passed in array, blockNumsToFind to increase from smallest to largest, %d is less than or equal to %d" % (next, previous)
            nextIndex+=1

        return foundBlockNums

    Print("Retrieve the whole blocklog for node 0")
    blockLog=cluster.getBlockLog(0)
    foundBlockNums=checkBlockLog(blockLog, [headBlockNum, headBlockNumAfter])
    assert foundBlockNums[0], "Couldn't find \"%d\" in blocklog:\n\"%s\"\n" % (foundBlockNums[0], blockLog)
    assert not foundBlockNums[1], "Should not find \"%d\" in blocklog:\n\"%s\"\n" % (foundBlockNums[1], blockLog)

    output=cluster.getBlockLog(0, blockLogAction=BlockLogAction.smoke_test)
    expectedStr="no problems found"
    assert output.find(expectedStr) != -1, "Couldn't find \"%s\" in:\n\"%s\"\n" % (expectedStr, output)

    blockLogDir=Utils.getNodeDataDir(0, "blocks")
    duplicateIndexFileName=os.path.join(blockLogDir, "duplicate.index")
    output=cluster.getBlockLog(0, blockLogAction=BlockLogAction.make_index, outputFile=duplicateIndexFileName)
    assert output is not None, "Couldn't make new index file \"%s\"\n" % (duplicateIndexFileName)

    blockIndexFileName=os.path.join(blockLogDir, "blocks.index")
    blockIndexFile=open(blockIndexFileName,"rb")
    duplicateIndexFile=open(duplicateIndexFileName,"rb")
    blockIndexStr=blockIndexFile.read()
    duplicateIndexStr=duplicateIndexFile.read()
    assert blockIndexStr==duplicateIndexStr, "Generated file \%%s\" didn't match original \"%s\"" % (duplicateIndexFileName, blockIndexFileName)

    try:
        Print("Head block num %d will not be in block log (it will be in reversible DB), so --trim will throw an exception" % (headBlockNum))
        output=cluster.getBlockLog(0, blockLogAction=BlockLogAction.trim, last=headBlockNum, throwException=True)
        Utils.errorExit("BlockLogUtil --trim should have indicated error for last value set to lib (%d) " +
                        "which should not do anything since only trimming blocklog and not irreversible blocks" % (lib))
    except subprocess.CalledProcessError as ex:
        pass

    beforeEndOfBlockLog=lib-20
    Print("Block num %d will definitely be at least one block behind the most recent entry in block log, so --trim will work" % (beforeEndOfBlockLog))
    output=cluster.getBlockLog(0, blockLogAction=BlockLogAction.trim, last=beforeEndOfBlockLog, throwException=True)

    Print("Kill the non production node, we want to verify its block log")
    cluster.getNode(2).kill(signal.SIGTERM)

    Print("Trim off block num 1 to remove genesis block from block log.")
    output=cluster.getBlockLog(2, blockLogAction=BlockLogAction.trim, first=2, throwException=True)

    Print("Smoke test the trimmed block log.")
    output=cluster.getBlockLog(2, blockLogAction=BlockLogAction.smoke_test)

    Print("Analyze block log.")
    trimmedBlockLog=cluster.getBlockLog(2, blockLogAction=BlockLogAction.return_blocks)

    verifyBlockLog(2, trimmedBlockLog)

    # relaunch the node with the truncated block log and ensure it catches back up with the producers
    current_head_block_num = node1.getInfo()["head_block_num"]
    cluster.getNode(2).relaunch(cachePopen=True)
    assert cluster.getNode(2).waitForBlock(current_head_block_num, timeout=60, reportInterval=15)

    # ensure it continues to advance
    current_head_block_num = node1.getInfo()["head_block_num"]
    assert cluster.getNode(2).waitForBlock(current_head_block_num, timeout=60, reportInterval=15)
    info = cluster.getNode(2).getInfo()
    block = cluster.getNode(2).getBlock(2)
    assert block is not None
    block = cluster.getNode(2).getBlock(1, silentErrors=True)
    assert block is None

    # verify it shuts down cleanly
    cluster.getNode(2).interruptAndVerifyExitStatus()

    firstBlock = info["last_irreversible_block_num"]
    Print("Trim off block num %s." % (firstBlock))
    output=cluster.getBlockLog(2, blockLogAction=BlockLogAction.trim, first=firstBlock, throwException=True)

    Print("Smoke test the trimmed block log.")
    output=cluster.getBlockLog(2, blockLogAction=BlockLogAction.smoke_test)

    Print("Analyze block log.")
    trimmedBlockLog=cluster.getBlockLog(2, blockLogAction=BlockLogAction.return_blocks)

    verifyBlockLog(firstBlock, trimmedBlockLog)

    # relaunch the node with the truncated block log and ensure it catches back up with the producers
    current_head_block_num = node1.getInfo()["head_block_num"]
    assert current_head_block_num >= info["head_block_num"]
    cluster.getNode(2).relaunch(cachePopen=True)
    assert cluster.getNode(2).waitForBlock(current_head_block_num, timeout=60, reportInterval=15)

    # ensure it continues to advance
    current_head_block_num = node1.getInfo()["head_block_num"]
    assert cluster.getNode(2).waitForBlock(current_head_block_num, timeout=60, reportInterval=15)
    info = cluster.getNode(2).getInfo()
    block = cluster.getNode(2).getBlock(firstBlock)
    assert block is not None
    block = cluster.getNode(2).getBlock(firstBlock - 1, silentErrors=True)
    assert block is None
    block = cluster.getNode(2).getBlock(1, silentErrors=True)
    assert block is None

    # verify it shuts down cleanly
    cluster.getNode(2).interruptAndVerifyExitStatus()

    # test for blocks.log
    blockLogFileName=os.path.join(blockLogDir, "blocks.log")
    blockIndexFileSize = os.path.getsize(blockIndexFileName)
    blockLogFileSize = os.path.getsize(blockLogFileName)

    blockLogFile = open(blockLogFileName, 'a')
    # truncate blocks.log by 1 byte
    blockLogFile.truncate(blockLogFileSize - 1)
    blockLogFile.close()
    truncatedBlockLogFileSize = os.path.getsize(blockLogFileName)
    assert truncatedBlockLogFileSize == blockLogFileSize - 1, "blocks.log not truncated properly\n"

    # fix_irreversible_blocks
    output=cluster.getBlockLog(0, blockLogAction=BlockLogAction.fix_irreversible_blocks)

    output=cluster.getBlockLog(0, blockLogAction=BlockLogAction.smoke_test)
    expectedStr="no problems found"
    assert output.find(expectedStr) != -1, "Couldn't find \"%s\" in:\n\"%s\"\n" % (expectedStr, output)

    testSuccessful=True

finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exit(0)
