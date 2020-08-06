#! /usr/bin/env python3

"""core.debugger - Hassle-free debugger for interactive Python environment

Just "from core.debugger import *" (or "from debugger import *" if in already
core) to dump all the functions and use them to query the current status of
launcher service and/or clusters.
"""

# standard libraries
import base64
import subprocess
import typing
# user-defined modules
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

# -------------------- call -------------------------------------------------------------------------------------------

def launch_cluster(cluster_id, node_count, shape="mesh", center_node_id=None, extra_configs=[], extra_args=""):
    return call("launch_cluster", cluster_id=cluster_id, node_count=node_count, shape=shape, center_node_id=center_node_id,
                extra_configs=extra_configs, extra_args=extra_args)


def stop_cluster(cluster_id):
    return call("stop_cluster", cluster_id=cluster_id)


def clean_cluster(cluster_id):
    return call("clean_cluster", cluster_id=cluster_id)


def start_node(cluster_id, node_id, extra_args=""):
    return call("stop_node", cluster_id=cluster_id, node_id=node_id, extra_args=extra_args)


def stop_node(cluster_id, node_id, kill_sig=15):
    return call("stop_node", cluster_id=cluster_id, node_id=node_id, kill_sig=kill_sig)


def get_cluster_running_state(cluster_id):
    return call("get_cluster_running_state", cluster_id=cluster_id)


def get_cluster_info(cluster_id):
    return call("get_cluster_info", cluster_id=cluster_id)


def get_info(cluster_id, node_id):
    return call("get_info", cluster_id=cluster_id, node_id=node_id)


def get_block(cluster_id, node_id, block_num_or_id):
    return call("get_block", cluster_id=cluster_id, node_id=node_id, block_num_or_id=block_num_or_id)


def get_account(cluster_id, node_id, name):
    return call("get_account", cluster_id=cluster_id, node_id=node_id, name=name)


def get_protocol_features(cluster_id, node_id=0):
    return call("get_protocol_features", cluster_id=cluster_id, node_id=node_id)


def get_log_data(cluster_id, node_id, offset, length=10000, filename="stderr_0.txt", make_return=False):
    data = base64.b64decode(call("get_log_data", cluster_id=cluster_id, node_id=node_id, offset=offset, length=length, filename=filename).response_dict["data"]).decode("utf-8")
    print(data)
    if make_return:
        return data


def verify_transaction(transaction_id, cluster_id, node_id=0, verify_key="irreversible"):
    try:
        return call("verify_transaction", cluster_id=cluster_id, node_id=node_id, transaction_id=transaction_id).response_dict[verify_key]
    except KeyError:
        return False


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


def call(api: str, port=1234, **data) -> Connection:
    cx = Connection(url=f"http://127.0.0.1:{port}/v1/launcher/{api}", data=data)
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

# -------------------- raw --------------------------------------------------------------------------------------------

def get_greylist(cluster_id, node_id=0):
    return raw(url="/v1/producer/get_greylist", cluster_id=cluster_id, node_id=node_id)


def get_net_plugin_connections(cluster_id, node_id=0):
    return raw(url="/v1/net/connections", cluster_id=cluster_id, node_id=node_id)


def raw(url, cluster_id, node_id=0, string_data: str="", json_data: dict={}):
    return call("send_raw", url=url, cluster_id=cluster_id, node_id=node_id, string_data=string_data, json_data=json_data)

# -------------------- system -----------------------------------------------------------------------------------------

def get_pid_list_by_pattern(pattern: str) -> typing.List[int]:
    out = run(["pgrep", "-f", pattern])
    return [int(x) for x in out]


def get_cmd_and_args_by_pid(pid: typing.Union[int, str]):
    return run(["ps", "-p", str(pid), "-o", "command="])


def is_launcher_service_running():
    pid_list = get_pid_list_by_pattern("launcher-service")
    print(pid_list)
    return bool(len(pid_list))


def ps_launcher_service():
    pid_list = get_pid_list_by_pattern("launcher-service")
    for pid in pid_list:
        print(f"[{pid}] {get_cmd_and_args_by_pid(pid)[0]}")


def run(args: typing.Union[str, typing.List[str]]):
    if isinstance(args, str):
        args = args.split(" ")
    return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True).stdout.splitlines()
