#! /usr/bin/env python3

from core.logger import Logger, ScreenWriter, FileWriter
from core.service import Service, Cluster


def init_cluster():
    test = "schedule"
    logger = Logger(ScreenWriter(threshold="info"),
                    FileWriter(filename=f"{test}-info.log", threshold="info", monochrome=True),
                    FileWriter(filename=f"{test}-debug.log", threshold="debug", monochrome=True),
                    FileWriter(filename=f"{test}-trace.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service, node_count=3, pnode_count=3, producer_count=12, dont_setprod=True, verify_retry=600, verify_async=True)
    return cluster


def test_round(clus, round, verify_key="irreversible"):
        clus.info(f">>> [Production Schedule Test] Round {round}: Set Producers")
        clus.info(">>> Producers to set:")
        prod = clus.node_to_producers[round]
        for i, v in enumerate(prod):
            clus.info(f"[{i}] {v}")
        clus.set_producers(prod, verify_key=verify_key)
        clus.info(f">>> [Production Schedule Test] Round {round}: Verify Production Round")
        clus.check_production_round(prod)


def main():
    with init_cluster() as clus:
        clus.info(">>> [Production Schedule Test] ---------- BEGIN ------------------------------------------")
        test_round(clus, round=1, verify_key="irreversible")
        test_round(clus, round=2, verify_key="irreversible")
        clus.info(">>> [Production Schedule Test] ---------- END --------------------------------------------")


if __name__ == "__main__":
    main()
