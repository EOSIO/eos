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

from typing import List
from printer import *

class LauncherCaller:
    DEFAULT_VERB = 0
    DEFAULT_ADDR = "127.0.0.1"
    DEFAULT_PORT = 1234
    DEFAULT_FILE = "./programs/eosio-launcher-service/eosio-launcher-service"
    DEFAULT_CLUSTER_ID = 0

    def parse_args(self):
        parser = argparse.ArgumentParser(description=underlined_str(green_str("Launcher Service for EOS Testing Framework")), add_help=False, formatter_class=lambda prog: argparse.HelpFormatter(prog, max_help_position=50))
        parser.add_argument('-h', '--help', action="help", help="Show this message and exit")
        parser.add_argument('-v', '--verbose', action="count", help="Verbosity level (-v for 1, -vv for 2)")
        parser.add_argument('-a', '--address', type=str, help="Address of launcher service")
        parser.add_argument('-p', '--port', type=int, help="Port to listen to")
        parser.add_argument('-s', '--start', action="store_true", help="Always start a new launcher service")
        parser.add_argument('-k', '--kill', action="store_true", help="Kill existing launcher services (if any)")
        parser.add_argument("-f", "--file", type=str, help="Path to launcher service file")
        parser.add_argument("-i", "--cluster-id", dest="cluster_id", metavar="ID", type=int, help="Cluster ID to launch with")
        return parser.parse_args()

    def connect_to_remote_service(self):
        pass

    def __init__(self):
        """
        Attributes
        ----------
        address         address of launcher service (default: 127.0.0.1)
        port            listening port of launcher service (default: 1234)
        file            file to launcher service
        link            http://[address]:[port]/v1/launcher
        """
        # parsing arguments
        print_in_blue(pad("parsing arguments"))
        self.args = self.parse_args()
        self.verbose = self.args.verbose
        self.address = self.override(self.args.address, self.DEFAULT_ADDR)
        self.port = self.override(self.args.port, self.DEFAULT_PORT)
        self.file = self.override(self.args.file, self.DEFAULT_FILE)
        self.cluster_id = self.override(self.args.cluster_id, self.DEFAULT_CLUSTER_ID)

        print("{:50s}{:}".format("Verbosity level", self.args.verbose))
        print("{:50s}{}".format("Address of launcher service", self.args.address))
        print("{:50s}{}".format("Listening port of launcher service", self.args.port))
        print("{:50s}{}".format("Path to launcher service file", self.args.file))
        print("{:50s}{}".format("Always start a new launcher service", self.args.start))
        print("{:50s}{}".format("Kill existing launcher services (if any)", self.args.kill))
        print("{:50s}{}".format("Cluster ID to launch with", self.args.cluster_id))

        if self.args.address is not None:
            self.connect_to_remote_service()
            return

        # changing working directory
        # HARDCODED - to change later
        print_in_blue(pad("changing working directory"))
        os.chdir('../build')
        print("Current working directory: {}.".format(os.getcwd()))

        # starting launcher service
        print_in_blue(pad("connecting to launcher service"))

        if self.args.kill:
            self.kill_service()
        if self.args.start:
            self.start_service()
        else:
            spid = self.get_service_pid()
            if spid:
                self.use_service(spid[0])
            else:
                self.start_service()

    def get_service_pid(self):
        pid = self.pgrep("eosio-launcher-service")
        if len(pid) == 0:
            print_in_yellow("No launcher is running currently.")
        elif len(pid) == 1:
            print_in_green("Launcher service is running with process ID [{}].".format(pid[0]))
        else:
            print_in_green("Multiple launcher services are running with process IDs {}".format(pid))
        return pid

    def get_service_port(self, pid):
        res = subprocess.Popen(['ps', '-p', str(pid), '-o', 'command='], stdout=subprocess.PIPE).stdout.read().decode('ascii')
        shlex.split(res)
        for x in shlex.split(res):
            if x.startswith("--http-server-address"):
                return x.split(':')[-1]
        assert False, red_str("Fail to get --http-server-address from process ID {}!".format(pid))

    def get_service_file(self, pid):
        return subprocess.Popen(['ps', '-p', str(pid), '-o', 'comm='], stdout=subprocess.PIPE).stdout.read().rstrip().decode('ascii')

    def use_service(self, pid):
        self.port = self.get_service_port(pid)
        self.file = self.get_service_file(pid)
        print_in_green("Using the existing launcher service with process ID [{}].".format(pid))
        print_in_green("Launcher service file is located at {}.".format(self.file))
        print_in_green("Listening port: {}.".format(self.port))
        if self.args.port: print_in_yellow("Command-line option [--port {}] is ignored.".format(self.args.port))
        if self.args.file: print_in_yellow("Command-line option [--file {}] is ignored.".format(self.args.file))
        print_in_yellow("To always start a new launcher service, pass -s or --start.")
        print_in_yellow("To kill existing launcher services, pass -k or --kill.")

    def start_service(self):
        assert self.check_service_file(), red_str("Launcher service file not found!")
        print_in_green("Starting a new launcher service.")
        subprocess.Popen([self.file, "--http-server-address=0.0.0.0:{}".format(self.port)])
        assert self.get_service_pid(), red_str("Launcher Service is not started properly.")

    def kill_service(self):
        pid = self.get_service_pid()
        for x in pid:
            print_in_yellow("Killing exisiting launcher service with process ID [{}].".format(x))
            subprocess.run(["kill", "-SIGTERM", str(x)])

    def check_service_file(self):
        exists = os.path.isfile(self.file)
        if exists:
            print_in_green("Found launcher service file at {}.".format(self.file))
        else:
            print_in_red("No launcher service file found at {}!".format(self.file))
        return exists

    def call_rpc(self, func: str, data):
        return requests.post("http://{}:{}/v1/launcher/{}".format(self.address, self.port, func), data=data)

    def describe(self, description, sleep=0):
        print_in_blue(pad(description))
        if sleep:
            time.sleep(sleep)

    def print_response(self, timeout=1):
        print_response(self.response, timeout)

    def print_system_info(self):
        print("{:20s}{}".format("UTC Time", time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())))
        print("{:20s}{}".format("Local Time", time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())))
        print("{:20s}{}".format("Platform", platform.platform()))
        print("{:20s}{}".format("Server", self.server))
        print("{:20s}{}".format("Port", self.port))

    def launch(self,
               cluster_id=None,
               total_nodes=1,
               producer_nodes=1,
               unstarted_nodes=0,
               per_node_producers=1,
               total_producers=None,
               topology="mesh",
               dont_boostrap=False,
               only_bios=False,
               only_set_producers=False,
               common_extra_args=None,
               specific_extra_args=None):
        """
        parameters
        ----------
        cluster_id          cluster ID
        total_nodes         total number of nodes
        ...
        """
        cluster_id = self.override(cluster_id, self.cluster_id)

        total_producers = total_producers if total_producers else per_node_producers * producer_nodes

        assert cluster_id >= 0, \
               red_str("Cluster ID ({}) must be non-negative.".format(cluster_id))
        assert total_nodes >= producer_nodes + unstarted_nodes, \
               red_str("total_nodes ({}) must be greater than or equal to producer_nodes ({}) + unstarted_nodes ({})."
                       .format(total_nodes, producer_nodes, unstarted_nodes))
        assert per_node_producers * (producer_nodes - 1) < total_producers <= per_node_producers * producer_nodes, \
               red_str("Incompatible producers configuration with "
                       "per_node_producers ({}), producer_nodes ({}), and total_producers ({})."
                       .format(per_node_producers, producer_nodes, total_producers))
        assert total_producers <= 26, \
               red_str("Trying to have {} producers. More than names defproducera ... defproducerz can accommodate."
                       .format(total_producers))
        # more assertions to follow ...

        info = {}
        info['cluster_id'] = cluster_id
        info['node_count'] = total_nodes
        info['shape'] = topology
        info['nodes'] = []
        for i in range(total_nodes):
            info['nodes'] += [{'node_id': i}]
            if i < producer_nodes:
                names = [] if i else ['eosio']
                for j in range(i * per_node_producers, min((i+1) * per_node_producers, total_producers)):
                    names += ["defproducer" + string.ascii_lowercase[j]]
                info['nodes'][i]['producers'] = names
        self.launch_cluster(json.dumps(info))

    def get_transaction_id(self):
        try:
            return json.loads(self.response.text)['transaction_id']
        except KeyError:
            return

    def launch_cluster(self, data):
        self.response = self.call_rpc("launch_cluster", data)

    def create_bios_accounts(self, data):
        self.response = self.call_rpc("create_bios_accounts", data)

    def create_account(self, data):
        self.response = self.call_rpc("create_account", data)

    def get_account(self, data):
        self.response = self.call_rpc("get_account", data)

    def get_cluster_info(self, data):
        self.response = self.call_rpc("get_cluster_info", data)

    def get_log_data(self, data):
        self.response = self.call_rpc("get_log_data", data)

    def get_protocol_features(self, data):
        self.response = self.call_rpc("get_protocol_features", data)

    def get_cluster_running_state(self, data):
        self.response = self.call_rpc("get_cluster_running_state", data)

    def push_actions(self, data):
        self.response = self.call_rpc("push_actions", data)

    def schedule_protocol_feature_activations(self, data):
        self.response = self.call_rpc("schedule_protocol_feature_activations", data)

    def set_contract(self, data):
        self.response = self.call_rpc("set_contract", data)

    def stop_all_clusters(self):
        self.response = self.call_rpc("stop_all_clusters", "")

    def verify_transaction(self, data):
        self.response = self.call_rpc("verify_transaction", data)

    # ---------- Utilities ------------------------------------------------------------------------

    @staticmethod
    def override(value, default_value):
        return value if value is not None else default_value

    def pgrep(self, pattern: str) -> List[int]:
        out = subprocess.Popen(['pgrep', pattern], stdout=subprocess.PIPE).stdout.read()
        return [int(x) for x in out.splitlines()]

