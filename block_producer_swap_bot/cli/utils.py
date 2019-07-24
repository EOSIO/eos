"""
Provide utils for command line interface.
"""
import json

import click

from cli.config import ConfigFile

config_file = ConfigFile()


def dict_to_pretty_json(data):
    return json.dumps(data, indent=4, sort_keys=True)


def print_result(result):
    """
    Print successful result to the terminal.
    """
    return click.echo(dict_to_pretty_json({'result': result}))


def print_errors(errors):
    """
    Print error messages to the terminal.

    Arguments:
        errors (string or dict): dictionary with error messages.

    References:
        - https://click.palletsprojects.com/en/7.x/utils/#ansi-colors
    """
    click.secho(dict_to_pretty_json({'errors': errors}), blink=True, bold=True, fg='red')


def get_default_nodeos_url():
    """
    Get default nodeos-url from config file.
    """
    return config_file.get_nodeos_url()


def get_default_eth_provider():
    """
    Get default ethereum provider from config file.
    """
    return config_file.get_eth_provider()


def get_block_producer_permission():
    """
    Get block producer permission to sign process-swap transactions.
    """
    return config_file.get_block_producer_permission()
