"""
Provide implementation of the remchain_swap_contract.
"""
import subprocess

from cli.constants import (
    PROCESS_SWAP_ACTION,
    REMME_SWAP_ACCOUNT,
)


class RemchainSwapContract:
    """
    Implements remchain_swap_contract.
    """

    def __init__(self, nodeos_url, permission):
        self.nodeos_url = nodeos_url
        self.permission = permission

    def init_swap(self, **kwargs):

        initiator = self.permission.split('@')[0]
        txid = kwargs.get('txid')
        chain_id = kwargs.get('chain_id')
        swap_pubkey = kwargs.get('swap_pubkey')
        amount = kwargs.get('amount')
        timestamp = kwargs.get('timestamp')

        command = f'cleos --url {self.nodeos_url} push action {REMME_SWAP_ACCOUNT} {PROCESS_SWAP_ACTION} ' \
            f'\'[ "{initiator}", "{txid}", "{chain_id}", "{swap_pubkey}", "{amount}", "{timestamp}" ]\' ' \
            f'-p {self.permission}'
        print(command)

        print(subprocess.call(command, shell=True))
