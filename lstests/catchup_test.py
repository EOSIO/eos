#! /usr/bin/env python3

import time

from core.logger import ScreenWriter, FileWriter, Logger
from core.service import Service, Cluster, LauncherServiceError, BlockchainError, SyncError

TEST_PLUGIN = "eosio::txn_test_gen_plugin"
CREATE_URL = "/v1/txn_test_gen/create_test_accounts"
START_URL = "/v1/txn_test_gen/start_generation"
STOP_URL = "/v1/txn_test_gen/stop_generation"
PRIVATE_KEY = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
CREATE_STR = f"[\"eosio\", \"{PRIVATE_KEY}\"]"
START_STR = "[\"salt\",10,10]"
REQUIRED_AVG = 150
SLIDING_AVG_WINDOW = 30
REQUIRED_SLIDING_WINDOW_AVG = 80
CATCHUP_ROUNDS = 3
REPEAT_EXCEPTIONS_ALLOWED = 10


def init_cluster():
    test = "catchup"
    logger = Logger(ScreenWriter(threshold="info"),
                    FileWriter(filename=f"{test}-info.log", threshold="info", monochrome=True),
                    FileWriter(filename=f"{test}-debug.log", threshold="debug", monochrome=True),
                    FileWriter(filename=f"{test}-trace.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service, node_count=3, pnode_count=3, producer_count=3,
                      dont_newacco=True,
                      extra_configs=[f"plugin={TEST_PLUGIN}",
                                     f"txn-test-gen-allowed-repeat-exceptions={REPEAT_EXCEPTIONS_ALLOWED}"])
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


def start_gen(clus):
    clus.info(">>> [Catch-up Test] Generate Transactions")
    clus.call("send_raw", url=START_URL, string_data=START_STR, node_id=1, level="trace")
    blks = 30
    leeway_time = blks * 0.5 + 2
    begin = clus.get_head_block_number(node_id=0)
    clus.info(f"Generation begins around block num {begin}")
    clus.debug(f"Wait {leeway_time} seconds for {blks} blocks...")
    time.sleep(leeway_time)
    end = clus.check_sync().block_num
    if end - begin < blks:
        raise BlockchainError(f"Blocks produced ({end - begin}) are fewer than expected ({blks})")
    clus.info(f"Generation syncing ends at block num {end}")
    return (begin, end)


def stop_gen(clus):
    clus.info(">>> [Catch-up Test] Stop Generating Transactions")
    begin = clus.get_head_block_number(node_id=0)
    try:
        clus.call("send_raw", url=STOP_URL, node_id=1, level="trace")
    except LauncherServiceError:
        clus.warn("Failed to stop generation, this is likely due to the txn_test_gen_plugin having already stopped." +
                  "If this is a problem, count_gen will identify it.")

    blks = 5
    clus.info(f"Generation stops around block num {begin}")
    return begin


def count_gen(clus, begin, end):
    total = 0
    sliding_total = 0
    sliding_window_avg_min = None
    window = []
    for i in range(begin + 1, end + 1):
        n = len(clus.get_block(i, level="trace").response_dict["transactions"])
        clus.info(f"Block {i} has {n} transactions.")
        total += n
        full_window = len(window) == SLIDING_AVG_WINDOW
        if full_window:
            old = window.pop(0)
            sliding_total -= old
        window.append(n)
        sliding_total += n
        if full_window:
            window_avg = sliding_total / SLIDING_AVG_WINDOW
            if window_avg < REQUIRED_SLIDING_WINDOW_AVG:
                raise BlockchainError(f"The average number of transactions per block ({window_avg}) over the last {SLIDING_AVG_WINDOW} blocks is less than required ({REQUIRED_SLIDING_WINDOW_AVG})")
            if sliding_window_avg_min is None or sliding_window_avg_min > window_avg:
                sliding_window_avg_min = window_avg

    clus.info(f"There are {total} transactions in {end - begin} blocks. Lowest sliding window average transactions per block: {sliding_window_avg_min}")
    avg = total / (end - begin)
    if avg < REQUIRED_AVG:
        raise BlockchainError(f"The average number of transactions per block ({avg}) is less than required ({REQUIRED_AVG})")
    return total


def assert_out_of_sync(clus, res):
    if res.in_sync:
        raise SyncError("Node 2 should be out of sync as it has been shut down")
    else:
        clus.info(f"Nodes out of sync")


def assert_in_sync(clus, begin, end):
    if end <= begin:
        raise BlockchainError(f"Chain stopped advancing at block num {end}")
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
        testname = "Catch-up Test"
        clus.print_begin(testname)
        begin = create_accounts(clus)
        (begin, end) = start_gen(clus)
        for i in range(CATCHUP_ROUNDS):
            end = catchup(clus, end, i+1)
        end = stop_gen(clus)
        # ensure transaction generation was maintained the entire time
        count_gen(clus, begin, end)
        clus.print_end(testname)

if __name__ == "__main__":
    main()
