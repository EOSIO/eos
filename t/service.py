#! /usr/bin/env python3

# standard libraries
from typing import List, Optional, Union
import argparse
import helper
import json
import math
import os
import platform
import shlex
import string
import subprocess
import threading
import time

# third-party libraries
import requests

# user-defined modules
from logger import WriterConfig, ScreenWriter, FileWriter, Logger
from connector import Connection as Interaction
import color

# user-defined modules TO BE DEPRECATED
import printer


PROGRAM = "eosio-launcher-service"

DEFAULT_ADDRESS = "127.0.0.1"
DEFAULT_PORT = 1234
DEFAULT_DIR = "../build"
DEFAULT_FILE = os.path.join(".", "programs", PROGRAM, PROGRAM)
DEFAULT_START = False
DEFAULT_KILL = False
DEFAULT_VERBOSITY = 1
DEFAULT_MONOCHROME = False
DEFAULT_DEBUG = False
DEFAULT_INFO = False

DEFAULT_CLUSTER_ID = 0
DEFAULT_CENTER_NODE_ID = None
DEFAULT_TOPOLOGY = "mesh"
DEFAULT_TOTAL_NODES = 4
DEFAULT_TOTAL_PRODUCERS = 4
DEFAULT_PRODUCER_NODES = 4
DEFAULT_UNSTARTED_NODES = 0

HELP_HELP = "Show this message and exit"
HELP_ADDRESS = "Address of launcher service"
HELP_PORT = "Listening port of launcher service"
HELP_DIR = "Working directory"
HELP_FILE = "Path to local launcher service file"
HELP_START = "Always start a new launcher service"
HELP_KILL = "Kill existing launcher services (if any)"
HELP_VERBOSE = "Verbosity level (-v for 1, -vv for 2, ...)"
HELP_SILENT = "Set verbosity level at 0 (keep silent)"
HELP_MONOCHROME = "Print in black and white instead of colors"
HELP_DEBUG = "Enable debug_print() and info_print()"
HELP_INFO = "Enable info_print()"

HELP_CLUSTER_ID = "Cluster ID to launch with"
HELP_CENTER_NODE_ID = "Center node ID for star or bridge"
HELP_TOPOLOGY = "Cluster topology to launch with"
HELP_TOTAL_NODES = "Number of total nodes"
HELP_TOTAL_PRODUCERS = "Number of total producers"
HELP_PRODUCER_NODES = "Number of nodes that have producers"
HELP_UNSTARTED_NODES = "Number of unstarted nodes"


