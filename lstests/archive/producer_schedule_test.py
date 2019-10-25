#!/usr/bin/env python3

import json
import time

import helper
from logger import LogLevel, WriterConfig, ScreenWriter, FileWriter, Logger
from service import Service, Cluster, CommandLineArguments

max_seen_blocknum = 1

def wait_get_block(cluster, block_num, print):
    tries = 10
    global max_seen_blocknum
    while (max_seen_blocknum < block_num) and tries > 0:
        ix = cluster.get_cluster_info()
        inforsp = json.loads(ix.response.text)
        max_seen_blocknum = inforsp["result"][0][1]["head_block_num"]
        if (max_seen_blocknum < block_num):
            time.sleep(0.5 * (block_num - max_seen_blocknum + 1))
        tries = tries - 1
    assert tries > 0, "failed to get block %d, max_seen_blocknum is %d" % (block_num, max_seen_blocknum)
    return json.loads(cluster.get_block(block_num).response.text)

def verifyProductionRound(cluster, exp_prod, print):

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

    ix = cluster.get_cluster_info()
    inforsp = json.loads(ix.response.text)
    last_lib0 = inforsp["result"][0][1]["last_irreversible_block_num"]

    seen_prod = {curprod : 1}
    verify_end_num = head_num + 12 * (len(exp_prod) + 1) + 2  # 1 round + 1 producer turn + 2 block tolerance
    for blk_num in range(head_num, verify_end_num):
        block = wait_get_block(cluster, blk_num, print=print)
        curprod = block["producer"]
        print("block %d, producer %s, %d blocks remain to verify" % (blk_num, curprod, verify_end_num - blk_num - 1))
        assert curprod in exp_prod, "producer %s is not expected in block %d" % (curprod, blk_num)
        seen_prod[curprod] = 1

    ix = cluster.get_cluster_info()
    inforsp = json.loads(ix.response.text)
    last_lib1 = inforsp["result"][0][1]["last_irreversible_block_num"]

    if len(seen_prod) == len(exp_prod) and last_lib1 > last_lib0:
        print("verification succeed and LIB is advancing from %d to %d" % (last_lib0, last_lib1))
        return True

    print("verification failed, #seen_prod is %d, expect %d" % (len(seen_prod), len(exp_prod)))
    return False

def setprods(cluster, node_prod):
    node_prod.sort()
    prod_keys = []
    for p in node_prod:
        prod_keys.append({"producer_name": p, "block_signing_key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"})

    actions = [{"account": "eosio",
                    "action": "setprods",
                    "permissions": [{"actor": "eosio", "permission": "active"}],
                    "data": { "schedule": prod_keys}}]
    cluster.push_actions(actions=actions, name="set producers")

def main():
    buffered_color_config = WriterConfig(buffered=True, monochrome=False, threshold="DEBUG")
    unbuffered_mono_config = WriterConfig(buffered=False, monochrome=True, threshold="TRACE")
    logger = Logger(ScreenWriter(config=buffered_color_config), FileWriter(filename="mono.log", config=unbuffered_mono_config))
    service = Service(logger=logger)

    print_info = lambda msg: logger.info(msg=msg)
    print_info(">>> Producer schedule test starts.")

    total_nodes = 3
    produers_per_node = 4

    cluster = Cluster(service=service, total_nodes=total_nodes, total_producers=total_nodes * produers_per_node, producer_nodes=total_nodes, dont_bootstrap=True)

    print_info("create producer accounts using bios contract...")
    accs = cluster.nodes[1]["producers"] + cluster.nodes[2]["producers"]
    acc_list = []
    for p in accs:
        acc_list.append({"name":p})
    cluster.call("create_bios_accounts",creator="eosio",accounts=acc_list,verify_key="irreversible")

    print_info("set producers from node 1 (%s)" % (','.join(cluster.nodes[1]["producers"])))
    setprods(cluster, cluster.nodes[1]["producers"])

    print_info("verfiying schedule 1...")
    verifyProductionRound(cluster, cluster.nodes[1]["producers"], print=print_info)

    print_info("set producers from node 2 (%s)" % (','.join(cluster.nodes[2]["producers"])))
    setprods(cluster, cluster.nodes[2]["producers"])

    print_info("verfiying new schedule 2...")
    verifyProductionRound(cluster, cluster.nodes[2]["producers"], print=print_info)

    print_info(">>> Producer schedule test finished successfully.")

if __name__ == '__main__':
    main()
