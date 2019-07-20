"""
Provide utils for command line interface.
"""
import json

#
import click


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
