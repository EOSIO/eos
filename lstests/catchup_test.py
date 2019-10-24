#! /usr/bin/env python3

import time

import core
from logger import LogLevel, ScreenWriter, FileWriter, Logger
from service import Service, Cluster


PLUGIN = "eosio::txn_test_gen_plugin"
txn_gen_node = 1
catchupCount = 3


def init_cluster():
    logger = Logger(ScreenWriter(threshold="debug"),
                    FileWriter(filename="debug.log", threshold="debug"),
                    FileWriter(filename="trace.log", threshold="trace"),
                    FileWriter(filename="mono.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    total_nodes = 3
    cluster = Cluster(service=service,
                      total_nodes=total_nodes,
                      total_producers=total_nodes,
                      producer_nodes=total_nodes,
                      dont_bootstrap=True,
                      extra_configs=["plugin={}".format(PLUGIN)])
    return cluster



def create_test_accounts(cluster):

    cluster.logger.info("Create test accounts on node {} via {}".format(txn_gen_node, PLUGIN))
    tries = 5
    while tries >= 0:
        cx = cluster.call("send_raw", retry=0, node_id=txn_gen_node, url="/v1/txn_test_gen/create_test_accounts", string_data="[\"eosio\",\"5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\"]")
        # first response 500 likely to be a bug
        if '"name": "account_name_exists_exception"' in cx.response_text:
            cluster.logger.info("Found")
            break
        tries -= 1
        time.sleep(1.0)



def generate_transactions(cluster, start_block_num):
    cluster.logger.info("Start generating transactions on node {} via {}".format(txn_gen_node, PLUGIN))
    cluster.call("send_raw", retry=10, node_id=txn_gen_node, url="/v1/txn_test_gen/start_generation", string_data="[\"salt\",5,10]")
    blocks_to_wait = 10
    cluster.logger.info("Wait for {} seconds for {} blocks...".format(blocks_to_wait * 0.5, blocks_to_wait))
    cluster.logger.info("Begin block is {}".format(start_block_num))
    time.sleep(blocks_to_wait * 0.5)
    tries = 5
    while tries >= 0:
        end_block_num = cluster.check_sync()[1]
        if end_block_num - start_block_num >= blocks_to_wait:
            break
        tries -= 1
        time.sleep(1)
    cluster.logger.info("Tries = {}".format(tries))
    cluster.logger.info("End block is {}".format(end_block_num))
    return end_block_num


def stop_generation(cluster):
    cluster.logger.info("Stop generating")
    cluster.call("send_raw", retry=10, node_id=txn_gen_node, url="/v1/txn_test_gen/stop_generation")
    return cluster.check_sync()[1]



def count_txn(cluster, beg, end):
    count = 0
    for i in range(beg + 1, end + 1):
        txn = len(cluster.get_block(i).response_dict["transactions"])
        cluster.logger.info("Block {} has {} transactions.".format(i, txn))
        count += txn

    cluster.logger.info("In total {} transactions in {} blocks".format(count, end-beg))

    return count



def catchup(cluster, block_num_2):
    cluster.print_header("Catch up")

    print_info = cluster.logger.info

    print_info("Gracefully shutdown node 2")
    cluster.terminate_node(node_id=2)

    r,minN,maxN = cluster.check_sync(retry=10)
    assert r == False, "node should not in-sync after stopping node 2"

    print_info("Restart node #2...")
    cluster.start_node(node_id=2)
    block_num_3 = cluster.check_sync()[1]

    assert block_num_3 > block_num_2, "Chain doesn't advance"

    print_info("Hard-kill node #2...")
    cluster.kill_node(node_id=2)

    r,minN,maxN = cluster.check_sync(retry=10)
    assert r == False, "node should not in-sync after stopping node 2"

    print_info("Restarting node 2 with --delete-all-blocks...")
    cluster.start_node(node_id=2, extra_args="--delete-all-blocks")
    block_num_4 = cluster.check_sync()[1]
    print_info("Cluster in-sync after restart with --delete-all-blocks, blocknum {}".format(block_num_4))

    assert block_num_4 > block_num_3, "Chain doesn't advance"
    block_num_2 = block_num_4


def main():
    cluster = init_cluster()
    create_test_accounts(cluster)
    block_num = cluster.check_sync()[1]
    block_num_1 = generate_transactions(cluster, block_num)
    block_num_2 = stop_generation(cluster)
    count_txn(cluster, block_num, block_num_1)

    for k in range(catchupCount):
        catchup(cluster, block_num_2)



if __name__ == "__main__":
    main()











