class ExceptionThread(threading.Thread):
    id = 0

    def __init__(self, channel, communicate, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.id = ExceptionThread.id
        self.channel = channel
        self.communicate = communicate
        ExceptionThread.id = ExceptionThread.id + 1

    def run(self):
        try:
            super().run()
        except Exception:
            self.communicate(self.channel, self.id)


class CommandLineArguments:
    def __init__(self):
        cla = self.parse()
        self.address = cla.address
        self.port = cla.port
        self.dir = cla.dir
        self.file = cla.file
        self.start = cla.start
        self.kill = cla.kill
        self.verbosity = cla.verbosity
        self.monochrome = cla.monochrome
        self.debug = cla.debug
        self.info = cla.info

        self.cluster_id = cla.cluster_id
        self.topology = cla.topology
        self.center_node_id = cla.center_node_id
        self.total_nodes = cla.total_nodes
        self.total_producers = cla.total_producers
        self.producer_nodes = cla.producer_nodes
        self.unstarted_nodes = cla.unstarted_nodes


    @staticmethod
    def parse():
        HEADER = printer.String().decorate("Launcher Service for EOS Testing Framework", style="underline", fcolor="green")
        OFFSET = 5

        parser = argparse.ArgumentParser(description=HEADER, add_help=False, formatter_class=lambda prog: argparse.RawTextHelpFormatter(prog, max_help_position=50))

        parser.add_argument("-h", "--help", action="help", help=' ' * OFFSET + HELP_HELP)

        info = lambda text, value: "{} ({})".format(helper.pad(text, left=OFFSET, total=50, char=' ', sep=""), value)
        parser.add_argument("-a", "--address", type=str, metavar="IP", help=info(HELP_ADDRESS, DEFAULT_ADDRESS))
        parser.add_argument("-p", "--port", type=int, help=info(HELP_PORT, DEFAULT_PORT))
        parser.add_argument("-d", "--dir", type=str, help=info(HELP_DIR, DEFAULT_DIR))
        parser.add_argument("-f", "--file", type=str, help=info(HELP_FILE, DEFAULT_FILE))
        parser.add_argument("-s", "--start", action="store_true", default=None, help=info(HELP_START, DEFAULT_START))
        parser.add_argument("-k", "--kill", action="store_true", default=None, help=info(HELP_KILL, DEFAULT_KILL))

        parser.add_argument("-i", "--cluster-id", dest="cluster_id", type=int, metavar="ID", help=info(HELP_CLUSTER_ID, DEFAULT_CLUSTER_ID))
        parser.add_argument("-t", "--topology", type=str, metavar="SHAPE", help=info(HELP_TOPOLOGY, DEFAULT_TOPOLOGY), choices={"mesh", "star", "bridge", "line", "ring", "tree"})
        parser.add_argument("-c", "--center-node-id", dest="center_node_id", type=int, metavar="ID", help=info(HELP_CENTER_NODE_ID, DEFAULT_CENTER_NODE_ID))
        parser.add_argument("-n", "--total-nodes", dest="total_nodes", type=int, metavar="NUM", help=info(HELP_TOTAL_NODES, DEFAULT_TOTAL_NODES))
        parser.add_argument("-y", "--total-producers", dest="total_producers", type=int, metavar="NUM", help=info(HELP_TOTAL_PRODUCERS, DEFAULT_TOTAL_PRODUCERS))
        parser.add_argument("-z", "--producer-nodes", dest="producer_nodes", type=int, metavar="NUM", help=info(HELP_PRODUCER_NODES, DEFAULT_PRODUCER_NODES))
        parser.add_argument("-u", "--unstarted-nodes", dest="unstarted_nodes", type=int, metavar="NUM", help=info(HELP_UNSTARTED_NODES, DEFAULT_UNSTARTED_NODES))

        verbosity = parser.add_mutually_exclusive_group()
        verbosity.add_argument("-v", "--verbose", dest="verbosity", action="count", default=None, help=info(HELP_VERBOSE, DEFAULT_VERBOSITY))
        verbosity.add_argument("-x", "--silent", dest="verbosity", action="store_false", default=None, help=info(HELP_SILENT, not DEFAULT_VERBOSITY))
        parser.add_argument("-m", "--monochrome", action="store_true", default=None, help=info(HELP_MONOCHROME, DEFAULT_MONOCHROME))
        parser.add_argument("--debug", action="store_true", default=None, help=info(HELP_DEBUG, DEFAULT_DEBUG))
        parser.add_argument("--info", action="store_true", default=None, help=info(HELP_INFO, DEFAULT_INFO))

        return parser.parse_args()




class Service:
    def __init__(self, address=None, port=None, dir=None, file=None, start=None, kill=None, verbosity=None, monochrome=None, debug=None, info=None, dont_connect=False, logger=None):

        # configure service
        self.cla        = CommandLineArguments()
        self.address    = helper.override(DEFAULT_ADDRESS,    address,    self.cla.address    if self.cla else None)
        self.port       = helper.override(DEFAULT_PORT,       port,       self.cla.port       if self.cla else None)
        self.dir        = helper.override(DEFAULT_DIR,        dir,        self.cla.dir        if self.cla else None)
        self.file       = helper.override(DEFAULT_FILE,       file,       self.cla.file       if self.cla else None)
        self.start      = helper.override(DEFAULT_START,      start,      self.cla.start      if self.cla else None)
        self.kill       = helper.override(DEFAULT_KILL,       kill,       self.cla.kill       if self.cla else None)
        self.verbosity  = helper.override(DEFAULT_VERBOSITY,  verbosity,  self.cla.verbosity  if self.cla else None)
        self.monochrome = helper.override(DEFAULT_MONOCHROME, monochrome, self.cla.monochrome if self.cla else None)
        self.debug      = helper.override(DEFAULT_DEBUG,      debug,      self.cla.debug      if self.cla else None)
        self.info       = helper.override(DEFAULT_INFO,       info,       self.cla.info       if self.cla else None)

        # determine remote or local launcher service to connect to
        if self.address in ("127.0.0.1", "localhost"):
            self.remote = False
        else:
            self.remote = True
            self.file = self.start = self.kill = None

        # register logger
        # TODO: set default behavior
        self.logger = logger
        self.log = logger.log
        self.trace = logger.trace
        self.debug = logger.debug
        self.info = logger.info
        self.warn = logger.warn
        self.error = logger.error
        self.fatal = logger.fatal


        # register printer
        # TO BE DEPRECATED
        self.print = printer.Print(invisible=not self.verbosity, monochrome=self.monochrome)
        self.string = printer.String(invisible=not self.verbosity, monochrome=self.monochrome)
        self.alert = printer.String(monochrome=self.monochrome)
        if self.verbosity > 2:
            self.print.response = lambda resp: self.print.response_in_full(resp)
        elif self.verbosity == 2:
            self.print.response = lambda resp: self.print.response_with_prompt(resp)
        elif self.verbosity == 1:
            self.print.response = lambda resp: self.print.response_in_short(resp)
        else:
            self.print.response = lambda resp: None
        self.string.offset = 0 if self.monochrome else 9

        if not dont_connect:
            self.connect()


    def connect(self):
        # print system info
        self.print_system_info()

        # print configuration
        self.print_config()

        # change working directory
        self.print_header("change working directory")
        os.chdir(self.dir)
        self.logger.debug("{:70}{}".format("Current working directory", os.getcwd()))
        self.logger.flush()

        # connect to remote service and return
        if self.remote:
            self.connect_to_remote_service()
            return

        # connect to launcher service
        self.print_header("connect to launcher service")
        spid = self.get_service_pid()
        if self.kill:
            self.kill_service(spid)
            spid.clear()
        if spid and not self.start:
            self.connect_to_local_service(spid[0])
        else:
            self.start_service()
        self.logger.flush()


    def print_header(self, text):
        # self.print.vanilla(helper.pad(self.string.decorate(text, fcolor="black", bcolor="cyan")))
        # TODO: change hard coded 80
        self.logger.debug(helper.header(text))


    def print_system_info(self):
        self.print_header("system info")
        # self.print.vanilla("{:70}{}".format("UTC Time", time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())))
        # self.print.vanilla("{:70}{}".format("Local Time", time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())))
        # self.print.vanilla("{:70}{}".format("Platform", platform.platform()))
        self.logger.debug("{:22}{}".format("UTC Time", time.strftime("%Y-%m-%d %H:%M:%S %Z", time.gmtime())))
        self.logger.debug("{:22}{}".format("Local Time", time.strftime("%Y-%m-%d %H:%M:%S %Z", time.localtime())))
        self.logger.debug("{:22}{}".format("Platform", platform.platform()))
        self.logger.flush()


    def print_config(self):
        self.print_header("service configuration")
        self.print_config_helper("-a: address",     HELP_ADDRESS,       self.address,       DEFAULT_ADDRESS)
        self.print_config_helper("-p: port",        HELP_PORT,          self.port,          DEFAULT_PORT)
        self.print_config_helper("-d: dir",         HELP_DIR,           self.dir,           DEFAULT_DIR)
        self.print_config_helper("-f: file",        HELP_FILE,          self.file,          DEFAULT_FILE)
        self.print_config_helper("-s: start",       HELP_START,         self.start,         DEFAULT_START)
        self.print_config_helper("-k: kill",        HELP_KILL,          self.kill,          DEFAULT_KILL)
        self.print_config_helper("-v: verbose",     HELP_VERBOSE,       self.verbosity,     DEFAULT_VERBOSITY)
        self.print_config_helper("-m: monochrome",  HELP_MONOCHROME,    self.monochrome,    DEFAULT_MONOCHROME)
        self.logger.flush()


    def print_config_helper(self, label, help, value, default_value, label_width=22, help_width=48):
        # self.print.vanilla("{:{label_offset_width}}{:{help_width}}{}"
        self.logger.debug("{:31}{:48}{}".format(color.yellow(label),
                                    help,
                                    helper.compress(str(value) if value == default_value else color.blue(value))))

    # TODO
    def connect_to_remote_service(self):
        pass


    def connect_to_local_service(self, pid):
        current_port = self.get_service_port(pid)
        current_file = self.get_service_file(pid)
        self.logger.debug(color.green("Connecting to existing launcher service with process ID [{}].".format(pid)))
        self.logger.debug(color.green("No new launcher service will be started."))
        self.logger.debug("Configuration of existing launcher service:")
        self.logger.debug("--- Listening port: [{}]".format(self.string.yellow(current_port)))
        self.logger.debug("--- Path to file: {}".format(self.string.vanilla(current_file)))
        if self.port != current_port:
            self.logger.debug(color.yellow("Warning: port setting (port = {}) ignored.".format(self.port)))
            self.port = current_port
        if self.file != current_file:
            self.logger.debug(color.yellow("Warning: file setting (file = {}) ignored.".format(self.file)))
            self.file = current_file
        self.logger.debug("To always start a new launcher service, pass {} or {}.".format(self.string.yellow("-s"), self.string.yellow("--start")))
        self.logger.debug("To kill existing launcher services, pass {} or {}.".format(self.string.yellow("-k"), self.string.yellow("--kill")))
        self.logger.flush()

    def start_service(self):
        self.logger.debug(color.green("Starting a new launcher service."))
        subprocess.Popen([self.file, "--http-server-address=0.0.0.0:{}".format(self.port), "--http-threads=4"])
        assert self.get_service_pid(), self.alert.red("Launcher service is not started properly.")


    def kill_service(self, spid=None):
        spid = self.get_service_pid() if spid is None else spid
        for x in spid:
            self.logger.debug(color.yellow("Killing exisiting launcher service with process ID [{}].".format(x)))
            subprocess.run(["kill", "-SIGTERM", str(x)])


    def get_service_pid(self) -> List[int]:
        """Returns a list of 0, 1, or more process IDs"""
        spid = helper.pgrep(PROGRAM)
        if len(spid) == 0:
            self.logger.debug(color.yellow("No launcher is running currently."))
        elif len(spid) == 1:
            self.logger.debug(color.green("Launcher service is running with process ID [{}].".format(spid[0])))
        else:
            self.logger.debug(color.green("Multiple launcher services are running with process IDs {}".format(spid)))
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


class Cluster:
    def __init__(self, service, cluster_id=None, topology=None, center_node_id=None, total_nodes=None, total_producers=None, producer_nodes=None, unstarted_nodes=None, dont_vote=False, dont_bootstrap=False):
        """
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

        # register service
        self.service = service
        self.cla = service.cla
        self.logger = service.logger
        self.print = service.print
        self.string = service.string
        self.alert = service.alert

        # TO BE DEPRECATED
        self.print_header = service.print_header
        self.print_config_helper = service.print_config_helper


        # configure cluster
        self.cluster_id      = helper.override(DEFAULT_CLUSTER_ID,      cluster_id,      self.cla.cluster_id      if self.cla else None)
        self.topology        = helper.override(DEFAULT_TOPOLOGY,        topology,        self.cla.topology        if self.cla else None)
        self.center_node_id  = helper.override(DEFAULT_CENTER_NODE_ID,  center_node_id,  self.cla.center_node_id  if self.cla else None)
        self.total_nodes     = helper.override(DEFAULT_TOTAL_NODES,     total_nodes,     self.cla.total_nodes     if self.cla else None)
        self.total_producers = helper.override(DEFAULT_TOTAL_PRODUCERS, total_producers, self.cla.total_producers if self.cla else None)
        self.producer_nodes  = helper.override(DEFAULT_PRODUCER_NODES,  producer_nodes,  self.cla.producer_nodes  if self.cla else None)
        self.unstarted_nodes = helper.override(DEFAULT_UNSTARTED_NODES, unstarted_nodes, self.cla.unstarted_nodes if self.cla else None)

        # reconcile conflict in config
        self.reconcile_config()

        # check for potential problems in config
        self.check_config()

        # establish connection between nodes and producers
        self.nodes = []
        self.producers = {}
        q, r = divmod(self.total_producers, self.producer_nodes)
        for i in range(self.total_nodes):
            self.nodes += [{"node_id": i}]
            if i < self.producer_nodes:
                prod = [] if i else ["eosio"]
                for j in range(i * q + r if i else 0, (i + 1) * q + r):
                    name = self.get_defproducer_names(j)
                    prod.append(name)
                    self.producers[name] = i
                self.nodes[i]["producers"] = prod

        if not dont_bootstrap:
            self.bootstrap(dont_vote=dont_vote)


    def reconcile_config(self):
        if self.producer_nodes > self.total_producers:
            self.print_header("resolve conflict in cluster configuration")
            self.print.vanilla("Conflict: total producers ({}) <= producer nodes ({}).".format(self.total_producers, self.producer_nodes))
            self.print.vanilla("Resolution: total_producers takes priority over producer_nodes.")
            self.logger.debug(color.yellow("Warning: producer nodes setting (producer_nodes = {}) ignored.".format(self.producer_nodes)))
            self.producer_nodes = self.total_producers


    def check_config(self):
        assert self.cluster_id >= 0
        assert self.total_nodes >= self.producer_nodes + self.unstarted_nodes, self.alert.red("Failed assertion: total_node ({}) >= producer_nodes ({}) + unstarted_nodes ({}).".format(self.total_nodes, self.producer_nodes, self.unstarted_nodes))
        if self.topology in ("star", "bridge"):
            assert self.center_node_id is not None, self.alert.red("Failed assertion: center_node_id is specified when topology is \"{}\".".format(self.topology))
        if self.topology == "bridge":
            assert self.center_node_id not in (0, self.total_nodes - 1), self.alert.red("Failed assertion: center_node_id ({}) is neither 0 nor last node ({}) when topology is \"bridge\".".format(self.center_node_id, self.total_nodes - 1))
            assert self.total_nodes >= 3, self.alert.red("Failed assertion: total_node ({}) >= 3 when topology is \"bridge\".".format(self.total_nodes))


    def bootstrap(self, dont_vote=False):
        """
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

        self.logger.info(">>> Bootstrap starts.")

        # print configuration
        self.print_config()

        # 1. launch a cluster
        self.launch_cluster()

        # 2. get cluster info
        self.get_cluster_info()

        # 3. create bios accounts
        self.create_bios_accounts()

        # 4. schedule protocol feature activations
        self.schedule_protocol_feature_activations()

        # 5. set eosio.token
        self.set_contract(node_id=0,
                          contract_file="../../contracts/build/contracts/eosio.token/eosio.token.wasm",
                          abi_file="../../contracts/build/contracts/eosio.token/eosio.token.abi",
                          account="eosio.token", name="eosio.token")

        # 6. create tokens
        total_supply = 1e9
        total_supply_formatted = "{:.4f} SYS".format(total_supply)

        create_tokens = [{"account": "eosio.token",
                          "action": "create",
                          "permissions": [{"actor": "eosio.token",
                                           "permission": "active"}],
                          "data": {"issuer": "eosio",
                                   "maximum_supply": total_supply_formatted,
                                   "can_freeze": 0,
                                   "can_recall": 0,
                                   "can_whitelist":0}}]
        self.push_actions(node_id=0, actions=create_tokens, name="create tokens")

        # 7. issue tokens
        issue_tokens = [{"account": "eosio.token",
                          "action": "issue",
                          "permissions": [{"actor": "eosio",
                                           "permission": "active"}],
                          "data": {"to": "eosio",
                                   "quantity": total_supply_formatted,
                                   "memo": "hi"}}]
        self.push_actions(node_id=0, actions=issue_tokens, name="issue tokens")

        # 8. set system contract
        self.set_contract(node_id=0,
                          contract_file="../../contracts/build/contracts/eosio.system/eosio.system.wasm",
                          abi_file="../../contracts/build/contracts/eosio.system/eosio.system.abi",
                          account="eosio", name="eosio.system")

        # 9. init system contract
        init_system_contract = [{"account": "eosio",
                                 "action": "init",
                                 "permissions": [{"actor": "eosio",
                                                  "permission": "active"}],
                                 "data": {"version": 0,
                                          "core": "4,SYS"}}]
        self.push_actions(node_id=0, actions=init_system_contract, name="init system contract")

        # 10. create producer accounts
        # 11. register producers
        stake_amount = total_supply * 0.075
        create_account_threads = []
        channel = []
        communicate = lambda channel, id: channel.append(id)

        for p in self.producers:
            def create_and_register(stake_amount):
                producer = p
                node_id = self.producers[p]
                stake_amount_formatted = "{:.4f} SYS".format(stake_amount)
                self.create_account(node_id=node_id, creator="eosio", name=producer,
                                    stake_cpu=stake_amount_formatted, stake_net=stake_amount_formatted, buy_ram_bytes=1048576,
                                    transfer=True)
                self.register_producer(node_id=node_id, producer=producer)
            # t = threading.Thread(target=create_and_register, args=(stake_amount,))
            t = ExceptionThread(channel, communicate, target=create_and_register, args=(stake_amount,))
            stake_amount = max(stake_amount / 2, 100)
            create_account_threads.append(t)
            t.start()

        for t in create_account_threads:
            t.join()

        assert len(channel) == 0, self.alert.red("{} exception(s) occurred in creating accounts / registering producers.".format(len(channel)))

        if not dont_vote:
            # 12. vote for producers
            self.vote_for_producers(node_id=0, voter="defproducera", voted_producers=list(self.producers.keys())[:min(21, len(self.producers))])
            # 13. verify head block producer is no longer eosio
            self.verify_head_block_producer(wait=1)

        # self.logger.debug(color.decorate(helper.pad(">>> Bootstrap finishes.", left=0, char=' ', sep="", total=80), fcolor="white", bcolor="black"))
        self.logger.info(">>> Bootstrap finishes.")
        self.logger.flush()


    def print_config(self):
        self.print_header("cluster configuration")
        self.print_config_helper("-i: cluster_id",      HELP_CLUSTER_ID,        self.cluster_id,        DEFAULT_CLUSTER_ID)
        self.print_config_helper("-t: topology",        HELP_TOPOLOGY,          self.topology,          DEFAULT_TOPOLOGY)
        self.print_config_helper("-c: center_node_id",  HELP_CENTER_NODE_ID,    self.center_node_id,    DEFAULT_CENTER_NODE_ID)
        self.print_config_helper("-n: total_nodes",     HELP_TOTAL_NODES,       self.total_nodes,       DEFAULT_TOTAL_NODES)
        self.print_config_helper("-y: total_producers", HELP_TOTAL_PRODUCERS,   self.total_producers,   DEFAULT_TOTAL_PRODUCERS)
        self.print_config_helper("-z: producer_nodes",  HELP_PRODUCER_NODES,    self.producer_nodes,    DEFAULT_PRODUCER_NODES)
        self.print_config_helper("-u: unstarted_nodes", HELP_UNSTARTED_NODES,   self.unstarted_nodes,   DEFAULT_UNSTARTED_NODES)


    def launch_cluster(self, **kwargs):
        return self.call("launch_cluster", cluster_id=self.cluster_id, center_node_id=self.center_node_id, node_count=self.total_nodes, shape=self.topology, nodes=self.nodes, verify=False, **kwargs)


    def get_cluster_info(self, **kwargs):
        return self.call("get_cluster_info", cluster_id=self.cluster_id, verify=False, **kwargs)


    def create_bios_accounts(self, **kwargs):
        accounts = [{"name":"eosio.bpay"},
                    {"name":"eosio.msig"},
                    {"name":"eosio.names"},
                    {"name":"eosio.ram"},
                    {"name":"eosio.ramfee"},
                    {"name":"eosio.rex"},
                    {"name":"eosio.saving"},
                    {"name":"eosio.stake"},
                    {"name":"eosio.token"},
                    {"name":"eosio.upay"}]
        return self.call("create_bios_accounts", cluster_id=self.cluster_id, creator="eosio", accounts=accounts, **kwargs)


    def schedule_protocol_feature_activations(self, **kwargs):
        return self.call("schedule_protocol_feature_activations",
                         cluster_id=self.cluster_id, node_id=0,
                         protocol_features_to_activate=["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"],
                         verify=False, **kwargs)


    def set_contract(self, contract_file: str, abi_file: str, account: str, node_id: int, name: str =None, **kwargs):
        return self.call("set_contract",
                         cluster_id=self.cluster_id, node_id=node_id, account = account,
                         contract_file=contract_file, abi_file=abi_file,
                         header=None if name is None else "set <{}> contract".format(name), **kwargs)


    def push_actions(self, node_id: int, actions: List[dict], name: str =None, **kwargs):
        return self.call("push_actions",
                         cluster_id=self.cluster_id, node_id=node_id, actions=actions,
                         header=None if name is None else name, **kwargs)


    def create_account(self, node_id, creator, name, stake_cpu, stake_net, buy_ram_bytes, transfer, **kwargs):
        return self.call("create_account", cluster_id=self.cluster_id, node_id=node_id, creator=creator, name=name,
                         stake_cpu=stake_cpu, stake_net=stake_net, buy_ram_bytes=buy_ram_bytes, transfer=transfer,
                         header="create \"{}\" account".format(name), **kwargs)

    def register_producer(self, node_id, producer, **kwargs):
            actions = [{"account": "eosio",
                        "action": "regproducer",
                        "permissions": [{"actor": "{}".format(producer),
                                        "permission": "active"}],
                        "data": {"producer": "{}".format(producer),
                                 "producer_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                                 "url": "www.test.com",
                                 "location": 0}}]
            self.push_actions(node_id=node_id, actions=actions, name="register \"{}\" producer".format(producer))


    def vote_for_producers(self, node_id, voter, voted_producers: List[str], **kwargs):
        assert len(voted_producers) <= 30, self.alert.red("Failed assertion: number of producers ({}) that account ({}) votes for <= 30 producers.".format(len(voted_producers), voter))
        actions = [{"account": "eosio",
                    "action": "voteproducer",
                    "permissions": [{"actor": "{}".format(voter),
                                     "permission": "active"}],
                    "data": {"voter": "{}".format(voter),
                             "proxy": "",
                             "producers": voted_producers}}]
        self.push_actions(node_id=node_id, actions=actions, name="votes for producers", **kwargs)


    def get_block(self, block_num_or_id, **kwargs):
        return self.call("get_block", cluster_id=self.cluster_id, block_num_or_id=block_num_or_id, verify=False, **kwargs)


    def verify_head_block_producer(self, retry=5, wait=1):
        self.print_header("get head block producer")
        get_head_block_producer = lambda : self.get_cluster_info(loud=False)["result"][0][1]["head_block_producer"]
        while retry >= 0:
            head_block_producer = get_head_block_producer()
            if head_block_producer == "eosio":
                self.logger.debug(color.yellow("Warning: Head block producer is still \"eosio\". Please wait for a while."))
            else:
                self.logger.debug(color.green("Head block producer is \"{}\", no longer eosio.".format(head_block_producer)))
                break
            time.sleep(wait)
            retry -= 1
        assert head_block_producer != "eosio"


    # TODO: set retry from command-line
    def call(self, endpoint: str, retry=5, wait=1, verify=True, loud=True, header: str = None, **data):
        if loud:
            header = endpoint.replace("_", " ") if header is None else header
            self.print_header(header)
        ix = Interaction(endpoint, self.service, data)
        if loud:
            # self.print_request(ix)
            self.logger.debug(ix.request.url)
            self.logger.debug(helper.pretty_json(ix.request.data))
        while not ix.response.ok and retry > 0:
            if loud:
                self.logger.debug(color.red(ix.response))
                self.logger.debug(color.vanilla("Retrying ..."))
            time.sleep(wait)
            ix.attempt()
            retry -= 1
        if loud:
            # self.print.magenta(ix.transaction_id)
            self.logger.debug(self.string.response_in_short(ix.response))
        assert ix.response.ok
        #  TODO: a better way to pass verify
        if verify:
            assert self.verify_transaction(ix.transaction_id, key=verify, loud=loud)
        self.logger.flush()
        return json.loads(ix.response.text)


    def verify_transaction(self, transaction_id, key=True, node_id=0, retry=600, wait=0.5, loud=True):
        key = "irreversible" if key == True else key
        if loud:
            self.logger.debug("Verifying ...")
        ix = Interaction("verify_transaction", self.service, dict(cluster_id=self.cluster_id, node_id=node_id, transaction_id=transaction_id))
        verified = helper.extract(ix.response, key=key, fallback=False)
        while not verified and retry > 0:
            time.sleep(wait)
            if loud:
                self.logger.debug("Verifying ...")
            ix.attempt()
            verified = helper.extract(ix.response, key=key, fallback=False)
            # debug temp
            # if not verified:
            #     self.print.response_in_full(ix.response)
            retry -= 1
        assert ix.response.ok
        if loud:
            if verified:
                self.logger.debug(color.decorate("Success!", fcolor="black", bcolor="green"))
            else:
                self.logger.debug(color.decorate("Failure!", fcolor="black", bcolor="red"))
                # debug temp
                # self.print.response_in_full(ix.response)
        return helper.extract(ix.response, key=key, fallback=False)


    # def print_request(self, ix):
    #     self.logger.debug(self.string.vanilla(ix.request.url))
    #     self.logger.debug(self.string.json(ix.request.data))


    @staticmethod
    def get_defproducer_names(num):
        def base26_to_int(s: str):
            res = 0
            for c in s:
                res *= 26
                res += ord(c) - ord('a')
            return res

        def int_to_base26(x: int):
            res = ""
            while True:
                q, r = divmod(x, 26)
                res = chr(ord('a') + r) + res
                x = q
                if q == 0:
                    break
            return res

        # 8031810176 = 26 ** 7 is integer for "baaaaaaa" in base26
        assert 0 <= num < 8031810176
        return "defproducer" + string.ascii_lowercase[num] if num < 26 else "defpr" + int_to_base26(8031810176 + num)[1:]


def test():
    buffered_color = WriterConfig(buffered=True, monochrome=False)
    unbuffered_mono = WriterConfig(buffered=False, monochrome=True)
    unbuffered_color = WriterConfig(buffered=False, monochrome=False)
    logger = Logger(ScreenWriter(config=buffered_color),
                    FileWriter(filename="mono.log", config=unbuffered_mono),
                    FileWriter(filename="color.log", config=unbuffered_color))
    service = Service(logger=logger)
    cluster = Cluster(service=service)


if __name__ == "__main__":
    test()
