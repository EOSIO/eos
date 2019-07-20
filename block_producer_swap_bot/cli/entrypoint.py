"""
Provide implementation of the command line interface to interact with Remme-core.
"""
import click

from cli.eth_swap_contract.cli import eth_swap_contract_commands


@click.group()
@click.version_option()
@click.help_option()
def cli():
    """
    Command-line interface to interact with Remme-core.
    """
    pass


cli.add_command(eth_swap_contract_commands)
