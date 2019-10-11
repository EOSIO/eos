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
import typing

# third-party libraries
import requests

# user-defined modules
from logger import LoggingLevel, WriterConfig, ScreenWriter, FileWriter, Logger
from connection import Interaction
import color
import thread

# user-defined modules TO BE DEPRECATED
import printer


PROGRAM = "eosio-launcher-service"

DEFAULT_ADDRESS = "127.0.0.1"
DEFAULT_PORT = 1234
DEFAULT_DIR = "../build"
DEFAULT_FILE = os.path.join(".", "programs", PROGRAM, PROGRAM)
DEFAULT_START = False
DEFAULT_KILL = False

DEFAULT_CLUSTER_ID = 0
DEFAULT_CENTER_NODE_ID = None
DEFAULT_TOPOLOGY = "mesh"
DEFAULT_TOTAL_NODES = 4
DEFAULT_TOTAL_PRODUCERS = 4
DEFAULT_PRODUCER_NODES = 4
DEFAULT_UNSTARTED_NODES = 0
DEFAULT_DONT_BOOTSTRAP = False

DEFAULT_BUFFERED = True
DEFAULT_MONOCHROME = False

HELP_HELP = "Show this message and exit"
HELP_ADDRESS = "Address of launcher service"
HELP_PORT = "Listening port of launcher service"
HELP_DIR = "Working directory"
HELP_FILE = "Path to local launcher service file"
HELP_START = "Always start a new launcher service"
HELP_KILL = "Kill existing launcher services (if any)"

HELP_CLUSTER_ID = "Cluster ID to launch with"
HELP_CENTER_NODE_ID = "Center node ID for star or bridge"
HELP_TOPOLOGY = "Cluster topology to launch with"
HELP_TOTAL_NODES = "Number of total nodes"
HELP_TOTAL_PRODUCERS = "Number of total producers"
HELP_PRODUCER_NODES = "Number of nodes that have producers"
HELP_UNSTARTED_NODES = "Number of unstarted nodes"
HELP_DONT_BOOTSTRAP = "Launch cluster without bootstrapping"

HELP_LOG_LEVEL = "Stdout logging level (numeric)"
HELP_MONOCHROME = "Print in black and white instead of colors"
HELP_UNBUFFER = "Do not buffer for stdout logging"

HELP_LOG_ALL = "Set stdout logging level to ALL (0)"
HELP_TRACE = "Set stdout logging level to TRACE (10)"
HELP_DEBUG = "Set stdout logging level to DEBUG (20)"
HELP_INFO = "Set stdout logging level to INFO (30)"
HELP_WARN = "Set stdout logging level to WARN (40)"
HELP_ERROR = "Set stdout logging level to ERROR (50)"
HELP_FATAL = "Set stdout logging level to FATAL (60)"
HELP_LOG_OFF = "Set stdout logging level to OFF (100)"



