"""
Provide implementation of the command line interface's remchain_swap_contract commands.
"""
import click

from cli.eth_swap_bot.help import PERMISSION_ARGUMENT_HELP_MESSAGE
from cli.generic.help import (
    ETH_PROVIDER_ARGUMENT_HELP_MESSAGE,
    NODEOS_URL_ARGUMENT_HELP_MESSAGE,
)
from cli.remchain_swap_contract.help import (
    ACCOUNT_NAME_TO_CREATE_ARGUMENT_HELP_MESSAGE,
    CHAIN_ID_ARGUMENT_HELP_MESSAGE,
    RECEIVER_ARGUMENT_HELP_MESSAGE,
    SWAP_AMOUNT_ARGUMENT_HELP_MESSAGE,
    SWAP_PUBKEY_ARGUMENT_HELP_MESSAGE,
    SWAP_REUQEST_TXID_ARGUMENT_HELP_MESSAGE,
    SWAP_SIGNATURE_ARGUMENT_HELP_MESSAGE,
    TIMESTAMP_ARGUMENT_HELP_MESSAGE,
)
from cli.remchain_swap_contract.service import RemchainSwapContract
from cli.utils import (
    get_block_producer_permission,
    get_default_eth_provider,
    get_default_nodeos_url,
)


@click.group('remchain-swap-contract', chain=True)
def remchain_swap_contract_commands():
    """
    Provide commands for working with remchain-swap-contract.
    """
    pass


@remchain_swap_contract_commands.command('finish-swap')
@click.option('--receiver', type=str, required=True, help=RECEIVER_ARGUMENT_HELP_MESSAGE)
@click.option('--account-name-to-create', type=str, required=True, help=ACCOUNT_NAME_TO_CREATE_ARGUMENT_HELP_MESSAGE)
@click.option('--txid', type=str, required=True, help=SWAP_REUQEST_TXID_ARGUMENT_HELP_MESSAGE)
@click.option('--chain-id', type=str, required=True, help=CHAIN_ID_ARGUMENT_HELP_MESSAGE)
@click.option('--swap-pubkey', type=str, required=True, help=SWAP_PUBKEY_ARGUMENT_HELP_MESSAGE)
@click.option('--signature', type=str, required=True, help=SWAP_SIGNATURE_ARGUMENT_HELP_MESSAGE)
@click.option('--amount', type=int, required=True, help=SWAP_AMOUNT_ARGUMENT_HELP_MESSAGE)
@click.option('--timestamp', type=str, required=True, help=TIMESTAMP_ARGUMENT_HELP_MESSAGE)
@click.option('--nodeos-url', type=str, required=True, help=NODEOS_URL_ARGUMENT_HELP_MESSAGE,
              default=get_default_nodeos_url())
@click.option('--eth-provider', type=str, required=True, help=ETH_PROVIDER_ARGUMENT_HELP_MESSAGE,
              default=get_default_eth_provider())
@click.option('--permission', type=str, required=True, help=PERMISSION_ARGUMENT_HELP_MESSAGE,
              default=get_block_producer_permission())
def finish_swap(**kwargs):
    """
    Finish swap on Remchain.
    """
    rem_swap_contract = RemchainSwapContract(
        nodeos_url=kwargs.get('nodeos_url'),
        permission=kwargs.get('permission'),
    )

    finish_swap_data = {
        'receiver': kwargs.get('receiver'),
        'account-name-to-create': kwargs.get('account_name_to_create'),
        'txid': kwargs.get('txid'),
        'chain-id': kwargs.get('chain_id'),
        'swap-pubkey': kwargs.get('swap_pubkey'),
        'signature': kwargs.get('signature'),
        'amount': kwargs.get('amount'),
        'timestamp': kwargs.get('timestamp'),
    }

    rem_swap_contract.finish_swap(**finish_swap_data)
