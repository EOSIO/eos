"""
Provide implementation of the eth_swap_contract interfaces.
"""


class EthSwapContractInterface:
    """
    Implements eth_swap_contract interface.
    """

    def process_swaps(self):
        """
        Process swap requests.
        """
        pass

    def manual_process_swap(self, **kwargs):
        """
        Process swap requests manually.
        """
        pass