class CommandLineArguments:
    def __init__(self):
        cla = self.parse()
        self.address = cla.address
        self.port = cla.port
        self.dir = cla.dir
        self.file = cla.file
        self.start = cla.start
        self.kill = cla.kill

        self.cluster_id = cla.cluster_id
        self.topology = cla.topology
        self.center_node_id = cla.center_node_id
        self.total_nodes = cla.total_nodes
        self.total_producers = cla.total_producers
        self.producer_nodes = cla.producer_nodes
        self.unstarted_nodes = cla.unstarted_nodes
        self.dont_bootstrap = cla.dont_bootstrap

        self.threshold = cla.threshold
        self.buffered = cla.buffered
        self.monochrome = cla.monochrome


    @staticmethod
    def parse():
        desc = color.decorate("Launcher Service for EOS Testing Framework", style="underline", fcolor="green")
        left = 5
        form = lambda text, value=None: "{} ({})".format(helper.pad(text, left=left, total=50, char=" ", sep=""), value)
        parser = argparse.ArgumentParser(description=desc, add_help=False, formatter_class=lambda prog: argparse.RawTextHelpFormatter(prog, max_help_position=50))
        parser.add_argument("-h", "--help", action="help", help=' ' * left + HELP_HELP)

        parser.add_argument("-a", "--address", type=str, metavar="IP", help=form(HELP_ADDRESS, DEFAULT_ADDRESS))
        parser.add_argument("-p", "--port", type=int, help=form(HELP_PORT, DEFAULT_PORT))
        parser.add_argument("-d", "--dir", type=str, metavar="PATH", help=form(HELP_DIR, DEFAULT_DIR))
        parser.add_argument("-f", "--file", type=str, metavar="PATH", help=form(HELP_FILE, DEFAULT_FILE))
        parser.add_argument("-s", "--start", action="store_true", default=None, help=form(HELP_START, DEFAULT_START))
        parser.add_argument("-k", "--kill", action="store_true", default=None, help=form(HELP_KILL, DEFAULT_KILL))

        parser.add_argument("-i", "--cluster-id", dest="cluster_id", type=int, metavar="ID", help=form(HELP_CLUSTER_ID, DEFAULT_CLUSTER_ID))
        parser.add_argument("-t", "--topology", type=str, metavar="SHAPE", help=form(HELP_TOPOLOGY, DEFAULT_TOPOLOGY), choices={"mesh", "star", "bridge", "line", "ring", "tree"})
        parser.add_argument("-c", "--center-node-id", dest="center_node_id", type=int, metavar="ID", help=form(HELP_CENTER_NODE_ID, DEFAULT_CENTER_NODE_ID))
        parser.add_argument("-n", "--total-nodes", dest="total_nodes", type=int, metavar="NUM", help=form(HELP_TOTAL_NODES, DEFAULT_TOTAL_NODES))
        parser.add_argument("-y", "--total-producers", dest="total_producers", type=int, metavar="NUM", help=form(HELP_TOTAL_PRODUCERS, DEFAULT_TOTAL_PRODUCERS))
        parser.add_argument("-z", "--producer-nodes", dest="producer_nodes", type=int, metavar="NUM", help=form(HELP_PRODUCER_NODES, DEFAULT_PRODUCER_NODES))
        parser.add_argument("-u", "--unstarted-nodes", dest="unstarted_nodes", type=int, metavar="NUM", help=form(HELP_UNSTARTED_NODES, DEFAULT_UNSTARTED_NODES))
        parser.add_argument("-xboot", "--dont-bootstrap", dest="dont_bootstrap", action="store_true", help=form(HELP_DONT_BOOTSTRAP, DEFAULT_DONT_BOOTSTRAP))

        threshold = parser.add_mutually_exclusive_group()
        threshold.add_argument("-l", "--log-level", dest="threshold", type=int, metavar="LEVEL", action="store", help=form(HELP_LOG_LEVEL))
        threshold.add_argument("--log-all", dest="threshold", action="store_const", const="ALL", help=form(HELP_LOG_ALL))
        threshold.add_argument("--trace", dest="threshold", action="store_const", const="TRACE", help=form(HELP_TRACE))
        threshold.add_argument("--debug", dest="threshold", action="store_const", const="DEBUG", help=form(HELP_DEBUG))
        threshold.add_argument("--info", dest="threshold", action="store_const", const="INFO", help=form(HELP_INFO))
        threshold.add_argument("--warn", dest="threshold", action="store_const", const="WARN", help=form(HELP_WARN))
        threshold.add_argument("--error", dest="threshold", action="store_const", const="ERROR", help=form(HELP_ERROR))
        threshold.add_argument("--fatal", dest="threshold", action="store_const", const="FATAL", help=form(HELP_FATAL))
        threshold.add_argument("--log-off", dest="threshold", action="store_const", const="OFF", help=form(HELP_LOG_OFF))
        parser.add_argument("-xbuff", "--unbuffer", dest="buffered", action="store_false", default=True, help=form(HELP_UNBUFFER, not DEFAULT_BUFFERED))
        parser.add_argument("-xcolor", "--monochrome", action="store_true", default=False, help=form(HELP_MONOCHROME, DEFAULT_MONOCHROME))

        return parser.parse_args()


