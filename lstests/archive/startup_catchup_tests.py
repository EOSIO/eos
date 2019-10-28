#!/usr/bin/env python3

import json
import time

import core
import helper
from logger import LogLevel, ScreenWriter, FileWriter, Logger
from service import Service, Cluster, CommandLineArguments

###############################################################
# nodeos_startup_catchup
#  Test configures a producing node and <--txn-plugins count> non-producing nodes with the
#  txn_test_gen_plugin.  Each non-producing node starts generating transactions and sending them
#  to the producing node.
#  1) Wait for at least 10 seconds
#  2) all nodes are in-sync
#  3) one node is shut down gracefully
#  4) restart the node
#  5) all nodes are in-sync
#  6) one node is hard-killed
#  7) restart the node with --delete-all-blocks
#  8) all nodes are in-sync
#  9) repeat 3-8, "catchupCount-1" more times
###############################################################

def get_txn_count(cluster, block_num):
    block_json = json.loads(cluster.get_block(block_num).response.text)
    return len(block_json["transactions"])

def isInSync(cluster, minNodeNum, assertInSync=False):
    tries = 5
    while tries > 0:
        ix = cluster.get_cluster_info()
        inforsp = json.loads(ix.response.text)
        node_count = len(inforsp["result"])
        head_block_id = inforsp["result"][0][1]["head_block_id"]
        insync = True
        count = 0
        min_block_num = 0
        max_block_num = 0
        for i in range(0, node_count):
            if "head_block_id" in inforsp["result"][i][1]:
                count = count + 1
                block_num = inforsp["result"][i][1]["head_block_num"]
                if min_block_num == 0 or block_num < min_block_num:
                    min_block_num = block_num
                if max_block_num == 0 or block_num > max_block_num:
                    max_block_num = block_num
                if head_block_id != inforsp["result"][i][1]["head_block_id"]:
                    insync = False
        if insync and count >= minNodeNum:
            return True,min_block_num,max_block_num
        if max_block_num - min_block_num > 2: # can't be in sync...
            break
        tries = tries - 1
        time.sleep(0.1)
    if assertInSync:
        cluster.service.logger.error(ix.response.text, buffer=False)
    return False,min_block_num,max_block_num

def waitInSync(cluster, timeout, minNodeNum):
    while timeout > 0:
        r,minN,maxN = isInSync(cluster, minNodeNum)
        if r:
            cluster.service.logger.info("blockchain is in-sync, block num %d" % (minN))
            return minN
        timeout = timeout - 1
        time.sleep(1.0)
    cluster.service.logger.error("Block chain is not in-sync, min block num %d, max block num %d" % (minN, maxN))
    assert False, "block chain not insync"


def main():
    logger = Logger(ScreenWriter(threshold="debug"),
                    FileWriter(filename="debug.log", threshold="debug"),
                    FileWriter(filename="trace.log", threshold="trace"),
                    FileWriter(filename="mono.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)

    print_info = lambda msg: logger.info(msg=msg)
    print_info(">>> Startup & catchup test starts.==============")

    total_nodes = 3
    minRequiredTxnsPerBlock = 200
    numBlocks = 30
    txn_gen_node = 1
    catchupCount = 3

    cluster = Cluster(service=service, total_nodes=total_nodes, total_producers=total_nodes * 1, producer_nodes=total_nodes, dont_vote=True, dont_bootstrap=True,extra_configs=["plugin = eosio::txn_test_gen_plugin"])

    print_info("create test accounts on node %d via txn_test_gen_plugin" % (txn_gen_node))

    tries = 5
    while tries > 0:
        ix = cluster.call("send_raw", retry=0, node_id=txn_gen_node, url="/v1/txn_test_gen/create_test_accounts", string_data="[\"eosio\",\"5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\"]")
        if "as that name is already taken" in ix.response_text:
            break
        tries = tries - 1
        time.sleep(1.0)

    blocknum0 = waitInSync(cluster, 30, 3)

    print_info("start generation of transactions on node %d via txn_test_gen_plugin" % (txn_gen_node))
    cluster.call("send_raw", retry=10, node_id=txn_gen_node, url="/v1/txn_test_gen/start_generation", string_data="[\"salt\",5,10]")

    print_info("wait for %d blocks..." % (numBlocks))
    time.sleep(numBlocks / 2)
    tries = 10
    while tries > 0:
        blocknum1 = waitInSync(cluster, 30, 3)
        if blocknum1 >= blocknum0 + numBlocks:
            break
        tries = tries - 1
        time.sleep(1.0)

    print_info("block chain has advanced from %d to %d, now stop generation..." % (blocknum0, blocknum1))
    cluster.call("send_raw", retry=10, node_id=txn_gen_node, url="/v1/txn_test_gen/stop_generation")

    blocknum2 = waitInSync(cluster, 30, 3)

    total_txn = 0
    for i in range(blocknum0 + 1, blocknum1 + 1):
        txn_count = get_txn_count(cluster, i)
        total_txn = total_txn + txn_count
        print_info("block %d has %d transactions" % (i, txn_count))

    print_info("%d transactions in %d blocks" % (total_txn, blocknum1 - blocknum0))
    assert total_txn / (blocknum1 - blocknum0) >= minRequiredTxnsPerBlock, "Total transaction per block %f less than required %d" % (total_txn / (blocknum1 - blocknum0), minRequiredTxnsPerBlock)

    for k in range(0, catchupCount):
        print_info("gracefully shutdown node 2")
        cluster.stop_node(node_id=2, kill_sig=15)
        time.sleep(1.0)

        r,minN,maxN = isInSync(cluster, 3)
        assert r == False, "node should not in-sync after stopping node 2"

        print_info("restarting node 2")
        cluster.start_node(node_id=2)
        blocknum3 = waitInSync(cluster, 30, 3)
        print_info("cluster in-sync after restart, blocknum %d" % (blocknum3))

        assert blocknum3 > blocknum2, "chain doesn't advance"

        print_info("hard-kill node 2")
        cluster.stop_node(node_id=2, kill_sig=9)
        time.sleep(1.0)

        r,minN,maxN = isInSync(cluster, 3)
        assert r == False, "node should not in-sync after stopping node 2"

        print_info("restarting node 2 with --delete-all-blocks")
        cluster.start_node(node_id=2, extra_args="--delete-all-blocks")
        blocknum4 = waitInSync(cluster, 90, 3)
        print_info("cluster in-sync after restart with --delete-all-blocks, blocknum %d" % blocknum4)

        assert blocknum4 > blocknum3, "chain doesn't advance"
        blocknum2 = blocknum4

    print_info(">>> Startup & catchup test finished successfully with %d total transactions." % (total_txn))

if __name__ == '__main__':
    main()
