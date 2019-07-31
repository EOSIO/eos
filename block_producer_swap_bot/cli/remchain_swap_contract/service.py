"""
Provide implementation of the remchain_swap_contract.
"""
import subprocess

from cli.constants import (
    FINISH_SWAP_ACTION,
    PROCESS_SWAP_ACTION,
    REM_SWAP_ACCOUNT,
)


class RemchainSwapContract:
    """
    Implements remchain_swap_contract.
    """

    def __init__(self, cleos, permission):
        """
        Constructor.

        Arguments:
            cleos (string, required): cleos script path with options
            permission (string, required): a permission to sign process swap transactions
        """
        self.cleos = cleos
        self.permission = permission

    def process_swap(self, **kwargs):
        """
        Initialize swap on Remchain.

        Arguments:
            initiator (string, required): sender of initialize swap action
            txid (string, required): an identifier of swap request transaction on the source blockchain
            chain_id (string, required): an identifier of destination blockchain
            swap_pubkey (string, required): a public key to verify signature with receiver and active, owner public keys
            amount (int, required): amount of tokens to swap
            timestamp (string, required): timestamp in UTC format of request swap transaction
                                          on the source blockchain (yyyy-mm-ddThh:mm:ss)

        """

        initiator = self.permission.split('@')[0]
        txid = kwargs.get('txid')
        chain_id = kwargs.get('chain_id')
        swap_pubkey = kwargs.get('swap_pubkey')
        amount = kwargs.get('amount')
        timestamp = kwargs.get('timestamp')

        command = f'{self.cleos} push action {REM_SWAP_ACCOUNT} {PROCESS_SWAP_ACTION} ' \
            f'\'[ "{initiator}", "{txid}", "{chain_id}", "{swap_pubkey}", "{amount}", "{timestamp}" ]\' ' \
            f'-p {self.permission}'
        print(command)
        return subprocess.call(command, shell=True)

    def finish_swap(self, **kwargs):
        """
        Finish swap on Remchain.

        Arguments:
            receiver (string, required): receiver of swapped tokens on Remchain
            txid (string, required): an identifier of swap request transaction on the source blockchain
            chain_id (string, required): an identifier of destination blockchain
            swap_pubkey (string, required): a public key to verify signature with receiver and account name to create
            amount (int, required): amount of tokens to swap
            timestamp (string, required): timestamp of request swap transaction
                                          on the source blockchain (yyyy-mm-ddThh:mm:ss)
            signature (string, required): a signature of receiver and active and owner keys
            active (string, required): a public key to authorize active permission
            owner (string, required): a public key to authorize owner permission
        """

        receiver = kwargs.get('receiver')
        txid = kwargs.get('txid')
        chain_id = kwargs.get('chain-id')
        swap_pubkey = kwargs.get('swap-pubkey')
        amount = kwargs.get('amount')
        timestamp = kwargs.get('timestamp')
        signature = kwargs.get('signature')
        active = kwargs.get('active')
        owner = kwargs.get('owner')

        command = f'{self.cleos} push action {REM_SWAP_ACCOUNT} {FINISH_SWAP_ACTION} ' \
            f'\'[ "{receiver}", "{txid}", "{chain_id}", "{swap_pubkey}",' \
            f'"{amount}", "{timestamp}", "{signature}", "{active}", "{owner}" ]\' ' \
            f'-p {self.permission}'
        print(command)
        return subprocess.call(command, shell=True)
