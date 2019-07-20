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
)
from cli.eth_swap_contract.interfaces import EthSwapContractInterface
from cli.utils import print_result


@implements(EthSwapContractInterface)
class EthSwapContract:
    """
    Implements eth_swap_contract.
    """

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

    @staticmethod
    def handle_swap_request_event(event):
        result = EthSwapContract.get_swap_request_parameters(event)

        print_result(result)

        # call eos contract to create swap request

    @staticmethod
    def new_swaps_loop(event_filter, poll_interval):
        while True:
            for event in event_filter.get_new_entries():
                EthSwapContract.handle_swap_request_event(event)
            time.sleep(poll_interval)

    @staticmethod
    def sync_swaps_loop(event_filter):
        for event in event_filter.get_all_entries():
            EthSwapContract.handle_swap_request_event(event)

    def process_swaps(self, eth_provider):
        """
        Process swap requests.
        """
        web3 = Web3(Web3.WebsocketProvider(eth_provider))

        eth_swap_contract = web3.eth.contract(
            address=ETH_SWAP_CONTRACT_ADDRESS,
            abi=ETH_SWAP_CONTRACT_ABI
        )

        request_swap_event_filter = eth_swap_contract.eventFilter(
            'SwapRequest',
            {'fromBlock': 0, 'toBlock': 'latest'}
        )

        self.sync_swaps_loop(request_swap_event_filter)

        worker = Thread(
            target=EthSwapContract.new_swaps_loop,
            args=(request_swap_event_filter, SHORT_POLLING_INTERVAL),
            daemon=True
        )
        worker.start()
        worker.join()

    def manual_process_swap(self, txid, chain_id, user_public_key, amount_to_swap, timestamp, eth_provider):
        """
        Process swap request manually.
        """

        # call eos contract to create swap request