class Service:
    def __init__(self, address=None, port=None, dir=None, file=None, start=None, kill=None, logger=None, dont_connect=False):

        # configure service
        self.cla        = CommandLineArguments()
        self.address    = helper.override(DEFAULT_ADDRESS, address, self.cla.address)
        self.port       = helper.override(DEFAULT_PORT,    port,    self.cla.port)
        self.dir        = helper.override(DEFAULT_DIR,     dir,     self.cla.dir)
        self.file       = helper.override(DEFAULT_FILE,    file,    self.cla.file)
        self.start      = helper.override(DEFAULT_START,   start,   self.cla.start)
        self.kill       = helper.override(DEFAULT_KILL,    kill,    self.cla.kill)

        # determine remote or local launcher service to connect to
        if self.address in ("127.0.0.1", "localhost"):
            self.remote = False
        else:
            self.remote = True
            self.file = self.start = self.kill = None

        # register logger
        # TODO: set default behavior
        self.logger = logger
        for w in self.logger.writers:
            if isinstance(w, ScreenWriter):
                th = helper.override(w.threshold, self.cla.threshold)
                if isinstance(th, str):
                    th = LoggingLevel[th]
                self.threshold = w.threshold = th
                self.buffered = w.buffered = helper.override(DEFAULT_BUFFERED, w.buffered, self.cla.buffered)
                self.monochrome = w.monochrome = helper.override(DEFAULT_MONOCHROME, w.monochrome, self.cla.monochrome)

        if not dont_connect:
            self.connect()


    def connect(self):
        # print system info
        self.print_system_info()

        # print configuration
        self.print_config()

        # change working directory
        self.change_dir()

        # connect to remote service and return
        if self.remote:
            self.connect_to_remote_service()
            return

        # connect to launcher service
        self.connect_to_local_service()


    def print_header(self, text, level="DEBUG", buffer=False):
        self.logger.log(helper.format_header(text), level=level, buffer=buffer)


    def print_system_info(self):
        self.print_header("system info")
        self.logger.debug("{:22}{}".format("UTC Time", time.strftime("%Y-%m-%d %H:%M:%S %Z", time.gmtime())))
        self.logger.debug("{:22}{}".format("Local Time", time.strftime("%Y-%m-%d %H:%M:%S %Z", time.localtime())))
        self.logger.debug("{:22}{}".format("Platform", platform.platform()))


    def print_config(self):
        self.print_header("service configuration")
        self.print_config_helper("-a: address",    HELP_ADDRESS,    self.address,      DEFAULT_ADDRESS)
        self.print_config_helper("-p: port",       HELP_PORT,       self.port,         DEFAULT_PORT)
        self.print_config_helper("-d: dir",        HELP_DIR,        self.dir,          DEFAULT_DIR)
        self.print_config_helper("-f: file",       HELP_FILE,       self.file,         DEFAULT_FILE)
        self.print_config_helper("-s: start",      HELP_START,      self.start,        DEFAULT_START)
        self.print_config_helper("-k: kill",       HELP_KILL,       self.kill,         DEFAULT_KILL)
        try:
            log_level = "{} ({})".format(LoggingLevel(self.threshold).name, self.threshold)
        except ValueError:
            log_level = self.threshold
        self.print_config_helper("-l: log-level",  HELP_LOG_LEVEL,  log_level)
        self.print_config_helper("-x: buffered",   HELP_UNBUFFER,   not self.buffered, not DEFAULT_BUFFERED)
        self.print_config_helper("-m: monochrome", HELP_MONOCHROME, self.monochrome,   DEFAULT_MONOCHROME)


    def print_config_helper(self, label, help, value, default_value=None, compress=True):
        colored = color.blue(value) if value != default_value else str(value)
        self.logger.debug("{:31}{:48}{}".format(color.yellow(label), help, helper.compress(colored) if compress else colored))


    def change_dir(self):
        self.print_header("change working directory")
        os.chdir(self.dir)
        self.logger.debug("{:22}{}".format("Working Directory", os.getcwd()))


    # TODO
    def connect_to_remote_service(self):
        pass


    def connect_to_local_service(self):
        self.print_header("connect to launcher service")
        spid = self.get_service_pid_list()
        if self.kill:
            self.kill_service(spid)
            spid.clear()
        if spid and not self.start:
            self.start_local_service(spid[0])
        else:
            self.start_service()


    def start_local_service(self, pid):
        current_port = self.get_service_port(pid)
        current_file = self.get_service_file(pid)
        self.logger.debug(color.green("Connecting to existing launcher service with process ID [{}].".format(pid)))
        self.logger.debug(color.green("No new launcher service will be started."))
        self.logger.debug("Configuration of existing launcher service:")
        self.logger.debug("--- Listening port: [{}]".format(color.yellow(current_port)))
        self.logger.debug("--- Path to file: {}".format(color.vanilla(current_file)))
        if self.port != current_port:
            self.logger.debug(color.yellow("Warning: port setting (port = {}) ignored.".format(self.port)))
            self.port = current_port
        if self.file != current_file:
            self.logger.debug(color.yellow("Warning: file setting (file = {}) ignored.".format(self.file)))
            self.file = current_file
        self.logger.debug("To always start a new launcher service, pass {} or {}.".format(color.yellow("-s"), color.yellow("--start")))
        self.logger.debug("To kill existing launcher services, pass {} or {}.".format(color.yellow("-k"), color.yellow("--kill")))


    def start_service(self):
        self.logger.debug(color.green("Starting a new launcher service."))
        subprocess.Popen([self.file, "--http-server-address=0.0.0.0:{}".format(self.port), "--http-threads=4"])
        if not self.get_service_pid_list():
            self.logger.error("Error: Launcher service is not started properly!", assert_false=True)


    def kill_service(self, spid=None):
        spid = self.get_service_pid_list() if spid is None else spid
        for x in spid:
            self.logger.debug(color.yellow("Killing exisiting launcher service with process ID [{}].".format(x)))
            subprocess.run(["kill", "-SIGTERM", str(x)])


    def get_service_pid_list(self) -> List[int]:
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
        # self.print = service.print
        # self.string = service.string
        # self.alert = service.alert

        self.print_header = service.print_header
        self.print_config_helper = service.print_config_helper


        # configure cluster
        self.cluster_id      = helper.override(DEFAULT_CLUSTER_ID,      cluster_id,      self.cla.cluster_id)
        self.topology        = helper.override(DEFAULT_TOPOLOGY,        topology,        self.cla.topology)
        self.center_node_id  = helper.override(DEFAULT_CENTER_NODE_ID,  center_node_id,  self.cla.center_node_id)
        self.total_nodes     = helper.override(DEFAULT_TOTAL_NODES,     total_nodes,     self.cla.total_nodes)
        self.total_producers = helper.override(DEFAULT_TOTAL_PRODUCERS, total_producers, self.cla.total_producers)
        self.producer_nodes  = helper.override(DEFAULT_PRODUCER_NODES,  producer_nodes,  self.cla.producer_nodes)
        self.unstarted_nodes = helper.override(DEFAULT_UNSTARTED_NODES, unstarted_nodes, self.cla.unstarted_nodes)
        self.dont_bootstrap  = helper.override(DEFAULT_DONT_BOOTSTRAP,  dont_bootstrap,  self.cla.dont_bootstrap)

        # reconcile conflict in config
        self.resolve_config_conflict()

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

        if not self.dont_bootstrap:
            self.bootstrap(dont_vote=dont_vote)
        else:
            self.launch_cluster_without_bootstrap()


    def resolve_config_conflict(self):
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


    def launch_cluster_without_bootstrap(self):
        self.logger.info(color.bold(">>> Launch without bootstrapping starts."))
        self.print_config()
        self.launch_cluster()
        self.get_cluster_info()
        self.set_bios_contract()
        self.logger.info(color.bold(">>> Launch without bootstrapping finishes."))


    # TODO: allow users to specify path instead of hardcoded "eosio.contracts"
    def set_bios_contract(self):
        self.set_contract(node_id=0,
                          contract_file="../../eosio.contracts/build/contracts/eosio.bios/eosio.bios.wasm",
                          abi_file="../../eosio.contracts/build/contracts/eosio.bios/eosio.bios.abi",
                          account="eosio", name="eosio.bios")



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

        self.logger.info(color.bold(">>> Bootstrap starts."))

        # 0. print configuration
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
                          contract_file="../../eosio.contracts/build/contracts/eosio.token/eosio.token.wasm",
                          abi_file="../../eosio.contracts/build/contracts/eosio.token/eosio.token.abi",
                          account="eosio.token", name="eosio.token")

        # 6. create tokens
        total_supply = 1e9
        total_supply_formatted = helper.format_tokens(total_supply)

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
                          contract_file="../../eosio.contracts/build/contracts/eosio.system/eosio.system.wasm",
                          abi_file="../../eosio.contracts/build/contracts/eosio.system/eosio.system.abi",
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
        report = lambda channel, id: channel.append(id)

        for p in self.producers:
            def create_and_register(stake_amount):
                producer = p
                node_id = self.producers[p]
