#!/usr/bin/env python3

import time

from core.logger import ScreenWriter, FileWriter, Logger
from core.service import Service, Cluster, BlockchainError, SyncError
from random import randint

def init_cluster():
    test = "fork"
    logger = Logger(ScreenWriter(threshold="info"),
                    FileWriter(filename=f"{test}-info.log", threshold="info", monochrome=True),
                    FileWriter(filename=f"{test}-debug.log", threshold="debug", monochrome=True),
                    FileWriter(filename=f"{test}-trace.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service, node_count=3, pnode_count=3, producer_count=7,
                      topology="bridge", center_node_id=1, dont_setprod=True)
    return cluster


def set_and_verify(clus):
    prod = clus.node_to_producers[0] + clus.node_to_producers[2]
    clus.set_producers(prod)
    clus.check_sync()
    clus.info("Verify schedule...")
    clus.check_production_round(expected_producers=prod)
    return clus.check_sync().block_num


def assert_out_of_sync(clus, res):
    if res.in_sync:
        raise SyncError("Nodes should be out of sync")
    else:
        clus.debug(f"Nodes are out of sync ({res.sync_count}/3 nodes in sync)")


def kill_and_verify(clus):
    wait = randint(0, 7) # randomize fork point
    clus.info("sleep for %d secs then kill bridge node" % (wait))
    time.sleep(wait)
    clus.stop_node(node_id=1, kill_sig=15)
    time.sleep(1)
    timeout = 15 # wait until bridge node fully shutdown
    while timeout > 0:
        res = clus.check_sync(min_sync_count=2, max_block_lag=2, dont_raise=True)
        if not res.in_sync:
            break
        timeout -= 1
        time.sleep(1)
    assert_out_of_sync(clus, res)
    min1, max1 = res.min_block_num, res.max_block_num
    wait = randint(1, 45) # randomize fork difference
    clus.info("Wait for %d secs and until 2 forks have different lengths" % (wait))
    time.sleep(wait)
    for __ in range(120):
        res = clus.check_sync(min_sync_count=2, max_block_lag=2, dont_raise=True)
        min2, max2 = res.min_block_num, res.max_block_num
        assert_out_of_sync(clus, res)
        if max2 > max1 and min2 > min1:
            clus.info("now we have 2 forks, min block num is %d, max block num is %d" % (min2, max2))
            return
        time.sleep(1)
    msg = f"Failed to have two diverse chains, max block num is {max2} (was {max1}), min block num is {min2} (was {min1})"
    clus.error(msg)
    raise BlockchainError(msg)


def restart_and_verify(clus, last_block_in_sync):
    res = clus.check_sync(min_sync_count=2, max_block_lag=2, dont_raise=True)
    _, watermark = res.min_block_num, res.max_block_num
    tbeg = time.time()
    clus.info("Restart bridge node...")
    clus.start_node(node_id=1)
    try:
        res = clus.check_sync()
        tend = time.time()
        watermark += (tend - tbeg) * 2 + 1
    except SyncError:
        clus.get_cluster_info(level="flag", response_text_level="flag")
        clus.get_cluster_running_state(level="flag", response_text_level="flag")
        raise
    if res.block_num <= last_block_in_sync:
        raise BlockchainError(f"Chain stops advancing at block num {res.block_num}")

    clus.info("forks resolved with block num %d, verifying blocks..." % (res.block_num))
    for block_num in range(last_block_in_sync, res.block_num + 1):
        blk0 = clus.get_block(block_num, node_id=0).response_dict
        blk1 = clus.get_block(block_num, node_id=1).response_dict
        blk2 = clus.get_block(block_num, node_id=2).response_dict
        if not (blk0 == blk1 and blk1 == blk2):
            msg = f"block verification failed at block {block_num}"
            clus.error(msg)
            raise BlockchainError(msg)

    res = clus.check_sync()
    tries = 120
    while watermark > res.block_num and tries > 0:
        if tries == 120:
            clus.info("wait until head num(%d) pass the estimated watermark(%d)" % (res.block_num, watermark))
        time.sleep(1)
        res = clus.check_sync()
        tries -= 1
    return res.block_num

def main():
    with init_cluster() as clus:
        clus.info(">>> [Fork Test] ------------------------- BEGIN ----------------------------------------------------")
        last_block_in_sync = set_and_verify(clus)
        for _ in range(0, 20):
            kill_and_verify(clus)
            last_block_in_sync = restart_and_verify(clus, last_block_in_sync)
        clus.info(">>> [Fork Test] ------------------------- END ------------------------------------------------------")


if __name__ == "__main__":
    main()
