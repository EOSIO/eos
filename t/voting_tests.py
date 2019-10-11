#!/usr/bin/env python3

import json
import time

import helper
from logger import LoggingLevel, WriterConfig, ScreenWriter, FileWriter, Logger
from service import Service, Cluster, CommandLineArguments

max_seen_blocknum = 1

def wait_get_block(cluster, block_num, print):
    tries = 10
    global max_seen_blocknum
    while (max_seen_blocknum < block_num) and tries > 0:
        # inforsp = cluster.get_cluster_info()
        ix = cluster.get_cluster_info()
        inforsp = json.loads(ix.response.text)
        max_seen_blocknum = inforsp["result"][0][1]["head_block_num"]
        if (max_seen_blocknum < block_num):
            time.sleep(0.5 * (block_num - max_seen_blocknum + 1))
        tries = tries - 1
    assert tries > 0, "failed to get block %d, max_seen_blocknum is %d" % (block_num, max_seen_blocknum)
    return json.loads(cluster.get_block(block_num).response.text)

def verifyProductionRound(cluster, exp_prod, print):

    # inforsp = cluster.get_cluster_info()
    ix = cluster.get_cluster_info()
    inforsp = json.loads(ix.response.text)
    head_num = inforsp["result"][0][1]["head_block_num"]
    print("head_num is %d" % (head_num))

    curprod = "(none)"
    while not curprod in exp_prod:
        block = wait_get_block(cluster, head_num, print=print)
        curprod = block["producer"]
        print("head_num is %d, producer %s, waiting for schedule change" % (head_num, curprod))
        head_num = head_num + 1

    seen_prod = {curprod : 1}
    verify_end_num = head_num + 12 * len(exp_prod)
    for blk_num in range(head_num, verify_end_num):
        block = wait_get_block(cluster, blk_num, print=print)
        curprod = block["producer"]
        print("block %d, producer %s, %d blocks remain to verify" % (blk_num, curprod, verify_end_num - blk_num - 1))
        assert curprod in exp_prod, "producer %s is not expected in block %d" % (curprod, blk_num)
        seen_prod[curprod] = 1

    if len(seen_prod) == len(exp_prod):
        print("verification succeed")
        return True

    print("verification failed, #seen_prod is %d, expect %d" % (len(seen_prod), len(exp_prod)))
    return False


def main():
    buffered_color_config = WriterConfig(buffered=True, monochrome=False, threshold="DEBUG")
    unbuffered_mono_config = WriterConfig(buffered=False, monochrome=True, threshold="TRACE")
    logger = Logger(ScreenWriter(config=buffered_color_config), FileWriter(filename="mono.log", config=unbuffered_mono_config))
    service = Service(logger=logger)

    print_info = lambda msg: logger.info(msg=msg, flush=True)

    print_info(">>> Voting test starts.")
    total_nodes = 4
    cluster = Cluster(service=service, total_nodes=total_nodes, total_producers=total_nodes * 21, producer_nodes=total_nodes, dont_vote=True)

    testers = []
    for i in range(1, 6):
        testers.append("tester" + str(i) * 6)

    stake_amount_formatted = helper.format_tokens(3.75e7)

    for i in range(5):
        cluster.create_account(node_id=0, creator="eosio", name=testers[i], stake_cpu=stake_amount_formatted, stake_net=stake_amount_formatted, buy_ram_bytes=1048576, transfer=True)
        stake_amount_formatted = helper.format_tokens(3.75e7 + 100)

    # TODO: remove eosio after bootstrap
    node_prod = cluster.nodes[0]["producers"]
    node_prod.remove("eosio")
    cluster.vote_for_producers(node_id=0, voter=testers[0], voted_producers=node_prod, verify_key="contained")
    for i in range(1, 5):
        node_prod = cluster.nodes[1]["producers"]
        node_prod.sort()
        cluster.vote_for_producers(node_id=0, voter=testers[i], voted_producers=node_prod, verify_key="contained")



    print_info("verfiying schedule...")
    verifyProductionRound(cluster, node_prod, print=print_info)

    # change schedule
    node_prod = cluster.nodes[2]["producers"]
    node_prod.sort()
    cluster.vote_for_producers(node_id=0, voter=testers[0], voted_producers=node_prod, verify_key="contained")
    for i in range(1, 5):
        node_prod = cluster.nodes[3]["producers"]
        node_prod.sort()
        cluster.vote_for_producers(node_id=0, voter=testers[i], voted_producers=node_prod, verify_key="contained")

    print_info("verfiying schedule change...")
    verifyProductionRound(cluster, node_prod, print=print_info)

    print_info(">>> Voting test finished successfully.")


if __name__ == '__main__':
    main()