<<<<<<< HEAD
                stake_amount_formatted = "{:.4f} SYS".format(stake_amount)
=======
                stake_amount_formatted = helper.format_tokens(stake_amount)
                # when create/register accounts: always push actions to node #0 instead of the nodes of the accounts
                # otherwise may encounter errors caused by nodes not synced up yet
                # "message": "false: Unknown action delegatebw in contract eosio"
>>>>>>> 0b5adeab6def3aef165a1c0886dc44d601e336ba
                self.create_account(node_id=0, creator="eosio", name=producer,
                                    stake_cpu=stake_amount_formatted, stake_net=stake_amount_formatted, buy_ram_bytes=1048576,
                                    transfer=True)
                self.register_producer(node_id=0, producer=producer)
            # t = threading.Thread(target=create_and_register, args=(stake_amount,))
            t = thread.ExceptionThread(channel, report, target=create_and_register, args=(stake_amount,))
            stake_amount = max(stake_amount / 2, 100)
            create_account_threads.append(t)
            t.start()

        for t in create_account_threads:
            t.join()

        if len(channel) != 0:
            self.logger.error("{} exception(s) occurred in creating accounts / registering producers.".format(len(channel)), assert_false=True)


        if not dont_vote:
            # 12. vote for producers
            self.vote_for_producers(node_id=0, voter="defproducera", voted_producers=list(self.producers.keys())[:min(21, len(self.producers))])
            # 13. verify head block producer is no longer eosio
            self.verify_head_block_producer()

        # self.logger.debug(color.decorate(helper.pad(">>> Bootstrap finishes.", left=0, char=' ', sep="", total=80), fcolor="white", bcolor="black"))
        self.logger.info(color.bold(">>> Bootstrap finishes."))


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
        return self.call("launch_cluster", cluster_id=self.cluster_id, center_node_id=self.center_node_id, node_count=self.total_nodes, shape=self.topology, nodes=self.nodes, expect_transaction_id=False, **kwargs)


    def get_cluster_info(self, **kwargs):
        return self.call("get_cluster_info", cluster_id=self.cluster_id, expect_transaction_id=False, **kwargs)

    def stop_node(self, **kwargs):
        return self.call("stop_node", cluster_id=self.cluster_id, expect_transaction_id=False, **kwargs)

    def start_node(self, **kwargs):
        return self.call("start_node", cluster_id=self.cluster_id, expect_transaction_id=False, **kwargs)

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
                         expect_transaction_id=False, **kwargs)


    def set_contract(self, contract_file: str, abi_file: str, account: str, node_id: int, name: str =None, **kwargs):
        return self.call("set_contract",
                         cluster_id=self.cluster_id, node_id=node_id, account = account,
                         contract_file=contract_file, abi_file=abi_file,
                         header=None if name is None else "set <{}> contract".format(name), **kwargs)


    def push_actions(self, node_id: int, actions: List[dict], name: str =None, **kwargs):
        return self.call("push_actions",
                         cluster_id=self.cluster_id, node_id=node_id, actions=actions,
                         header=None if name is None else name, **kwargs)


    def create_account(self, node_id, creator, name, stake_cpu, stake_net, buy_ram_bytes, transfer, buffer=True, **kwargs):
        return self.call("create_account", cluster_id=self.cluster_id, node_id=node_id, creator=creator, name=name,
                         stake_cpu=stake_cpu, stake_net=stake_net, buy_ram_bytes=buy_ram_bytes, transfer=transfer,
                         buffer=buffer, header="create \"{}\" account".format(name), **kwargs)

    def register_producer(self, node_id, producer, buffer=True, **kwargs):
            actions = [{"account": "eosio",
                        "action": "regproducer",
                        "permissions": [{"actor": "{}".format(producer),
                                        "permission": "active"}],
                        "data": {"producer": "{}".format(producer),
                                 "producer_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                                 "url": "www.test.com",
                                 "location": 0}}]
            self.push_actions(node_id=node_id, actions=actions, name="register \"{}\" producer".format(producer), buffer=buffer)


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
        return self.call("get_block", cluster_id=self.cluster_id, block_num_or_id=block_num_or_id, expect_transaction_id=False, **kwargs)


    def verify_head_block_producer(self, retry=5, sleep=1):
        self.print_header("get head block producer")
        ix = self.get_cluster_info(silent=True)
        # get_head_block_producer = lambda : self.get_cluster_info(silent=True)["result"][0][1]["head_block_producer"]
        extract_head_block_producer = lambda ix: json.loads(ix.response.text)["result"][0][1]["head_block_producer"]
        while retry >= 0:
            ix = self.get_cluster_info(silent=True)
            head_block_producer = extract_head_block_producer(ix)
            self.logger.trace(ix.get_formatted_response())
            if head_block_producer == "eosio":
                self.logger.debug(color.yellow("Head block producer is still \"eosio\"."))
            else:
                self.logger.debug(color.green("Head block producer is now \"{}\", no longer eosio.".format(head_block_producer)))
                break
            self.logger.trace("{} {} for head block producer verification...".format(retry, "retries remain" if retry > 1 else "retry remains"))
            self.logger.trace("Sleep for {}s before next retry...".format(sleep))
            time.sleep(sleep)
            retry -= 1
        assert head_block_producer != "eosio"


    # TODO: set retry from command-line
    def call(self, endpoint: str, retry=5, sleep=1, expect_transaction_id=True, verify_key="irreversible", silent=False, buffer=False, header: str = None, **data) -> dict:
        with self.logger as log:
            if not silent:
