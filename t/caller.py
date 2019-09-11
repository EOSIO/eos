#! /usr/bin/env python3

import argparse
import json
import os
import platform
import requests
import shlex
import string
import subprocess
import time

from helper import fetch, get_transaction_id
from printer import Print, String, pad
from typing import List, Optional, Union

class LauncherCaller:

    # ----- initialize --------------------------------------------------------

    DEFAULT_ADDRESS = "127.0.0.1"
    DEFAULT_PORT = 1234
    DEFAULT_DIR = "../build"
    DEFAULT_FILE = "./programs/eosio-launcher-service/eosio-launcher-service"
    DEFAULT_START = False
    DEFAULT_KILL = False
    DEFAULT_CLUSTER_ID = 0
    DEFAULT_TOPOLOGY = "mesh"
    DEFAULT_VERBOSITY = 1
    DEFAULT_TIMEOUT = 1
    DEFAULT_MONOCHROME = True
    PROGRAM = "eosio-launcher-service"

    def __init__(self):
        # parse command-line arguments and set attributes
        self.args = self.parse_args()
        self.address = self.override(self.DEFAULT_ADDRESS, self.args.address)
        self.port = self.override(self.DEFAULT_PORT, self.args.port)
        self.dir = self.override(self.DEFAULT_DIR, self.args.dir)
        self.file = self.override(self.DEFAULT_FILE, self.args.file)
        self.start = self.override(self.DEFAULT_START, self.args.start)
        self.kill = self.override(self.DEFAULT_KILL, self.args.kill)
        self.cluster_id = self.override(self.DEFAULT_CLUSTER_ID, self.args.cluster_id)
        self.topology = self.override(self.DEFAULT_TOPOLOGY, self.args.topology)
        self.verbosity = self.override(self.DEFAULT_VERBOSITY, self.args.verbosity)
        self.monochrome = self.override(self.DEFAULT_MONOCHROME, self.args.monochrome)
        self.timeout = self.override(self.DEFAULT_TIMEOUT, self.args.timeout)

        # decide whether to connect to a remote or local launcher service
        if self.address in ("127.0.0.1", "localhost"):
            self.remote = False
        else:
            self.remote = True
            self.file = self.start = self.kill = None

        # regsiter printer
        self.print = Print(invisible=not self.verbosity, monochrome=self.monochrome)
        self.string = String(invisible=not self.verbosity, monochrome=self.monochrome)
        self.alert = String(monochrome=self.monochrome)
        if self.verbosity >= 3:
            self.print.response = lambda resp: self.print.response_in_full(resp)
        elif self.verbosity == 2:
            self.print.response = lambda resp: self.print.response_in_interaction(resp, timeout=self.timeout)
        elif self.verbosity == 1:
            self.print.response = lambda resp: self.print.response_in_short(resp)
        else:
            self.print.response = lambda resp: None

        # print system info
        self.print_system_info()

        # print configuration
        self.print_config()

        # change working directory
        self.describe("change working directory")
        os.chdir(self.dir)
        self.print.vanilla("{:50}{}".format("Current working directory", os.getcwd()))

        # connect to remote service and return
        if self.remote:
            self.connect_to_remote_service()
            return

        # connect to launcher service
        self.describe("connect to launcher service")
        spid = self.get_service_pid()
        if self.kill:
            self.kill_service(spid)
            spid.clear()
        if spid and not self.start:
            self.connect_to_local_service(spid[0])
        else:
            self.start_service()

    def parse_args(self):
        header = lambda text: String().decorate(text, style="underline", fcolor="green")
        parser = argparse.ArgumentParser(description=header("Launcher Service for EOS Testing Framework"), add_help=False,
                                         formatter_class=lambda prog: argparse.RawTextHelpFormatter(prog, max_help_position=50))
        couple = parser.add_mutually_exclusive_group()
        offset = 5
        helper = lambda text, value: "{} ({})".format(pad(text, left=offset, total=50, char=' ', sep=""), value)
        parser.add_argument("-h", "--help", action="help", help=' ' * offset + "Show this message and exit")
        parser.add_argument("-a", "--address", type=str, metavar="IP", help=helper("Address of launcher service", self.DEFAULT_ADDRESS))
        parser.add_argument("-p", "--port", type=int, help=helper("Listening port of launcher service", self.DEFAULT_PORT))
        parser.add_argument("-d", "--dir", type=str, help=helper("Working directory", self.DEFAULT_DIR))
        parser.add_argument("-f", "--file", type=str, help=helper("Path to local launcher service file", self.DEFAULT_FILE))
        parser.add_argument("-s", "--start", action="store_true", default=None, help=helper("Always start a new launcher service", self.DEFAULT_START))
        parser.add_argument("-k", "--kill", action="store_true", default=None, help=helper("Kill existing launcher services (if any)", self.DEFAULT_KILL))
        parser.add_argument("-i", "--cluster-id", dest="cluster_id", metavar="ID", type=int, help=helper("Cluster ID to launch with", self.DEFAULT_CLUSTER_ID))
        parser.add_argument("-t", "--topology", type=str, metavar="SHAPE", help=helper("Cluster topology to launch with", self.DEFAULT_TOPOLOGY), choices={"mesh", "star", "bridge", "line", "ring", "tree"})
        couple.add_argument("-v", "--verbose", dest="verbosity", action="count", default=None, help=helper("Verbosity level (-v for 1, -vv for 2, ...)", self.DEFAULT_VERBOSITY))
        couple.add_argument("-x", "--silent", dest="verbosity", action="store_false", default=None, help=helper("Set verbosity level at 0 (keep silent)", "False"))
        parser.add_argument("-c", "--color", dest="monochrome", action="store_false", default=None, help=helper("Print in colors", not self.DEFAULT_MONOCHROME))
        parser.add_argument("--timeout", type=float, metavar="TIME", default=None, help=helper("Pause time for interactive printing", self.DEFAULT_TIMEOUT))
        return parser.parse_args()

    def print_system_info(self):
        self.describe("system info")
        self.print.vanilla("{:50s}{}".format("UTC Time", time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())))
        self.print.vanilla("{:50s}{}".format("Local Time", time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())))
        self.print.vanilla("{:50s}{}".format("Platform", platform.platform()))

    def print_config(self):
        self.describe("configuration")
        helper = lambda arg, val: str(arg) + " --> N/A" if val is None else "None --> " + str(val) if arg is None else str(val)
        self.print.vanilla("{:50s}{}".format("Address of launcher service", helper(self.args.address, self.address)))
        self.print.vanilla("{:50s}{}".format("Listening port of launcher service", helper(self.args.port, self.port)))
        self.print.vanilla("{:50s}{}".format("Working directory", helper(self.args.dir, self.dir)))
        self.print.vanilla("{:50s}{}".format("Path to local launcher service file", helper(self.args.file, self.file)))
        self.print.vanilla("{:50s}{}".format("Always start a new launcher service", helper(self.args.start, self.start)))
        self.print.vanilla("{:50s}{}".format("Kill existing launcher services (if any)", helper(self.args.kill, self.kill)))
        self.print.vanilla("{:50s}{}".format("Cluster ID to launch with", helper(self.args.cluster_id, self.cluster_id)))
        self.print.vanilla("{:50s}{}".format("Cluster topology to launch with", helper(self.args.topology, self.topology)))
        self.print.vanilla("{:50s}{}".format("Verbosity level", helper(self.args.verbosity, self.verbosity)))
        self.print.vanilla("{:50s}{}".format("Print in colors", helper(self.args.monochrome, self.monochrome)))
        self.print.vanilla("{:50s}{}".format("Pause time for interactive printing", helper(self.args.timeout, self.timeout)))

    # TODO
    def connect_to_remote_service(self):
        pass

    def connect_to_local_service(self, pid):
        self.port = self.get_service_port(pid)
        self.file = self.get_service_file(pid)
        self.print.green("Connecting to existing launcher service with process ID [{}].".format(pid))
        self.print.vanilla("Configuration of existing launcher service:")
        self.print.vanilla("--- Listening port: [{}]".format(self.string.yellow(str(self.port))))
        self.print.vanilla("--- Path to file: {}".format(self.string.vanilla(self.file)))
        if self.args.port and self.args.port != self.port:
            self.print.yellow("Warning: Command-line argument -p/--port {} is ignored.".format(self.args.port))
        if self.args.file and self.args.file != self.file:
            self.print.yellow("Warning: Command-line argument -f/--file {} is ignored.".format(self.args.file))
        self.print.vanilla("To always start a new launcher service, pass {} or {}.".format(self.string.yellow("-s"), self.string.yellow("--start")))
        self.print.vanilla("To kill existing launcher services, pass {} or {}.".format(self.string.yellow("-k"), self.string.yellow("--kill")))

    def start_service(self):
        self.print.green("Starting a new launcher service.")
        subprocess.Popen([self.file, "--http-server-address=0.0.0.0:{}".format(self.port), "--http-threads=4"])
        assert self.get_service_pid(), self.alert.red("Launcher service is not started properly.")

    def kill_service(self, spid=None):
        spid = self.get_service_pid() if spid is None else spid
        for x in spid:
            self.print.yellow("Killing exisiting launcher service with process ID [{}].".format(x))
            subprocess.run(["kill", "-SIGTERM", str(x)])

    def get_service_pid(self) -> List[int]:
        """Returns a list of 0, 1, or more process IDs"""
        spid = self.pgrep(self.PROGRAM)
        if len(spid) == 0:
            self.print.yellow("No launcher is running currently.")
        elif len(spid) == 1:
            self.print.green("Launcher service is running with process ID [{}].".format(spid[0]))
        else:
            self.print.green("Multiple launcher services are running with process IDs {}".format(spid))
        return spid

    def get_service_port(self, pid):
        res = subprocess.Popen(["ps", "-p", str(pid), "-o", "command="], stdout=subprocess.PIPE).stdout.read().decode("ascii")
        shlex.split(res)
        for x in shlex.split(res):
            if x.startswith("--http-server-address"):
                return int(x.split(':')[-1])
        assert False, self.alert.red("Failed to get --http-server-address from process ID {}!".format(pid))

    def get_service_file(self, pid):
        return subprocess.Popen(["ps", "-p", str(pid), "-o", "comm="], stdout=subprocess.PIPE).stdout.read().rstrip().decode("ascii")


    # ----- RPC ---------------------------------------------------------------


    def rpc(self, url: str, data: str):
        return requests.post(url, data=data)

    def get_endpoint_url(self, func: str) -> str:
        return "http://{}:{}/v1/launcher/{}".format(self.address, self.port, func)

    def call(self, endpoint: str, data: dict, text: str, pause=0, retry=2):
        self.describe(text, pause=pause)
        self.request_url = self.get_endpoint_url(endpoint)
        self.request_data = json.dumps(data)
        self.print.vanilla(self.request_url)
        self.print.json(self.request_data, func=self.print.vanilla)
        resp = self.rpc(self.request_url, self.request_data)
        while not resp.ok and retry > 0:
            self.print.red(resp)
            self.print.vanilla("Retrying ...")
            time.sleep(1)
            resp = self.rpc(self.request_url, self.request_data)
            retry -= 1
            if retry == 0:
                break
        self.print.response(resp)


    def launch_cluster(self, data: dict):
        self.call("launch_cluster", data, "launch cluster")
        # TODO: sleep until get info is successful

    def get_cluster_info(self, data: dict):
        self.call("get_cluster_info", data, "get cluster info")

    def create_bios_accounts(self, data: dict):
        self.call("create_bios_accounts", data, "create bios accounts")

    def schedule_protocol_feature_activations(self, data: dict):
        self.call("schedule_protocol_feature_activations", data, "schedule protocol feature activations")

    def set_contract(self, data: dict, name: str):
        self.call("set_contract", data, "set <{}> contract".format(name))

    def push_actions(self, data: dict, text: str):
        self.call("push_actions", data, text)

    def create_account(self, data: dict, name: str):
        self.call("create_account", data, "create \"{}\" account".format(name))

    def stop_node(self, data: dict, text: str):
        self.call("stop_node", data, text)

    def terminate_node(self, data: dict):
        data.update(signal_id=15)
        self.stop_node(data, "terminate node #{}".format(data["node_id"]))

    def kill_node(self, data: dict):
        data.update(signal_id=9)
        self.stop_node(data, "kill node #{}".format(data["node_id"]))

    def stop_all_clusters(self):
        self.call("stop_all_clusters", dict(), "stop all clusters")

    # ----- Misc --------------------------------------------------------------

    # def get_account(self, data):
    #     self.response = self.rpc("get_account", data)

    # def get_log_data(self, data):
    #     self.response = self.rpc("get_log_data", data)

    # def get_protocol_features(self, data):
    #     self.response = self.rpc("get_protocol_features", data)

    # def get_cluster_running_state(self, data):
    #     self.response = self.rpc("get_cluster_running_state", data)

    # def verify_transaction(self, data: str):
    #     self.call("verify_transaction", data, "verify transaction id <{}>".format(data["transaction_id"]))

    # ----- Bootstrap ---------------------------------------------------------

    def boostrap(self,
             cluster_id=None,
             total_nodes=1,
             producer_nodes=1,
             unstarted_nodes=0,
             per_node_producers=1,
             total_producers=None,
             topology=None,
             dont_boostrap=False,
             only_bios=False,
             only_set_producers=False,
             common_extra_args=None,
             specific_extra_args=None):
        """
        Parameters
        ----------
        cluster_id          cluster ID
        total_nodes         total number of nodes
        ...

        Bootstrap
        ---------
        1. launch a cluster
        2. get cluster info
        3. create bios account
        4. schedule protocol feature activations
        5. set eosio.token contract
        6. create tokens
        7. issue tokens
        8. set system contract
        9. init system contract
        10. create producer accounts
        11. register producers
        12. vote for producers
        13. verify head producer
        """
        cluster_id = self.override(self.cluster_id, cluster_id)
        topology = self.override(self.topology, topology)

        total_producers = total_producers if total_producers else per_node_producers * producer_nodes

        # TODO: more assertions
        assert cluster_id >= 0, \
               self.alert.red("Cluster ID ({}) must be non-negative.".format(cluster_id))
        assert total_nodes >= producer_nodes + unstarted_nodes, \
               self.alert.red("total_nodes ({}) must be greater than or equal to producer_nodes ({}) + unstarted_nodes ({})."
                              .format(total_nodes, producer_nodes, unstarted_nodes))
        assert per_node_producers * (producer_nodes - 1) < total_producers <= per_node_producers * producer_nodes, \
               self.alert.red("Incompatible producers configuration with per_node_producers ({}), producer_nodes ({}), and total_producers ({})."
                              .format(per_node_producers, producer_nodes, total_producers))
        assert total_producers <= 26, \
               self.alert.red("Trying to have {} producers. More than names defproducera ... defproducerz can accommodate."
                              .format(total_producers))

         # launch cluster
        info = {}
        info["cluster_id"] = cluster_id
        info["node_count"] = total_nodes
        info["shape"] = topology
        info["nodes"] = []
        for i in range(total_nodes):
            info["nodes"] += [{"node_id": i}]
            if i < producer_nodes:
                names = [] if i else ["eosio"]
                for j in range(i * per_node_producers, min((i+1) * per_node_producers, total_producers)):
                    names += ["defproducer" + string.ascii_lowercase[j]]
                info["nodes"][i]["producers"] = names

        # launch a cluster
        self.launch_cluster(fetch(info, ["cluster_id", "node_count", "shape", "nodes"]))

        # get cluster info: assert success
        self.get_cluster_info(fetch(info, ["cluster_id"]))

        # # create system accounts
        info["creator"] = "eosio"
        info["accounts"] = [{"name":"eosio.bpay"},
                            {"name":"eosio.msig"},
                            {"name":"eosio.names"},
                            {"name":"eosio.ram"},
                            {"name":"eosio.ramfee"},
                            {"name":"eosio.rex"},
                            {"name":"eosio.saving"},
                            {"name":"eosio.stake"},
                            {"name":"eosio.token"},
                            {"name":"eosio.upay"}]
        self.create_bios_accounts(fetch(info, ["cluster_id", "creator", "accounts"]))

        # verify transaction

        # schedule protocol feature activations
        info["protocol_features_to_activate"] = ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]
        info["node_id"] = 0
        self.schedule_protocol_feature_activations(fetch(info, ["cluster_id", "node_id", "protocol_features_to_activate"]))

        # # set eosio.token
        info["account"] = "eosio.token"
        info["contract_file"] = "../../contracts/build/contracts/eosio.token/eosio.token.wasm"  # hardcoded, to be changed later
        info["abi_file"] = "../../contracts/build/contracts/eosio.token/eosio.token.abi"        # hardcoded, to be changed later
        self.set_contract(fetch(info, ["cluster_id", "node_id", "account", "contract_file", "abi_file"]), "eosio.token")

        # create tokens
        info["actions"] = [{"account": "eosio.token",
                            "action": "create",
                            "permissions": [{"actor": "eosio.token",
                                             "permission": "active"}],
                            "data": {"issuer": "eosio",
                                    "maximum_supply": "1000000000.0000 SYS",
                                    "can_freeze": 0,
                                    "can_recall": 0,
                                    "can_whitelist":0}}]
        self.push_actions(fetch(info, ["cluster_id", "node_id", "actions"]), "create tokens")

        info["actions"] = [{"account": "eosio.token",
                            "action": "issue",
                            "permissions": [{"actor": "eosio",
                                             "permission": "active"}],
                            "data": {"to": "eosio",
                                     "quantity": "1000000000.0000 SYS",
                                     "memo": "hi"}}]
        self.push_actions(fetch(info, ["cluster_id", "node_id", "actions"]), "issue tokens")

        info["account"] = "eosio"
        info["contract_file"] = "../../contracts/build/contracts/eosio.system/eosio.system.wasm"  # hardcoded, to be changed later
        info["abi_file"] = "../../contracts/build/contracts/eosio.system/eosio.system.abi"        # hardcoded, to be changed later
        self.set_contract(fetch(info, ["cluster_id", "node_id", "account", "contract_file", "abi_file"]), "eosio.system")

        info["actions"] = [{"account": "eosio",
                            "action": "init",
                            "permissions": [{"actor": "eosio",
                                             "permission": "active"}],
                            "data": {"version": 0,
                                     "core": "4,SYS"}}]
        self.push_actions(fetch(info, ["cluster_id", "node_id", "actions"]), "init system contract")

        # create producer accounts
        # TODO: make iteration through producers more efficient
        info["stake_cpu"] = "75000000.0000 SYS"
        info["stake_net"] = "75000000.0000 SYS"
        info["buy_ram_bytes"] = 1048576
        info["transfer"] = True
        producers_list = []
        for i in range(producer_nodes):
            for p in info["nodes"][i]["producers"]:
                if p != "eosio":
                    producers_list.append(p)
                    info["node_id"] = i
                    info["name"] = p
                    self.create_account(fetch(info, ["cluster_id", "node_id", "creator", "name", "stake_cpu", "stake_net", "buy_ram_bytes", "transfer"]), p)
                    info["actions"] = [{"account": "eosio",
                                        "action": "regproducer",
                                        "permissions": [{"actor": "{}".format(p),
                                                        "permission": "active"}],
                                        "data": {"producer": "{}".format(p),
                                                 "producer_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                                                 "url": "www.test.com",
                                                 "location": 0}}]
                    self.push_actions(fetch(info, ["cluster_id", "node_id", "actions"]), "register \"{}\" account".format(p))

        # vote for producers
        info["node_id"] = 0
        info["actions"] = [{"account": "eosio",
                            "action": "voteproducer",
                            "permissions": [{"actor": "defproducera",
                                             "permission": "active"}],
                            "data": {"voter": "defproducera",
                                     "proxy": "",
                                     "producers": producers_list}}]
        self.push_actions(fetch(info, ["cluster_id", "node_id", "actions"]), "vote for producers")
        print(' ' * 100)

    # ---------- Utilities ----------------------------------------------------

    @staticmethod
    def override(default_value, value):
        return default_value if value is None else value

    @staticmethod
    def pgrep(pattern: str) -> List[int]:
        out = subprocess.Popen(['pgrep', pattern], stdout=subprocess.PIPE).stdout.read()
        return [int(x) for x in out.splitlines()]

    def describe(self, text, pause=0):
        self.print.vanilla(pad(self.string.decorate(text, fcolor="black", bcolor="cyan")))
        time.sleep(pause)


def main():
    caller = LauncherCaller()
    caller.print.white(">>> Bootstrapping ...")
    caller.boostrap(producer_nodes=3, unstarted_nodes=2, total_nodes=6, total_producers=5, per_node_producers=2)

if __name__ == '__main__':
    main()
