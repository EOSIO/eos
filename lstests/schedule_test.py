#! /usr/bin/env python3

import time

from logger import LogLevel, WriterConfig, ScreenWriter, FileWriter, Logger
from service import Service, Cluster

def init_cluster():
    debug_buffer_colo = WriterConfig(threshold="DEBUG", buffered=True,  monochrome=False)
    trace_buffer_colo = WriterConfig(threshold="TRACE", buffered=True,  monochrome=False)
    trace_unbuff_mono = WriterConfig(threshold="TRACE", buffered=False, monochrome=True)
    logger = Logger(ScreenWriter(config=debug_buffer_colo),
                    FileWriter(config=debug_buffer_colo, filename="debug_buffer_colo.log"),
                    FileWriter(config=trace_buffer_colo, filename="trace_buffer_colo.log"),
                    FileWriter(config=trace_unbuff_mono, filename="trace_unbuff_mono.log"))
    service = Service(logger=logger)
    total_nodes = 3
    producers_per_node = 4
    cluster = Cluster(service=service,
                      total_nodes=total_nodes,
                      producer_nodes=total_nodes,
                      total_producers=total_nodes * producers_per_node,
                      dont_bootstrap=True)
    return cluster


def create_accounts(cluster):
    print_info = lambda msg: cluster.logger.info(msg=msg)
    print_info("create producer accounts using bios contract...")
    accs = cluster.nodes[1]["producers"] + cluster.nodes[2]["producers"]
    acc_list = []
    for p in accs:
        acc_list.append({"name":p})
    cluster.create_bios_accounts(accounts=acc_list)




def main():
    cluster = init_cluster()

    print_info = lambda msg: cluster.logger.info(msg=msg)
    print_info(">>> Producer schedule test starts.")

    create_accounts(cluster)
    cluster.set_producers(cluster.nodes[1]["producers"])
    cluster.check_production_round(cluster.nodes[1]["producers"], level="DEBUG")

    cluster.set_producers(cluster.nodes[2]["producers"])
    cluster.check_production_round(cluster.nodes[2]["producers"], level="DEBUG")

if __name__ == "__main__":
    main()



