<<<<<<< HEAD
                self.logger.trace(color.red(ix.response))
                # TODO: detailed info for retry
                self.logger.trace("{} {} for http connection...".format(retry, "retries remain" if retry > 1 else "retry remains"))
                self.logger.trace("Sleep for {}s before next retry...".format(sleep))
            time.sleep(sleep)
            ix.attempt()
            retry -= 1
        if not silent:
            self.logger.debug(ix.get_formatted_response())
        if not ix.response.ok:
            self.logger.error(ix.get_formatted_response(show_content=True), flush=True, terminate=True)
        # assert ix.response.ok
        if expect_transaction_id and ix.transaction_id is None:
            self.logger.warn("Warning: No transaction ID returned.")
        if expect_transaction_id:
            assert self.verify_transaction(ix.transaction_id, verify_key=verify_key, silent=silent)
        self.logger.flush()
        # TODO: change to return ix
        # return json.loads(ix.response.text)
        return ix


    # MARK 191010: TODO assert --> logger.error(terminate=True)
=======
                header = endpoint.replace("_", " ") if header is None else header
                self.print_header(header, buffer=buffer)
            ix = Interaction(endpoint, self.service, data)
            if not silent:
                log.debug(ix.request.url, buffer=buffer)
                log.debug(helper.format_json(ix.request.data), buffer=buffer)
            while not ix.response.ok and retry > 0:
                if not silent:
                    log.trace(color.red(ix.response))
                    # TODO: detailed info for retry
                    log.trace("{} {} for http connection...".format(retry, "retries remain" if retry > 1 else "retry remains"))
                    log.trace("Sleep for {}s before next retry...".format(sleep))
                time.sleep(sleep)
                ix.attempt()
                retry -= 1
            if not silent:
                log.debug(ix.get_formatted_response(), buffer=buffer)
            # assert ix.response.ok
            if not ix.response.ok:
                log.error(ix.response.text, assert_false=True)
            if expect_transaction_id and ix.transaction_id is None:
                log.warn("Warning: No transaction ID returned.")
            if expect_transaction_id:
                assert self.verify_transaction(ix.transaction_id, verify_key=verify_key, silent=silent, buffer=buffer)
            # log.flush()
            # TODO: change to return ix
            # return json.loads(ix.response.text)
            if buffer:
                log.flush()
            return ix


    # MARK 191010: TODO assert --> logger.error(assert_false=True)
