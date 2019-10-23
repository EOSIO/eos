#! /usr/bin/env python3

# user-defined modules
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
    producers_per_node = 4
    cluster = Cluster(service=service,
                      total_nodes=total_nodes,
                      producer_nodes=total_nodes,
                      total_producers=total_nodes * producers_per_node,
                      dont_bootstrap=True)
    return cluster


def main():
    # Using "with", ensure that logger buffer is flushed when an exception is thrown
    with init_cluster() as cluster:
        cluster.info(">>> Producer schedule test starts.")
        # create producer accounts using bios contract
        cluster.info("Create producer accounts using bios contract...")
        producers = cluster.nodes[1]["producers"] + cluster.nodes[2]["producers"]
        accounts = []
        for p in producers:
            accounts.append({"name":p})
        cluster.create_bios_accounts(accounts=accounts)
        # [round 1] set producers 1
        cluster.info("[Round 1] Set Producers")
        cluster.set_producers(cluster.nodes[1]["producers"])
        # [round 1] verify production
        cluster.check_production_round(cluster.nodes[1]["producers"], level="DEBUG")
        # [round 2] set producers
        cluster.set_producers(cluster.nodes[2]["producers"], verify_key="contained")
        # [round 2] verify production
        cluster.check_production_round(cluster.nodes[2]["producers"], level="DEBUG")
        cluster.info(">>> Producer schedule test ends.")


if __name__ == "__main__":
    main()
