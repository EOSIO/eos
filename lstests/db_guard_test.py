#!/usr/bin/env python3

import time

from core.logger import ScreenWriter, FileWriter, Logger
from core.service import Service, Cluster, BlockchainError, bassert

account_count = 0

def init_cluster():
    test = "db-guard"
    logger = Logger(ScreenWriter(threshold="info"),
                    FileWriter(filename=f"{test}-info.log", threshold="info", monochrome=True),
                    FileWriter(filename=f"{test}-debug.log", threshold="debug", monochrome=True),
                    FileWriter(filename=f"{test}-trace.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service, node_count=4, pnode_count=4, producer_count=4, dont_newacco=True)
    return cluster


def main():
    with init_cluster() as clus:
        def print_db():
            clus.info(f"Guard: {guard} MB")
            clus.info(f"Capacity: {cap} MB")

        def restart_nodes():
            for i in range(1, clus.node_count):
                clus.stop_node(node_id=i, kill_sig=15)
            time.sleep(1)
            for i in range(1, clus.node_count):
                clus.start_node(node_id=i, extra_args=f"--chain-state-db-guard-size-mb {guard} "
                                                      f"--chain-state-db-size-mb {cap}")

        def check_sync():
            clus.info(f">>> Check Sync")
            clus.check_sync()

        def push_limit():
            # conservatively assuming that ram  usage is 512 bytes for one account
            while (clus.get_running_nodes() == clus.node_count and
                   account_count < (cap - guard) * 1024 * 1024 / 512):
                create_accounts(clus, 100)
                if account_count % 1000 == 0:
                    clus.info(f"Account count: {account_count}")

        def create_accounts(clus, num):
            global account_count
            acco = []
            for i in range(0, num):
                acco.append(Cluster.make_defproducer_name(account_count))
                account_count += 1
            clus.bios_create_accounts(accounts=acco, verify_key=None, level="trace")

        testname = "DB Guard Test"
        clus.print_begin(testname)
        clus.info(f">>> [DB Guard Test] Step 0: Set Up Database Guard")
        guard = 1002
        cap = 1010
        print_db()
        restart_nodes()
        for i in range(1, clus.node_count):
            clus.info(f"Database guard set for node {i}")
        check_sync()

        clus.info(f">>> [DB Guard Test] Step 1: Keep Creating Accounts until Database Guard is Triggered")
        push_limit()
        time.sleep(1)
        run_node_count = clus.get_running_nodes()
        bassert(run_node_count == 1, f"Nodes 1, 2, 3 should have been shut down. Running node count = {run_node_count}.")
        clus.info(f"{account_count} accounts created before nodes are shut down")
        clus.info(f">>> Examine Log Files")
        target_string = "database chain::guard_exception"
        for i in range(1, clus.node_count):
            log = clus.get_log(node_id=i)
            if log.find(target_string):
                clus.info(f"\"{target_string}\" found in node {i}'s log")
            else:
                msg = f"Node {i}'s log does not contain {target_string}"
                clus.error(msg)
                raise BlockchainError(msg)

        clus.info(f">>> [DB Guard Test] Step 2: Restart Nodes with New Capacity")
        for _ in range(10):
            cap += 4
            print_db()
            restart_nodes()
            if clus.check_sync(retry=10, dont_raise=True).in_sync:
                clus.info(f"Managed to restart.\n-------------------")
                break
            clus.info(f"Failed to restart.\n-------------------")
        else:
            msg = f"Nodes cannot be restarted with guard={guard}mb, capacity={cap}mb"
            clus.error(msg)
            raise BlockchainError(msg)

        clus.info(f">>> [DB Guard Test] Step 3: Keep Creating Accounts until Database Guard is Triggered")
        push_limit()
        bassert(clus.get_running_nodes() < clus.node_count, "At least one node should have been shut down")
        found = False
        for i in range(1, clus.node_count):
            log = clus.get_log(node_id=i)
            if log.find(target_string):
                clus.info("Node %d's log contains \"%s\"" % (i, target_string))
                found = True
        bassert(found, "Cannot find chain::guard_exception in any node's log")

        clus.info(f">>> [DB Guard Test] Step 4: Restart with Much Larger Capacity")
        cap += 100
        print_db()
        restart_nodes()
        check_sync()

        clus.info(f">>> [DB Guard Test] Step 5: Create 100 More Accounts")
        create_accounts(clus, 100)
        time.sleep(1)
        check_sync()
        clus.print_end(testname)


if __name__ == "__main__":
    main()
