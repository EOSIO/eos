"""
Provide implementation of the eth_swap_contract interfaces.
"""


class EthSwapContractInterface:
    """
    Implements eth_swap_contract interface.
    """

    def process_swaps(self, eth_provider):
        """
        Process swap requests.
        """
        pass

    def manual_process_swap(self, txid, chain_id, user_public_key, amount_to_swap, timestamp, eth_provider):
        """
        Process swap requests manually.
        """
        pass
