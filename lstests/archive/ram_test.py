#!/usr/bin/env python3

import time

from core.logger import ScreenWriter, FileWriter, Logger
from core.service import Service, Cluster, BlockchainError, bassert

account_count = 0

def init_cluster():
    logger = Logger(ScreenWriter(threshold="debug"),
                    FileWriter(filename="info.log", threshold="info"),
                    FileWriter(filename="debug.log", threshold="debug"),
                    FileWriter(filename="trace.log", threshold="trace"))
    service = Service(logger=logger)
    cluster = Cluster(service=service, node_count=4, pnode_count=4, producer_count=4, dont_newaccount=True)
    return cluster


def create_accounts(clus, num):
    global account_count
    acco = []
    for i in range(0, num):
        acco.append(Cluster.get_defproducer_name(account_count))
        account_count += 1
    clus.bios_create_accounts(accounts=acco, verify_key="")


def main():
    with init_cluster() as clus:
        clus.info(">>> [RAM Test] --- BEGIN -----------------------------------------------------------------")
        clus.info(f">>> [RAM Test] Step 0: Set Up Database Guard")
        guard = 1002
        capacity = 1010
        clus.info(f"db-guard-size: {guard} MB")
        clus.info(f"db-size: {capacity} MB")
        for i in range(1, clus.node_count):
            clus.terminate_node(node_id=i)
        time.sleep(1)
        for i in range(1, clus.node_count):
            clus.start_node(node_id=i, extra_args=f"--chain-state-db-guard-size-mb {guard} --chain-state-db-size-mb {capacity}")
            clus.info(f"Database guard set for node {i}")
        clus.info(f">>> Check Sync")
        clus.check_sync()


        clus.info(f">>> [RAM Test] Step 1: Keep Creating Accounts until Database Guard is Triggered")
        ram_bytes_per_account = 512
        while clus.get_running_nodes() == clus.node_count and account_count < (capacity - guard) * 1024 * 1024 / ram_bytes_per_account:
            create_accounts(clus, 100)
            if account_count % 1000 == 0:
                clus.info(f"Account count: {account_count}")

        time.sleep(1.0)
        running_node_count = clus.get_running_nodes()
        bassert(running_node_count == 1,
                "Nodes 1, 2, 3 should all have been shut down. "
                f"Now there are {running_node_count} running nodes.")
        clus.info(f"{account_count} accounts created before nodes are shut down.")

        clus.info(f">>> Examine Log Files")
        target_str = "database chain::guard_exception"
        for i in range(1, clus.node_count):
            log = clus.get_log(node_id=i)
            if log.find(target_str):
                clus.info(f"\"{target_str}\" found in node {i}'s log")
            else:
                msg = f"Node {i}'s log does not contain {target_str}"
                clus.error(msg)
                raise BlockchainError(msg)

        clus.info(f">>> [RAM Test] Step 2: Restart Nodes with New Capacity")
        for _ in range(10):
            capacity += 4
            clus.info(f"db-guard-size: {guard} MB")
            clus.info(f"db-size: {capacity} MB")
            for i in range(1, clus.node_count):
                clus.terminate_node(node_id=i)
            time.sleep(1)
            for i in range(1, clus.node_count):
                clus.start_node(node_id=i, extra_args=f"--chain-state-db-guard-size-mb {guard} --chain-state-db-size-mb {capacity}")
            if clus.check_sync(retry=10, dont_raise=True).in_sync:
                break
            clus.info(f"Cannot restart with capacity={capacity} MB")
        else:
            msg = f"Nodes cannot be restarted with guard={guard}mb, capacity={capacity}mb"
            clus.error(msg)
            raise BlockchainError(msg)

        clus.info(f">>> [RAM Test] Step 3: Keep Creating Accounts until Database Guard is Triggered")
        while clus.get_running_nodes() == clus.node_count and account_count < ((capacity - guard) * 1024 * 1024 / ram_bytes_per_account):
            create_accounts(clus, 100)
            if account_count % 1000 == 0:
                clus.info(f"Account count: {account_count}")


        bassert(clus.get_running_nodes() < clus.node_count, "at least one node should have shut down")
        found = False
        for i in range(1, clus.node_count):
            log = clus.get_log(node_id=i)
            if log.find(target_str):
                clus.info("node %d's log contains \"%s\"" % (i, target_str))
                found = True
        bassert(found, "Cannot find chain::guard_exception in any node's log")


        clus.info(f">>> [RAM Test] Step 4: Restart with More Larger Capacity")
        capacity += 100
        clus.info(f"db-guard-size: {guard} MB")
        clus.info(f"db-size: {capacity} MB")
        for i in range(1, clus.node_count):
            clus.terminate_node(node_id=i)
        time.sleep(1)
        for i in range(1, clus.node_count):
            clus.start_node(node_id=i, extra_args=f"--chain-state-db-guard-size-mb {guard} --chain-state-db-size-mb {capacity}")

        clus.info(f">>> Check Sync")
        clus.check_sync()

        clus.info(f">>> [RAM Test] Step 5: Create 100 More Accounts")
        create_accounts(clus, 100)
        time.sleep(1)
        clus.info(f">>> Check Sync")
        clus.check_sync()
        clus.info(">>> [RAM Test] --- END -------------------------------------------------------------------")


if __name__ == "__main__":
    main()
