#!/usr/bin/env python3

import time


import core
from logger import LogLevel, ScreenWriter, FileWriter, Logger
from service import Service, Cluster


def init_cluster():
    logger = Logger(ScreenWriter(threshold="debug"),
                    FileWriter(filename="debug.log", threshold="debug"),
                    FileWriter(filename="trace.log", threshold="trace"),
                    FileWriter(filename="mono.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    total_nodes = 3
    total_producers = total_nodes * 2 + 1
    cluster = Cluster(service=service,
                      total_nodes=total_nodes,
                      producer_nodes=total_nodes,
                      total_producers=total_producers,
                      topology="bridge",
                      center_node_id=1,
                      dont_vote=True)
    return cluster




def vote_and_verify(cluster):
    stake_amount = "75000001.0000 SYS"
    cluster.create_account(node_id=0, creator="eosio", name="tester1", stake_cpu=stake_amount, stake_net=stake_amount, buy_ram_bytes=1048576, transfer=True)

    node_prod = cluster.nodes[0]["producers"] + cluster.nodes[2]["producers"]
    node_prod.remove("eosio")
    cluster.logger.info("vote for following producers (from node 0 and node 2):")
    for p in node_prod:
        if p in cluster.nodes[0]["producers"]:
            cluster.logger.info("%s (node 0)" % (p))
        else:
            cluster.logger.info("%s (node 2)" % (p))
    cluster.vote_for_producers(voter="tester1", voted_producers=node_prod)
    res,min_blk,max_blk = cluster.check_sync(min_sync_nodes=3, max_block_lag=2, dont_raise=True)
    assert res, "cluster is not in sync"

    cluster.print_header("Verifying schedule...", level="INFO")
    cluster.check_production_round(node_prod)


    res,min_blk,max_blk = cluster.check_sync(min_sync_nodes=3, max_block_lag=2, dont_raise=True)
    last_known_insync_blk = max_blk
    assert res, "cluster is not in sync"

    return last_known_insync_blk



def kill_and_verify(cluster):
    cluster.print_header("cluster is in-sync. killing bridge node (node 1)", level="INFO")
    cluster.stop_node(node_id=1, kill_sig=9)

    time.sleep(1)

    cluster.print_header("verify out of sync", level="INFO")
    res,min_blk0,max_blk0 = cluster.check_sync(min_sync_nodes=2, max_block_lag=2, dont_raise=True)
    assert res == False, "cluster should not in sync"

    cluster.print_header("wait until 2 forks has different lengths", level="INFO")
    tries = 60
    while tries > 0:
        time.sleep(1)
        res,min_blk1,max_blk1 = cluster.check_sync(min_sync_nodes=2, max_block_lag=2, dont_raise=True)
        cluster.logger.info("min head block num is %d, max head block num is %d" % (min_blk1, max_blk1))
        assert res == False, "cluster should not in sync"
        if max_blk1 >= min_blk1 + 13 and min_blk1 > min_blk0 and max_blk1 > max_blk0:
            break
        tries  -= 1
    assert min_blk1 > min_blk0 and max_blk1 > max_blk0, "nodes are not advancing"



def restart_and_verify(cluster, last_known_insync_blk):
    cluster.print_header("restarting bridge node with empty database", level="INFO")
    cluster.start_node(node_id=1, extra_args="--delete-all-blocks")

    tries = 60
    while tries > 0:
        res,min_blk,max_blk = cluster.check_sync(min_sync_nodes=3, max_block_lag=2, dont_raise=True)
        if res:
            break
        tries -= 1
        time.sleep(1)
    assert tries > 0, "cluster is not in sync"

    tries = 60
    while tries > 0:
        time.sleep(1)
        res,min_blk2,max_blk2 = cluster.check_sync(min_sync_nodes=3, max_block_lag=2, dont_raise=True)
        if res and min_blk2 > min_blk:
            break
        tries -= 1
    assert tries > 0, "cluster is not advancing"

    cluster.print_header("Nodes are in-synced and advancing, head block num is %d. Begin to verify from block %d to %d" % (max_blk2, last_known_insync_blk, max_blk2), level="INFO")

    for block_num in range(last_known_insync_blk, max_blk2 + 1):
        cluster.logger.debug(block_num)
        blk0 = cluster.get_block(block_num, node_id=0).response_dict
        blk1 = cluster.get_block(block_num, node_id=1).response_dict
        blk2 = cluster.get_block(block_num, node_id=2).response_dict
        if not (blk0 == blk1 and blk1 == blk2):
            print_error("block verification failed at block %d" % (block_num))
            assert False

    cluster.print_header("Fork successfully resolved", level="INFO")


def main():
    with init_cluster() as cluster:
        cluster.print_header("Fork test starts.", level="INFO")
        last_known_insync_blk = vote_and_verify(cluster)
        kill_and_verify(cluster)
        restart_and_verify(cluster, last_known_insync_blk)



if __name__ == '__main__':
    main()
