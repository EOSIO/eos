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
# generic_message_test
#  Test uses test_generic_message_plugin to verify that generic_messages can be
#  sent between instances of nodeos
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
pnodes=5
cluster=Cluster(walletd=True)
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontKill=args.leave_running
prodCount=1
killAll=args.clean_run
walletPort=TestHelper.DEFAULT_WALLET_PORT
totalNodes=pnodes

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
    specificExtraNodeosArgs={}
#    specificExtraNodeosArgs[0] = "--plugin eosio::test_generic_message::test_generic_message_api_plugin --register-generic-type type1 --register-generic-type type2 "
    specificExtraNodeosArgs[0] = "--plugin eosio::test_generic_message::test_generic_message_api_plugin "
#    specificExtraNodeosArgs[1] = "--plugin eosio::test_generic_message::test_generic_message_api_plugin --register-generic-type type2 --register-generic-type type3 "
    specificExtraNodeosArgs[1] = "--plugin eosio::test_generic_message::test_generic_message_api_plugin "
#    specificExtraNodeosArgs[2] = "--plugin eosio::test_generic_message::test_generic_message_api_plugin --register-generic-type type3 --register-generic-type type1 "
    specificExtraNodeosArgs[2] = "--plugin eosio::test_generic_message::test_generic_message_api_plugin "
#    specificExtraNodeosArgs[3] = "--plugin eosio::test_generic_message::test_generic_message_api_plugin --register-generic-type type1 --register-generic-type type2 --register-generic-type type3 "
    specificExtraNodeosArgs[3] = "--plugin eosio::test_generic_message::test_generic_message_api_plugin "
#    specificExtraNodeosArgs[3]="--plugin eosio::test_generic_message::test_generic_message_api_plugin --plugin eosio::test_generic_message::test_generic_message_plugin "

    Print("Stand up cluster")
    if cluster.launch(prodCount=prodCount, onlyBios=False, pnodes=pnodes, totalNodes=totalNodes, totalProducers=pnodes*prodCount,
                      specificExtraNodeosArgs=specificExtraNodeosArgs) is False:
        Utils.errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    # biosNode=cluster.biosNode
    # node0=cluster.getNode(0)
    # node1=cluster.getNode(1)
    # node2=cluster.getNode(2)
    # node3=cluster.getNode(3)
    # node4=cluster.getNode(4)

    testSuccessful=True

finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killWallet, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exit(0)
