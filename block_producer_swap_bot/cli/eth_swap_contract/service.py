"""
Provide implementation of the eth_swap_contract.
"""
import time
from threading import Thread

from accessify import implements
from web3 import Web3

from cli.constants import (
    ETH_SWAP_CONTRACT_ABI,
    ETH_SWAP_CONTRACT_ADDRESS,
    SHORT_POLLING_INTERVAL,
    ETH_SWAP_REQUEST_EVENT_NAME)
from cli.eth_swap_contract.interfaces import EthSwapContractInterface
from cli.utils import print_result


@implements(EthSwapContractInterface)
class EthSwapContract:
    """
    Implements eth_swap_contract.
    """

    def __init__(self, eth_provider, nodeos_url, permission):
        self.eth_provider = eth_provider
        self.nodeos_url = nodeos_url
        self.permission = permission

        self.web3 = Web3(Web3.WebsocketProvider(eth_provider))

        self.eth_swap_contract = self.web3.eth.contract(
            address=ETH_SWAP_CONTRACT_ADDRESS,
            abi=ETH_SWAP_CONTRACT_ABI
        )

    @staticmethod
    def get_swap_request_parameters(event):
        event_args = event['args']
        transaction_id = str(event['transactionHash'].hex())
        chain_id = str(event_args['chainId'])
        user_public_key = str(event_args['userPublicKey'])
        amount_to_swap = int(event_args['amountToSwap'])
        timestamp = int(event_args['timestamp'])

        result = {
            'txid': transaction_id,
            'chain_id': chain_id,
            'user_public_key': user_public_key,
            'amount_to_swap': amount_to_swap,
            'timestamp': timestamp,
        }

        return result

    def handle_swap_request_event(self, event):
        result = self.get_swap_request_parameters(event)

        print_result(result)

        # call eos contract to create swap request

    def new_swaps_loop(self, event_filter, poll_interval):
        while True:
            for event in event_filter.get_new_entries():
                self.handle_swap_request_event(event)
            time.sleep(poll_interval)

    def sync_swaps_loop(self, event_filter):
        for event in event_filter.get_all_entries():
            self.handle_swap_request_event(event)

    def process_swaps(self):
        """
        Process swap requests.
        """
        request_swap_event_filter = self.eth_swap_contract.eventFilter(
            ETH_SWAP_REQUEST_EVENT_NAME,
            {'fromBlock': 0, 'toBlock': 'latest'}
        )

        self.sync_swaps_loop(request_swap_event_filter)

        worker = Thread(
            target=self.new_swaps_loop,
            args=(request_swap_event_filter, SHORT_POLLING_INTERVAL),
            daemon=True
        )
        worker.start()
        worker.join()

    def manual_process_swap(self, **kwargs):
        """
        Process swap request manually.
        """
        print(kwargs)
        # call eos contract to create swap request
