#!/usr/bin/env python3

import random
import time

from core.logger import ScreenWriter, FileWriter, Logger
from core.service import Service, Cluster, BlockchainError, SyncError


def init_cluster():
    test = "fork"
    logger = Logger(ScreenWriter(threshold="info"),
                    FileWriter(filename=f"{test}-info.log", threshold="info", monochrome=True),
                    FileWriter(filename=f"{test}-debug.log", threshold="debug", monochrome=True),
                    FileWriter(filename=f"{test}-trace.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service, node_count=3, pnode_count=3, producer_count=7,
                      topology="bridge", center_node_id=1, dont_setprod=True,
                      special_log_levels=[["net_plugin_impl", "debug"]])
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
    # randomize fork point
    sleep = random.randint(0, 7)
    clus.info(f"Sleep for {sleep}s before killing the bridge node")
    time.sleep(sleep)
    clus.stop_node(node_id=1, kill_sig=15)
    time.sleep(1)
    # sleep until bridge node has fully shut down
    for _ in range(15):
        res = clus.check_sync(min_sync_count=2, max_block_lag=2, dont_raise=True)
        if not res.in_sync:
            break
        time.sleep(1)
    assert_out_of_sync(clus, res)
    min1, max1 = res.min_block_num, res.max_block_num
    # Randomize fork difference
    sleep = random.randint(1, 45)
    clus.info(f"Sleep for {sleep}s until 2 forks have different lengths")
    time.sleep(sleep)
    for _ in range(120):
        res = clus.check_sync(min_sync_count=2, max_block_lag=2, dont_raise=True)
        min2, max2 = res.min_block_num, res.max_block_num
        assert_out_of_sync(clus, res)
        if max2 > max1 and min2 > min1:
            clus.info(f"Now we have 2 forks: min block num {min2}, max block num {max2}")
            return
        time.sleep(1)
    msg = f"Failed to have two diverse chains: max block num is {max2} (was {max1}), min block num is {min2} (was {min1})"
    clus.error(msg)
    raise BlockchainError(msg)


def restart_and_verify(clus, last_block_in_sync):
    res = clus.check_sync(min_sync_count=2, max_block_lag=2, dont_raise=True)
    watermark = res.max_block_num
    tbeg = time.time()
    clus.info("Restart bridge node...")
    clus.start_node(node_id=1)
    try:
        res = clus.check_sync()
    except SyncError:
        clus.get_cluster_info(level="flag", response_text_level="flag")
        clus.get_cluster_running_state(level="flag", response_text_level="flag")
        raise
    tend = time.time()
    watermark += (tend - tbeg) * 2 + 1
    if res.block_num <= last_block_in_sync:
        raise BlockchainError(f"Chain stopped advancing at block num {res.block_num}")
    clus.info(f"Forks resolved with block num {res.block_num}, verifying blocks...")
    for block_num in range(last_block_in_sync, res.block_num + 1):
        blk0 = clus.get_block(block_num, node_id=0).response_dict
        blk1 = clus.get_block(block_num, node_id=1).response_dict
        blk2 = clus.get_block(block_num, node_id=2).response_dict
        if not (blk0 == blk1 and blk1 == blk2):
            msg = f"Block verification failed at block {block_num}"
            clus.error(msg)
            raise BlockchainError(msg)
    clus.info(f"Wait until head num ({res.block_num}) passes the estimated watermark ({watermark:.1f})")
    for _ in range(120):
        res = clus.check_sync()
        if res.block_num >= watermark:
            break
        time.sleep(1)
    return res.block_num


def main():
    with init_cluster() as clus:
        testname = "Fork Test"
        clus.print_begin(testname)
        last_block_in_sync = set_and_verify(clus)
        for _ in range(0, 20):
            kill_and_verify(clus)
            last_block_in_sync = restart_and_verify(clus, last_block_in_sync)
        clus.print_end(testname)


if __name__ == "__main__":
    main()