def main():
    caller = LauncherCaller()

    caller.describe("launching a cluster of nodes")
    caller.launch(producer_nodes=3, unstarted_nodes=2, total_nodes=6, total_producers=5, per_node_producers=2)
    caller.print_response()

    caller.describe("getting cluster information", sleep=1)
    caller.get_cluster_info('828')
    caller.print_response()

    print(" " * 100)

def prototype():
    caller = LauncherCaller()

    caller.describe("launching a cluster of nodes")
    caller.launch_cluster('{"node_count":4,"cluster_id":2,"nodes":[{"node_id":0,"producers":["eosio","inita"]}, {"node_id":1,"producers":["initb"]}]}')
    caller.print_response()

    caller.describe("getting cluster information", sleep=1)
    caller.get_cluster_info('2')
    caller.print_response()

    caller.describe("getting cluster running state", sleep=1)
    caller.get_cluster_running_state('2')
    caller.print_response()

    caller.describe("creating system account", sleep=1)
    caller.create_bios_accounts(
        '{"cluster_id":2,"node_id":0,"creator":"eosio","accounts":[{"name":"inita","owner":"EOS8kE63z4NcZatvVWY4jxYdtLg6UEA123raMGwS6QDKwpQ69eGcP","active":"EOS8kE63z4NcZatvVWY4jxYdtLg6UEA123raMGwS6QDKwpQ69eGcP"}]}')
    caller.print_response()

    caller.describe("getting account information", sleep=1)
    caller.get_account(
        '{"cluster_id":2,"node_id":0,"name":"inita"}')
    caller.print_response()

    caller.describe("setting system contract", sleep=1)
    caller.set_contract(
        '{"cluster_id":2,"node_id":0,"account":"eosio","contract_file":"../../contracts/build/contracts/eosio.system/eosio.system.wasm","abi_file":"../../contracts/build/contracts/eosio.system/eosio.system.abi"}')
    caller.print_response()

    tid = caller.get_transaction_id()
    if tid:
        print("Getting transaction ID {}".format(tid))
        caller.describe("verifying transaction", sleep=1)
        caller.verify_transaction(
            '{{"cluster_id":2,"node_id":0,"transaction_id":"{}"}}'.format(tid))
        caller.print_response()
    else:
        print("No transaction ID returned.")

    if not tid:
        caller.describe("setting system contract", sleep=1)
        caller.set_contract(
            '{"cluster_id":2,"node_id":0,"account":"eosio","contract_file":"../../contracts/build/contracts/eosio.system/eosio.system.wasm","abi_file":"../../contracts/build/contracts/eosio.system/eosio.system.abi"}')
        caller.print_response()

        tid = caller.get_transaction_id()
        if tid:
            print("Getting transaction ID {}".format(tid))
            caller.describe("verifying transaction", sleep=1)
            caller.verify_transaction(
                '{{"cluster_id":2,"node_id":0,"transaction_id":"{}"}}'.format(tid))
            caller.print_response()
        else:
            print("No transaction ID returned.")

    print(" " * 100)


if __name__ == '__main__':
    main()
