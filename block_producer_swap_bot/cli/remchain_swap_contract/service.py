"""
Provide implementation of the remchain_swap_contract.
"""
import subprocess

from cli.constants import (
    FINISH_SWAP_ACTION,
    PROCESS_SWAP_ACTION,
    REMME_SWAP_ACCOUNT,
)


class RemchainSwapContract:
    """
    Implements remchain_swap_contract.
    """

    def __init__(self, nodeos_url, permission):
        """
        Constructor.

        Arguments:
            nodeos_url (string, required): an address of nodeos url
            permission (string, required): a permission to sign process swap transactions
        """
        self.nodeos_url = nodeos_url
        self.permission = permission

    def process_swap(self, **kwargs):
        """
        Initialize swap on Remchain.

        Arguments:
            initiator (string, required): sender of initialize swap action
            txid (string, required): an identifier of swap request transaction on the source blockchain
            chain_id (string, required): an identifier of destination blockchain
            swap_pubkey (string, required): a public key to verify signature with receiver and account name to create
            amount (int, required): amount of tokens to swap
            timestamp (string, required): timestamp of request swap transaction on the source blockchain (yyyy-mm-dd)

        """

        initiator = self.permission.split('@')[0]
        txid = kwargs.get('txid')
        chain_id = kwargs.get('chain_id')
        swap_pubkey = kwargs.get('swap_pubkey')
        amount = kwargs.get('amount')
        timestamp = kwargs.get('timestamp')

        command = f'cleos --url {self.nodeos_url} push action {REMME_SWAP_ACCOUNT} {PROCESS_SWAP_ACTION} ' \
            f'\'[ "{initiator}", "{txid}", "{chain_id}", "{swap_pubkey}", "{amount}", "{timestamp}" ]\' ' \
            f'-p {self.permission} -j'

        res = subprocess.call(command, shell=True)

        return res

    def finish_swap(self, **kwargs):
        """
        Finish swap on Remchain.

        Arguments:
            receiver (string, required): receiver of swapped tokens on Remchain
            account_name_to_create (string, required): an account name to create on Remchain
            txid (string, required): an identifier of swap request transaction on the source blockchain
            chain_id (string, required): an identifier of destination blockchain
            swap_pubkey (string, required): a public key to verify signature with receiver and account name to create
            signature (string, required): a signature of receiver and account name to create
            amount (int, required): amount of tokens to swap
            timestamp (string, required): timestamp of request swap transaction on the source blockchain (yyyy-mm-dd)

        """

        receiver = kwargs.get('receiver')
        account_name_to_create = kwargs.get('account_name_to_create')
        txid = kwargs.get('txid')
        chain_id = kwargs.get('chain-id')
        swap_pubkey = kwargs.get('swap-pubkey')
        signature = kwargs.get('signature')
        amount = kwargs.get('amount')
        timestamp = kwargs.get('timestamp')

        command = f'cleos --url {self.nodeos_url} push action {REMME_SWAP_ACCOUNT} {FINISH_SWAP_ACTION} ' \
            f'\'[ "{receiver}", "{account_name_to_create}", "{txid}", "{chain_id}", "{swap_pubkey}",' \
            f'"{signature}", "{amount}", "{timestamp}" ]\' ' \
            f'-p {self.permission}'

        subprocess.call(command, shell=True)
