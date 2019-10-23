#!/usr/bin/env python3

import json
import time

import core
import helper
from logger import LogLevel, WriterConfig, ScreenWriter, FileWriter, Logger
from service import Service, Cluster, CommandLineArguments

seed = 0
def create_accounts(cluster, num):
    global seed
    acc_list = []
    for i in range(0, num):
        name = ""
        r = seed
        seed = seed + 1
        for k in range(0, 6):
            name = chr(ord('a') + r % 26) + name
            r = r // 26
        acc_list.append({"name":name})
    cluster.call("create_bios_accounts",creator="eosio",accounts=acc_list,verify_key="")

def currentRunningNodes(cluster):
    ix = cluster.get_cluster_info()
    inforsp = json.loads(ix.response.text)
    node_count = len(inforsp["result"])
    count = 0
    for i in range(0, node_count):
        if "head_block_id" in inforsp["result"][i][1]:
            count = count + 1
    return count

def isInSync(cluster, minNodeNum, assertInSync=False):
    tries = 2
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
            return True,min_block_num,max_block_num,count
        if max_block_num - min_block_num > 2: # can't be in sync...
            break
        tries = tries - 1
        time.sleep(0.05)
    if assertInSync:
        cluster.service.logger.error(ix.response.text, buffer=False)
    return False,min_block_num,max_block_num,count

def waitUntilInsync(cluster, total_nodes, timeout=30, expectInSyc=True):
    tries = timeout
    tries2 = 10 # stop trying eariler if nodes can't start
    ok = False
    count = 0
    while tries > 0 and (count == total_nodes or tries2 > 0):
        res,min_blk,max_blk,count = isInSync(cluster, total_nodes)
        if res:
            ok = True
            break
        tries = tries - 1
        tries2 = tries2 - 1
        time.sleep(1)
    if (not expectInSyc) and (not ok):
        return False
    assert ok, "cluster is not in sync"
    cluster.service.logger.info("cluster is in sync with %d nodes, head_block_num %d" % (total_nodes, min_blk))
    return True

def main():
    buffered_color_config = WriterConfig(buffered=True, monochrome=False, threshold="DEBUG")
    unbuffered_mono_config = WriterConfig(buffered=False, monochrome=True, threshold="TRACE")
    logger = Logger(ScreenWriter(config=buffered_color_config), FileWriter(filename="mono.log", config=unbuffered_mono_config))
    service = Service(logger=logger)

    print_info = lambda msg: logger.info(msg=msg)
    print_info(">>> Min avail RAM test starts.")

    total_nodes = 4
    cluster = Cluster(service=service, total_nodes=total_nodes, total_producers=total_nodes, producer_nodes=total_nodes, dont_bootstrap=True)

    dbsize = 1010
    print_info("set guard size for node 1, node 2 & node 3")
    for i in range(1, total_nodes):
        cluster.stop_node(node_id=i, kill_sig=15)
    time.sleep(1.0)
    for i in range(1, total_nodes):
        cluster.start_node(node_id=i, extra_args="--chain-state-db-guard-size-mb 1002 --chain-state-db-size-mb %d" % (dbsize))

    waitUntilInsync(cluster, total_nodes)

    # let's keep creating accounts until they reach database guard levels
    perAccountRAM = 512 # in bytes
    while currentRunningNodes(cluster) == total_nodes and seed < (8 * 1024 * 1024 // perAccountRAM):
        create_accounts(cluster, 100)

    time.sleep(1.0)
    assert currentRunningNodes(cluster) == 1, "node 1,2,3 should have shut down"

    print_info("%d accounts created before nodes shutdown, trying to examine log files of nodes" % (seed))
    search_str = "database chain::guard_exception"
    for i in range(1, total_nodes):
        log = cluster.get_log(node_id=i)
        if log.find(search_str):
            print_info("node %d's log contains \"%s\"" % (i, search_str))
        else:
            print_error("node %d's should contain \"%s\"" % (i, search_str))
            assert False, "node's log doesn't contains chain::guard_exception"

    dbsize_tries = 10
    while dbsize_tries > 0: # try to increase capacity gradually until all nodes can start
        dbsize += 4
        print_info("try to restart with new capacity %d" % (dbsize))
        for i in range(1, total_nodes):
            cluster.stop_node(node_id=i, kill_sig=15)
        time.sleep(1.0)
        for i in range(1, total_nodes):
            cluster.start_node(node_id=i, extra_args="--chain-state-db-guard-size-mb 1002 --chain-state-db-size-mb %d" % (dbsize))
        if waitUntilInsync(cluster, total_nodes, 30, False):
            break
        print_info("looks like one or more nodes can't start, retrying with more capacity")
        dbsize_tries = dbsize_tries - 1

    while currentRunningNodes(cluster) == total_nodes and seed < (20 * 1024 * 1024 // perAccountRAM):
        create_accounts(cluster, 100)

    assert currentRunningNodes(cluster) < total_nodes, "at least one node should have shut down"
    ok = False
    for i in range(1, total_nodes):
        log = cluster.get_log(node_id=i)
        if log.find(search_str):
            print_info("node %d's log contains \"%s\"" % (i, search_str))
            ok = True
    assert ok, "can not find chain::guard_exception in any node's log"

    dbsize += 100
    print_info("restart with even more capacity(%d) for node 1, node 2 & node 3" % (dbsize))
    for i in range(1, total_nodes):
        cluster.stop_node(node_id=i, kill_sig=15)
    time.sleep(1.0)
    for i in range(1, total_nodes):
        cluster.start_node(node_id=i, extra_args="--chain-state-db-guard-size-mb 1002 --chain-state-db-size-mb %d" % (dbsize))

    print_info("wait for all nodeos in-sync")
    waitUntilInsync(cluster, total_nodes)

    create_accounts(cluster, 100)
    time.sleep(1.0)
    waitUntilInsync(cluster, total_nodes)
    print_info(">>> Min avail RAM test finished successfully.")

if __name__ == '__main__':
    main()
