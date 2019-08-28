#! /usr/bin/env python3

import argparse
import json
import os
import pgrep
import platform
import requests
import subprocess
import time

from printer import print_in_blue, print_in_green, print_in_red, print_response, pad, underlined_str, green_str, yellow_str

class LauncherCaller:
    DEFAULT_SERVER = "localhost"
    DEFAULT_PORT = 1234

    def __init__(self):
        args = self.parse_args()
        self.directory = os.path.dirname(os.path.realpath(__file__))
        os.chdir(os.path.join(self.directory, '../build'))
        print("Current working directory: {}.".format(os.getcwd()))
        self.path = './programs/eosio-launcher-service/eosio-launcher-service'
        if not os.path.isfile(self.path):
            print_in_red(
                "ERROR: Launcher service file {} not found!".format(self.path))
        else:
            print_in_green("Found launcher service file {}.".format(self.path))
        self.listen = 1234
        print("HTTP port to listen to: {}.".format(self.listen))
        self.link = 'http://127.0.0.1:1234/v1/launcher'
        self.option = '--http-server-address=0.0.0.0:1234'
        if self.pgrep('eosio-launcher-service'):
            print("Killing existing launcher service ...")
            subprocess.run(['pkill', 'eosio-launcher-service'])
        subprocess.Popen([self.path, self.option])
        self.check_status()

    def parse_args(self):
        parser = argparse.ArgumentParser(description=underlined_str(green_str("Launcher Service for EOS Testing Framework")), add_help=False, formatter_class=lambda prog: argparse.HelpFormatter(prog, max_help_position=50))
        parser.add_argument('-h', '--help', action="help", help="Show this message and exit")
        parser.add_argument('-v', '--verbose', metavar='LEVEL', type=int, default=1, help="Verbosity level")
        parser.add_argument('-s', '--server', type=str, default=LauncherCaller.DEFAULT_SERVER, help="Server for nodeos")
        parser.add_argument('-p', '--port', type=int, default=LauncherCaller.DEFAULT_PORT, help="Port to listen to")
        return parser.parse_args()

    def check_status(self):
        pid = self.pgrep('eosio-launcher-service')
        if pid is None:
            print_in_red("Launcher service is not running!")
        else:
            print_in_green("Launcher service is running with process ID {}".format(pid))

    def rpc(self, procedure: str) -> str:
        return self.link + '/' + procedure

    def describe(self, description, sleep=0):
        print_in_blue(pad(description))
        if sleep:
            time.sleep(sleep)

    def print_response(self, timeout=1):
        print_response(self.response, timeout)

    def print_system_info(self):
        print("{:20s}{}".format("UTC Time", time.strftime(
            "%Y-%m-%d %H:%M:%S", time.gmtime())))
        print("{:20s}{}".format("Local Time", time.strftime(
            "%Y-%m-%d %H:%M:%S", time.localtime())))
        print("{:20s}{}".format("Platform", platform.platform()))
        print("{:20s}{}".format("Server", self.server))
        print("{:20s}{}".format("Port", self.port))

    def launch_cluster(self, data):
        self.response = requests.post(self.rpc('launch_cluster'), data=data)

    def create_bios_accounts(self, data):
        self.response = requests.post(self.rpc('create_bios_accounts'), data=data)

    def create_account(self, data):
        self.response = requests.post(self.rpc('create_account'), data=data)

    def get_account(self, data):
        self.response = requests.post(self.rpc('get_account'), data=data)

    def get_cluster_info(self, data):
        self.response = requests.post(self.rpc('get_cluster_info'), data=data)

    def get_log_data(self, data):
        self.response = requests.post(self.rpc('get_log_data'), data=data)

    def get_protocol_features(self, data):
        self.response = requests.post(self.rpc('get_protocol_features'), data=data)

    def get_cluster_running_state(self, data):
        self.response = requests.post(self.rpc('get_cluster_running_state'), data=data)

    def push_actions(self, data):
        self.response = requests.post(self.rpc('push_actions'), data=data)

    def schedule_protocol_feature_activations(self, data):
        self.response = requests.post(self.rpc('schedule_protocol_feature_activations'), data=data)

    def set_contract(self, data):
        self.response = requests.post(self.rpc('set_contract'), data=data)

    def stop_all_clusters(self):
        self.response = requests.post(self.rpc('stop_all_clusters'), data='')

    def verify_transaction(self, data):
        self.response = requests.post(self.rpc('verify_transaction'), data=data)

    def get_transaction_id(self):
        try:
            return json.loads(self.response.text)['transaction_id']
        except KeyError:
            return

    def pgrep(self, pattern: str) -> str:
        out = subprocess.Popen(['pgrep', pattern], stdout=subprocess.PIPE).stdout.read()
        return [int(x) for x in out.splitlines()]

def main():
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


if __name__ == '__main__':
    main()