>>>>>>> 0b5adeab6def3aef165a1c0886dc44d601e336ba
    # keep majority of system logging at DEBUG, leave INFO for user code
    def verify_transaction(self, transaction_id, verify_key="irreversible", node_id=0, retry=10, sleep=0.5, silent=False, buffer=False):
        # TODO: can have an assert to guard against non-existing field other than irreversible / contained
        with self.logger as log:
            if not silent:
                log.trace("Verifying ...", buffer=buffer)
            ix = Interaction("verify_transaction", self.service, dict(cluster_id=self.cluster_id, node_id=node_id, transaction_id=transaction_id))
            verified = helper.extract(ix.response, key=verify_key, fallback=False)
            if not silent:
                log.trace(ix.get_formatted_response(show_content=True), buffer=buffer)
            while not verified and retry > 0:
                if not silent:
                    log.trace("{} {} for verification...".format(retry, "retries remain" if retry > 1 else "retry remains"), buffer=buffer)
                    log.trace("Sleep for {}s before next retry...".format(sleep), buffer=buffer)
                time.sleep(sleep)
                ix.attempt()
                if not silent:
                    log.trace(ix.get_formatted_response(show_content=True), buffer=buffer)
                verified = helper.extract(ix.response, key=verify_key, fallback=False)
                retry -= 1
            # assert ix.response.ok
            if not ix.response.ok:
                log.error(ix.response.text, buffer=buffer, assert_false=True)
            if not silent:
                if verified:
                    log.debug(color.decorate("{}!".format(verify_key.title()), fcolor="black", bcolor="green"), buffer=buffer)
                else:
                    log.error(ix.get_formatted_response(show_content=True), buffer=buffer)
                    log.error("Failed to verify as \"{}\"!".format(verify_key), buffer=buffer, assert_false=True)
            if buffer:
                log.flush()
            return verified


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
    buffered_color = WriterConfig(buffered=True, monochrome=False, threshold="DEBUG")
    unbuffered_mono = WriterConfig(buffered=False, monochrome=True, threshold="TRACE")
    unbuffered_color = WriterConfig(buffered=False, monochrome=False, threshold="TRACE")
    logger = Logger(ScreenWriter(config=buffered_color),
                    FileWriter(filename="mono.log", config=unbuffered_mono),
                    FileWriter(filename="colo.log", config=unbuffered_color))
    service = Service(logger=logger)
    cluster = Cluster(service=service)


if __name__ == "__main__":
    test()
