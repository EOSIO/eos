"""
Provide implementation of the eth_swap_bot interfaces.
"""


class EthSwapBotInterface:
    """
    Implements eth_swap_bot interface.
    """

    def process_swaps(self):
        """
        Process swap requests.
        """
        pass

    def manual_process_swap(self, **kwargs):
        """
        Process swap request manually.
        """
        pass
