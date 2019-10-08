#!/usr/bin/env python3

import printer
import json
import time
from service import Service, Cluster, CommandLineArguments
# from logger import logging_func

# logger = Logger(ScreenWriter(config=LoggingConfig(buffered=False)))
max_seen_blocknum = 1

def wait_get_block(cluster, block_num):
    tries = 10
    global max_seen_blocknum
    while (max_seen_blocknum < block_num) and tries > 0:
        inforsp = cluster.get_cluster_info()
        max_seen_blocknum = inforsp["result"][0][1]["head_block_num"]
        if (max_seen_blocknum < block_num):
            time.sleep(0.5 * (block_num - max_seen_blocknum + 1))
        tries = tries - 1
    assert tries > 0, print("failed to get block %d, max_seen_blocknum is %d" % (block_num, max_seen_blocknum))
    return cluster.get_block(block_num)

def verifyProductionRound(cluster, exp_prod):

    inforsp = cluster.get_cluster_info()
    head_num = inforsp["result"][0][1]["head_block_num"]
    print("head_num is %d" % (head_num))

    curprod = "(none)"
    while not curprod in exp_prod:
        block = wait_get_block(cluster, head_num)
        curprod = block["producer"]
        print("head_num is %d, producer %s, waiting for schedule change" % (head_num, curprod))
        head_num = head_num + 1

    seen_prod = {curprod : 1}
    verify_end_num = head_num + 12 * len(exp_prod)
    for blk_num in range(head_num, verify_end_num):
        block = wait_get_block(cluster, blk_num)
        curprod = block["producer"]
        print("block %d, producer %s, %d blocks remain to verify" % (blk_num, curprod, verify_end_num - blk_num - 1))
        assert curprod in exp_prod, print("producer %s is not expected in block %d" % (curprod, blk_num))
        seen_prod[curprod] = 1

    if len(seen_prod) == len(exp_prod):
        print("verification succeed")
        return True

    print("verification failed, #seen_prod is %d, expect %d" % (len(seen_prod), len(exp_prod)))
    return False

# def control():

#     logger.log("Hello")
#     another_test(logger)

# def test2():
#     logging_func(...)


def main():
    service = Service()
    # service.log(..., level="info")
    # service.info()
    # service.debug(...)
    service.print.decorate(printer.pad(">>> Voting test starts.", left=0, char=' ', sep=""), fcolor="white", bcolor="black")
    total_nodes = 4
    cluster = Cluster(service=service, total_nodes=total_nodes, total_producers=total_nodes * 21, producer_nodes=total_nodes, dont_vote=True)


    testers = []
    for i in range(1, 6):
        testers.append("tester" + str(i) * 6)


    stake_amount_formatted = "{:.4f} SYS".format(37.5e6)


    for i in range(5):
        cluster.create_account(node_id=0, creator="eosio", name=testers[i], stake_cpu=stake_amount_formatted, stake_net=stake_amount_formatted, buy_ram_bytes=1048576, transfer=True)


    # TODO: remove eosio after bootstrap
    node_prod = cluster.nodes[0]["producers"]
    node_prod.remove("eosio")
    cluster.vote_for_producers(node_id=0, voter=testers[0], voted_producers=node_prod, verify=False)
    for i in range(1, 5):
        node_prod = cluster.nodes[1]["producers"]
        node_prod.sort()
        cluster.vote_for_producers(node_id=0, voter=testers[i], voted_producers=node_prod, verify=False)

    print("verfiying schedule...")
    verifyProductionRound(cluster, node_prod)

    # change schedule
    node_prod = cluster.nodes[2]["producers"]
    node_prod.sort()
    cluster.vote_for_producers(node_id=0, voter=testers[0], voted_producers=node_prod, verify=False)
    for i in range(1, 5):
        node_prod = cluster.nodes[3]["producers"]
        node_prod.sort()
        cluster.vote_for_producers(node_id=0, voter=testers[i], voted_producers=node_prod, verify=False)

    print("verfiying schedule change...")
    verifyProductionRound(cluster, node_prod)

    service.print.decorate(printer.pad(">>> Voting test finished successfully.", left=0, char=' ', sep=""), fcolor="white", bcolor="black")



if __name__ == '__main__':
    main()