"""
Provide implementation of the command line interface's eth_swap_contract commands.
"""
import click

from cli.eth_swap_contract.help import (
    AMOUNT_TO_SWAP_ARGUMENT_HELP_MESSAGE,
    CHAIN_ID_ARGUMENT_HELP_MESSAGE,
    ETH_PROVIDER_ARGUMENT_HELP_MESSAGE,
    TIMESTAMP_ARGUMENT_HELP_MESSAGE,
    TXID_ARGUMENT_HELP_MESSAGE,
    USER_PUBLIC_KEY_ARGUMENT_HELP_MESSAGE,
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
def process_swaps(eth_provider):
    """
    Process swap requests.
    """
    eth_swap_contract = EthSwapContract()

    eth_swap_contract.process_swaps(eth_provider)


@eth_swap_contract_commands.command('manual-confirm-swap')
@click.option('--txid', type=str, required=True, help=TXID_ARGUMENT_HELP_MESSAGE)
@click.option('--chain-id', type=str, required=True, help=CHAIN_ID_ARGUMENT_HELP_MESSAGE)
@click.option('--user-public-key', type=str, required=True, help=USER_PUBLIC_KEY_ARGUMENT_HELP_MESSAGE)
@click.option('--amount-to-swap', type=int, required=True, help=AMOUNT_TO_SWAP_ARGUMENT_HELP_MESSAGE)
@click.option('--timestamp', type=int, required=True, help=TIMESTAMP_ARGUMENT_HELP_MESSAGE)
@click.option('--eth-provider', type=str, required=True, help=ETH_PROVIDER_ARGUMENT_HELP_MESSAGE)
def manual_process_swap(txid, chain_id, user_public_key, amount_to_swap, timestamp, eth_provider):
    """
    Process swap request manually.
    """
    eth_swap_contract = EthSwapContract()

    eth_swap_contract.manual_process_swap(
        txid,
        chain_id,
        user_public_key,
        amount_to_swap,
        timestamp,
        eth_provider
    )
