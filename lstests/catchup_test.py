#! /usr/bin/env python3

import time

from core.logger import ScreenWriter, FileWriter, Logger
from core.service import Service, Cluster, BlockchainError, SyncError

TEST_PLUGIN = "eosio::txn_test_gen_plugin"
CREATE_URL = "/v1/txn_test_gen/create_test_accounts"
START_URL = "/v1/txn_test_gen/start_generation"
STOP_URL = "/v1/txn_test_gen/stop_generation"
PRIVATE_KEY = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
CREATE_STR = f"[\"eosio\", \"{PRIVATE_KEY}\"]"
START_STR = "[\"salt\",5,10]"
CATCHUP_ROUNDS = 3


def init_cluster():
    test = "catchup"
    logger = Logger(ScreenWriter(threshold="info"),
                    FileWriter(filename=f"{test}-info.log", threshold="info", monochrome=True),
                    FileWriter(filename=f"{test}-debug.log", threshold="debug", monochrome=True),
                    FileWriter(filename=f"{test}-trace.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service, node_count=3, pnode_count=3, producer_count=3,
                      dont_newaccount=True, extra_configs=[f"plugin={TEST_PLUGIN}"])
    return cluster


def create_accounts(clus):
    clus.info(">>> [Catch-up Test] Create Test Accounts")
    for _ in range(5):
        cx = clus.call("send_raw", url=CREATE_URL, string_data=CREATE_STR, node_id=1,
                        retry=0, level="trace", error_level="trace", dont_raise=True)
        if '"name": "account_name_exists_exception"' in cx.response_text:
            return clus.check_sync().block_num
        time.sleep(1)
    raise BlockchainError(f"Failed to create test accounts with {TEST_PLUGIN}")


def start_gen(clus, begin):
    clus.info(">>> [Catch-up Test] Generate Transactions")
    clus.call("send_raw", url=START_URL, string_data=START_STR, node_id=1, level="trace")
    blks = 30
    clus.info(f"Generation begins at block num {begin}")
    clus.debug(f"Wait {blks * 0.5} seconds for {blks} blocks...")
    time.sleep(blks * 0.5)
    end = clus.check_sync().block_num
    if end - begin < blks:
        raise BlockchainError(f"Blocks produced ({end - begin}) are fewer than expected ({blks})")
    clus.info(f"Generation ends at block num {end}")
    return end


def stop_gen(clus):
    clus.call("send_raw", url=STOP_URL, node_id=1)
    stop = clus.check_sync().block_num
    clus.info(f"Generation stops at block num {stop}")
    return stop


def count_gen(clus, begin, end):
    total = 0
    for i in range(begin + 1, end + 1):
        n = len(clus.get_block(i, level="trace").response_dict["transactions"])
        clus.info(f"Block {i} has {n} transactions.")
        total += n
    clus.info(f"There are {total} transactions in {end - begin} blocks.")
    return total


def assert_out_of_sync(clus, res):
    if res.in_sync:
        raise SyncError("Node 2 should be out of sync as it has been shut down")
    else:
        clus.info(f"Nodes out of sync")


def assert_in_sync(clus, begin, end):
    if end <= begin:
        raise BlockchainError(f"Chain stops advancing at block num {end}")
    clus.info(f"Nodes in sync at block num {end}")


def catchup(clus, begin, round):
    clus.info(f"-------------------------------\n>>> [Catch-up Test] Round {round}")
    clus.info("-------------------------------\nGracefully shut down node...")
    clus.stop_node(node_id=2, kill_sig=15)
    res = clus.check_sync(retry=5, dont_raise=True)
    assert_out_of_sync(clus, res)

    clus.info("-------------------------------\nRestart node...")
    clus.start_node(node_id=2)
    restart = clus.check_sync().block_num
    assert_in_sync(clus, begin, restart)

    clus.info("-------------------------------\nHard-kill node...")
    clus.stop_node(node_id=2, kill_sig=9)
    res = clus.check_sync(retry=5, dont_raise=True)
    assert_out_of_sync(clus, res)

    clus.info("-------------------------------\nFreshly restart node...")
    clus.start_node(node_id=2, extra_args="--delete-all-blocks")
    refresh = clus.check_sync().block_num
    assert_in_sync(clus, restart, refresh)

    return refresh


def main():
    with init_cluster() as clus:
        clus.info(">>> [Catch-up Test] --------------------- BEGIN ----------------------------------------------------")
        begin = create_accounts(clus)
        end = start_gen(clus, begin)
        stop = stop_gen(clus)
        count_gen(clus, begin, end)
        for i in range(CATCHUP_ROUNDS):
            stop = catchup(clus, stop, i+1)
        clus.info(">>> [Catch-up Test] --------------------- END ------------------------------------------------------")


if __name__ == "__main__":
    main()
