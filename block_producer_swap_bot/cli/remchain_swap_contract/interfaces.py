"""
Provide implementation of the remchain_swap_contract interfaces.
"""


class RemchainSwapContractInterface:
    """
    Implements remchain_swap_contract interface.
    """

    def init_swap(self, **kwargs):
        """
        Push initswap action to rem.swap account.
        """
        pass

    def finish_swap(self, **kwargs):
        """
        Push finishswap action to rem.swap account.
        """
        pass
