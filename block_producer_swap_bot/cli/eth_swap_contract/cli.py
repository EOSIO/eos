"""
Provide implementation of the command line interface's eth_swap_contract commands.
"""
import click

from cli.eth_swap_contract.help import (
    AMOUNT_TO_SWAP_ARGUMENT_HELP_MESSAGE,
    CHAIN_ID_ARGUMENT_HELP_MESSAGE,
    ETH_PROVIDER_ARGUMENT_HELP_MESSAGE,
    NODEOS_URL_ARGUMENT_HELP_MESSAGE,
    TIMESTAMP_ARGUMENT_HELP_MESSAGE,
    TXID_ARGUMENT_HELP_MESSAGE,
    SWAP_PUBLIC_KEY_ARGUMENT_HELP_MESSAGE,
    PERMISSION_ARGUMENT_HELP_MESSAGE,
)
from cli.eth_swap_contract.service import EthSwapContract


@click.group('eth-swap-contract', chain=True)
def eth_swap_contract_commands():
    """
    Provide commands for working with eth_swap_contract.
    """
    pass


@eth_swap_contract_commands.command('process-swaps')
@click.option('--eth-provider', type=str, required=True, help=ETH_PROVIDER_ARGUMENT_HELP_MESSAGE)
@click.option('--nodeos-url', type=str, required=True, help=NODEOS_URL_ARGUMENT_HELP_MESSAGE)
@click.option('--permission', type=str, required=True, help=PERMISSION_ARGUMENT_HELP_MESSAGE)
def process_swaps(eth_provider, nodeos_url, permission):
    """
    Process swap requests.
    """
    eth_swap_contract = EthSwapContract(
        eth_provider,
        nodeos_url,
        permission,
    )

    eth_swap_contract.process_swaps()


@eth_swap_contract_commands.command('manual-process-swap')
@click.option('--txid', type=str, required=True, help=TXID_ARGUMENT_HELP_MESSAGE)
@click.option('--chain-id', type=str, required=True, help=CHAIN_ID_ARGUMENT_HELP_MESSAGE)
@click.option('--swap-pubkey', type=str, required=True, help=SWAP_PUBLIC_KEY_ARGUMENT_HELP_MESSAGE)
@click.option('--amount', type=int, required=True, help=AMOUNT_TO_SWAP_ARGUMENT_HELP_MESSAGE)
@click.option('--timestamp', type=int, required=True, help=TIMESTAMP_ARGUMENT_HELP_MESSAGE)
@click.option('--eth-provider', type=str, required=True, help=ETH_PROVIDER_ARGUMENT_HELP_MESSAGE)
@click.option('--nodeos-url', type=str, required=True, help=NODEOS_URL_ARGUMENT_HELP_MESSAGE)
@click.option('--permission', type=str, required=True, help=PERMISSION_ARGUMENT_HELP_MESSAGE)
def manual_process_swap(**kwargs):
    """
    Process swap request manually.
    """
    eth_swap_contract = EthSwapContract(
        eth_provider=kwargs.get('eth_provider'),
        nodeos_url=kwargs.get('nodeos_url'),
        permission=kwargs.get('permission'),
    )

    swap_parameters = {
        'txid': kwargs.get('txid'),
        'chain_id': kwargs.get('chain_id'),
        'swap_pubkey': kwargs.get('swap_pubkey'),
        'amount': kwargs.get('amount'),
        'timestamp': kwargs.get('timestamp'),
    }

    eth_swap_contract.manual_process_swap(**swap_parameters)
