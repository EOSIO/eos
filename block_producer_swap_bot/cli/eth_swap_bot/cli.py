"""
Provide implementation of the command line interface's eth_swap_bot commands.
"""
import click

from cli.eth_swap_bot.service import EthSwapBot
from cli.generic.help import (
    ETH_PROVIDER_ARGUMENT_HELP_MESSAGE,
    CLEOS_ARGUMENT_HELP_MESSAGE,
    PERMISSION_ARGUMENT_HELP_MESSAGE)
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
