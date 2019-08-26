#! /usr/bin/env python3

import argparse
import json
import os
import pgrep
import requests
import subprocess
import time

from printer import print_in_blue, print_in_green, print_in_red, print_response, pad

VERBOSITY = 0

class LauncherCaller:
    def __init__(self):
        self.directory = os.path.dirname(os.path.realpath(__file__))
        os.chdir(os.path.join(self.directory, '../build'))
        print("Current working directory: {}.".format(os.getcwd()))
        self.path = './programs/eosio-launcher-service/eosio-launcher-service'
        if not os.path.isfile(self.path):
            print_in_red("ERROR: Launcher service file {} not found!".format(self.path))
        else:
            print_in_green("Found launcher service file {}.".format(self.path))
        self.listen = 1234
        print("HTTP port to listen to: {}.".format(self.listen))
        self.link = 'http://127.0.0.1:1234/v1/launcher'
        self.option = '--http-server-address=0.0.0.0:1234'
        if pgrep.pgrep('eosio-launcher-service'):
            print("Killing existing launcher service ...")
            subprocess.run(['pkill', 'eosio-launcher-service'])
        subprocess.Popen([self.path, self.option])
        self.check_status()

    def check_status(self):
        pid = pgrep.pgrep('eosio-launcher-service')
        if pid is None:
            print_in_red("Launcher service is not running!")
        else:
            print_in_green("Launcher service is running with process ID {}".format(pid))

    def rpc(self, procedure : str) -> str:
        return self.link + '/' + procedure


class Cluster:
    def __init__(self, caller: LauncherCaller, data: str):
        self.caller = caller
        self.data = data

    def launch(self) -> requests.Response:
        return requests.post(self.caller.rpc('launch_cluster'), data=self.data)

    def get_info(self, data):
        return requests.post(self.caller.rpc('get_cluster_info'), data=data)

    def create_bios_accounts(self, data):
        return requests.post(self.caller.rpc('create_bios_accounts'), data=data)

    def get_account(self, data):
        return requests.post(self.caller.rpc('get_account'), data=data)

    def get_cluster_running_state(self, data):
        return requests.post(self.caller.rpc('get_cluster_running_state'), data=data)

    def set_contract(self, data):
        return requests.post(self.caller.rpc('set_contract'), data=data)

    def verify_transaction(self, data):
        return requests.post(self.caller.rpc('verify_transaction'), data=data)


class Tester:
    @staticmethod
    def start(description, sleep=0):
        print_in_blue(pad(description))
        if sleep:
            time.sleep(sleep)

    def load(self, response: requests.Response):
        self.response = response

    def print(self, timeout=1):
        print_response(self.response, timeout, verbosity=VERBOSITY)

    def get_transaction_id(self):
        try:
            return json.loads(self.response.text)['transaction_id']
        except KeyError:
            return

def parse():
    parser = argparse.ArgumentParser(description="Description Here", add_help=True, formatter_class=lambda prog: argparse.HelpFormatter(prog,max_help_position=50))
    parser.add_argument('-v', '--verbosity', metavar='LEVEL', type=int, default=1, help="verbosity level")

    result = parser.parse_args()
    global VERBOSITY
    VERBOSITY = result.verbosity

    return result

def main():
    args = parse()

    caller = LauncherCaller()
    tester = Tester()

    tester.start("launching a cluster of nodes")
    cluster = Cluster(caller, '{"node_count":4,"cluster_id":2,"nodes":[{"node_id":0,"producers":["eosio","inita"]}, {"node_id":1,"producers":["initb"]}]}')
    tester.load(cluster.launch())
    tester.print()

    tester.start("getting cluster information", sleep=1)
    tester.load(cluster.get_info('2'))
    tester.print()

    tester.start("getting cluster running state", sleep=1)
    tester.load(cluster.get_cluster_running_state('2'))
    tester.print()

    tester.start("creating system account", sleep=1)
    tester.load(cluster.create_bios_accounts('{"cluster_id":2,"node_id":0,"creator":"eosio","accounts":[{"name":"inita","owner":"EOS8kE63z4NcZatvVWY4jxYdtLg6UEA123raMGwS6QDKwpQ69eGcP","active":"EOS8kE63z4NcZatvVWY4jxYdtLg6UEA123raMGwS6QDKwpQ69eGcP"}]}'))
    tester.print()

    tester.start("getting account information", sleep=1)
    tester.load(cluster.get_account('{"cluster_id":2,"node_id":0,"name":"inita"}'))
    tester.print()

    tester.start("setting system contract", sleep=1)
    tester.load(cluster.set_contract('{"cluster_id":2,"node_id":0,"account":"eosio","contract_file":"../../contracts/build/contracts/eosio.system/eosio.system.wasm","abi_file":"../../contracts/build/contracts/eosio.system/eosio.system.abi"}'))
    tester.print()

    tid = tester.get_transaction_id()
    if tid:
        print("Getting transaction ID {}".format(tid))
        tester.start("verifying transaction", sleep=1)
        tester.load(cluster.verify_transaction('{{"cluster_id":2,"node_id":0,"transaction_id":"{}"}}'.format(tid)))
        tester.print()
    else:
        print("No transaction ID returned.")

    if not tid:
        tester.start("setting system contract", sleep=1)
        tester.load(cluster.set_contract('{"cluster_id":2,"node_id":0,"account":"eosio","contract_file":"../../contracts/build/contracts/eosio.system/eosio.system.wasm","abi_file":"../../contracts/build/contracts/eosio.system/eosio.system.abi"}'))
        tester.print()

        tid = tester.get_transaction_id()
        if tid:
            print("Getting transaction ID {}".format(tid))
            tester.start("verifying transaction", sleep=1)
            tester.load(cluster.verify_transaction('{{"cluster_id":2,"node_id":0,"transaction_id":"{}"}}'.format(tid)))
            tester.print()
        else:
            print("No transaction ID returned.")

if __name__ == '__main__':
    main()
