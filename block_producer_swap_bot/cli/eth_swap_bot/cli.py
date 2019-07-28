"""
Provide implementation of the command line interface's eth_swap_bot commands.
"""
import click

from cli.eth_swap_bot.help import (
    CHAIN_ID_ARGUMENT_HELP_MESSAGE,
    ETH_TIMESTAMP_ARGUMENT_HELP_MESSAGE,
    PERMISSION_ARGUMENT_HELP_MESSAGE,
    SWAP_PUBLIC_KEY_ARGUMENT_HELP_MESSAGE,
    TXID_ARGUMENT_HELP_MESSAGE,
)
from cli.eth_swap_bot.service import EthSwapBot
from cli.generic.help import (
    AMOUNT_TO_SWAP_ARGUMENT_HELP_MESSAGE,
    ETH_PROVIDER_ARGUMENT_HELP_MESSAGE,
    CLEOS_ARGUMENT_HELP_MESSAGE,
)
from cli.utils import (
    get_block_producer_permission,
    get_default_eth_provider,
    get_default_cleos,
)


@click.group('eth-swap-bot', chain=True)
def eth_swap_bot_commands():
    """
    Provide commands for working with eth_swap_bot.
    """
    pass


@eth_swap_bot_commands.command('process-swaps')
@click.option('--eth-provider', type=str, required=True, help=ETH_PROVIDER_ARGUMENT_HELP_MESSAGE,
              default=get_default_eth_provider())
@click.option('--cleos', type=str, required=True, help=CLEOS_ARGUMENT_HELP_MESSAGE,
              default=get_default_cleos())
@click.option('--permission', type=str, required=True, help=PERMISSION_ARGUMENT_HELP_MESSAGE,
              default=get_block_producer_permission())
def process_swaps(eth_provider, cleos, permission):
    """
    Process swap requests.
    """
    eth_swap_bot = EthSwapBot(
        eth_provider,
        cleos,
        permission,
    )

    eth_swap_bot.process_swaps()


@eth_swap_bot_commands.command('manual-process-swap')
@click.option('--txid', type=str, required=True, help=TXID_ARGUMENT_HELP_MESSAGE)
@click.option('--chain-id', type=str, required=True, help=CHAIN_ID_ARGUMENT_HELP_MESSAGE)
@click.option('--swap-pubkey', type=str, required=True, help=SWAP_PUBLIC_KEY_ARGUMENT_HELP_MESSAGE)
@click.option('--amount', type=str, required=True, help=AMOUNT_TO_SWAP_ARGUMENT_HELP_MESSAGE)
@click.option('--timestamp', type=str, required=True, help=ETH_TIMESTAMP_ARGUMENT_HELP_MESSAGE)
@click.option('--eth-provider', type=str, required=True, help=ETH_PROVIDER_ARGUMENT_HELP_MESSAGE,
              default=get_default_eth_provider())
@click.option('--cleos', type=str, required=True, help=CLEOS_ARGUMENT_HELP_MESSAGE,
              default=get_default_cleos())
@click.option('--permission', type=str, required=True, help=PERMISSION_ARGUMENT_HELP_MESSAGE,
              default=get_block_producer_permission())
def manual_process_swap(**kwargs):
    """
    Process swap request manually.
    """
    eth_swap_bot = EthSwapBot(
        eth_provider=kwargs.get('eth_provider'),
        cleos=kwargs.get('cleos'),
        permission=kwargs.get('permission'),
    )

    swap_parameters = {
        'txid': kwargs.get('txid'),
        'chain_id': kwargs.get('chain_id'),
        'swap_pubkey': kwargs.get('swap_pubkey'),
        'amount': kwargs.get('amount'),
        'timestamp': kwargs.get('timestamp'),
    }

    eth_swap_bot.manual_process_swap(**swap_parameters)
