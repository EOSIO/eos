#! /usr/bin/env python3

from core.logger import Logger, ScreenWriter, FileWriter
from core.service import Service, Cluster


def init_cluster():
    test = "schedule"
    logger = Logger(ScreenWriter(threshold="info"),
                    FileWriter(filename=f"{test}-info.log", threshold="info",  monochrome=True),
                    FileWriter(filename=f"{test}-debug.log", threshold="debug",  monochrome=True),
                    FileWriter(filename=f"{test}-trace.log", threshold="trace",  monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service, node_count=3, pnode_count=3, producer_count=12, dont_setprod=True,
                      verify_retry=600, verify_async=True)
    return cluster


def test_round(clus, round, prods, new_prods, verify_key="irreversible"):
        clus.info(f">>> [Production Schedule Test] Round {round}: Set Producers")
        clus.info(">>> Producers to set:")
        for i, v in enumerate(prods):
            clus.info(f"[{i}] {v}")
        clus.set_producers(prods, verify_key=verify_key)
        clus.info(f">>> [Production Schedule Test] Round {round}: Verify Production Round")
        clus.check_production_round(expected_producers=prods, new_producers=new_prods)


def main():
    with init_cluster() as clus:
        testname = "Production Schedule Test"
        clus.print_begin(testname)
        test_round(clus, round=1, prods=clus.node_to_producers[1], new_prods=clus.node_to_producers[1], verify_key="irreversible")
        test_round(clus, round=2, prods=clus.node_to_producers[2], new_prods=clus.node_to_producers[2], verify_key="irreversible")
        test_round(clus, round=3, prods=clus.node_to_producers[1] + clus.node_to_producers[2], new_prods=clus.node_to_producers[1], verify_key="irreversible")
        clus.print_end(testname)


if __name__ == "__main__":
    main()
