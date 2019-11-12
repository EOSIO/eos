#! /usr/bin/env python3

import typing

if __package__:
    from .logger import LogLevel, Logger, ScreenWriter, FileWriter
    from .connection import Connection
    from . import color
    from . import helper
else:
    from logger import LogLevel, Logger, ScreenWriter, FileWriter
    from connection import Connection
    import color
    import helper

"""
====================
Launcher Service API
====================
@ plugins/launcher_service_plugin/include/eosio/launcher_service_plugin/launcher_service_plugin.hpp

-- cluster-related calls ----------------
1. launch_cluster
2. stop_cluster
3. stop_all_clusters
4. clean_cluster
5. start_node
6. stop_node

--- wallet-related calls ----------------
7. generate_key
8. import_keys

--- queries -----------------------------
9. get_cluster_info
10. get_cluster_running_state
11. get_info
12. get_block
13. get_account
14. get_code_hash
15. get_protocol_features
16. get_table_rows
17. get_log_data
18. verify_transaction

--- transactions ------------------------
19. schedule_protocol_feature_activations
20. set_contract
21. push_actions

--- miscellaneous------------------------
22. send_raw
"""


# def launch_cluster(cluster_id, nodes, node_count, shape="mesh", center_node_id=None, extra_configs=[], extra_args=""):
#     return _call("launch_cluster",
#                  cluster_id=cluster_id,
#                  nodes=nodes,
#                  node_count=node_count,
#                  shape=shape,
#                  center_node_id=center_node_id,
#                  extra_configs=extra_configs,
#                  extra_args=extra_args)


def stop_cluster(cluster_id):
    return _call("stop_cluster", cluster_id=cluster_id)


def stop_node(self, node_id, kill_sig=15):
    return _call("stop_node", node_id=node_id, kill_sig=kill_sig)


def stop_all_nodes(self, kill_sig=15):
    for node_id in self.node_count:
        return _call("stop_nodes", node_id=node_id, kill_sig=kill_sig)


def get_cluster_info(cluster_id):
    return _call("get_cluster_info", cluster_id=cluster_id)


def get_cluster_running_state(cluster_id):
    return _call("get_cluster_running_state", cluster_id=cluster_id)


def get_info(cluster_id, node_id):
    return _call("get_cluster_info", cluster_id=cluster_id, node_id=node_id)


def get_block(cluster_id, node_id, block_num_or_id):
    return _call("get_block", cluster_id=cluster_id, node_id=node_id, block_num_or_id=block_num_or_id)


def get_account(cluster_id, node_id, name):
    return _call("get_account", cluster_id=cluster_id, node_id=node_id, name=name)


def get_node_pid(cluster_id, node_id):
    return get_cluster_running_state(cluster_id).response_dict["result"]["nodes"][node_id][1]["pid"]


def is_node_down(cluster_id, node_id):
    return get_node_pid(cluster_id, node_id) == 0


def is_node_ready(cluster_id, node_id):
    return "error" not in get_cluster_info(cluster_id).response_dict["result"][node_id][1]


def get_head_block_id(cluster_id, node_id=0):
    return get_cluster_info(cluster_id).response_dict["result"][node_id][1]["head_block_id"]


def get_head_block_time(cluster_id, node_id=0):
    return get_cluster_info(cluster_id).response_dict["result"][node_id][1]["head_block_time"]


def get_head_block_producer(cluster_id, node_id=0):
    return get_cluster_info(cluster_id).response_dict["result"][node_id][1]["head_block_producer"]


def get_head_block_num(cluster_id, node_id=0):
    return get_cluster_info(cluster_id).response_dict["result"][node_id][1]["head_block_num"]


def get_last_irreversible_block_num(cluster_id, node_id=0):
    return get_cluster_info(cluster_id).response_dict["result"][node_id][1]["last_irreversible_block_num"]


def get_process_by_pid(pid: typing.Union[int, str]):
    return helper.interacitve_run(["ps", "-p", str(pid), "-o", "command="])


def _call(api: str, **data) -> Connection:
    cx = Connection(url=f"http://127.0.0.1:1234/v1/launcher/{api}", data=data)
    print(cx.url)
    print(cx.request_text)
    if cx.response.ok:
        print(cx.response_code)
        if cx.transaction_id:
            print(color.green(f"<Transaction ID> {cx.transaction_id}"))
        else:
            print(color.yellow("<No Transaction ID>"))
        print(cx.response_text)
    else:
        print(cx.response_code)
        print(cx.response_text)
    return cx


def _interacitve_run(args: list):
    return subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True).stdout.read().rstrip()
