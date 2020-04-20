#!/usr/bin/env python3

# This script tests that the compiled binaries produce expected output in
# response to the `--help` option. It also contains a couple of additional
# CLI-related checks as well as test cases for CLI bugfixes.

from testUtils import Utils

import subprocess
import re


def nodeos_help_test():
    help_text = subprocess.check_output(["./programs/nodeos/nodeos", "--help"])

    if not re.search(b'Application.*Options', help_text):
        raise Exception('No application options found '
                        'in the output of "nodeos --help"')

    if not re.search(b'Options for .*_plugin', help_text):
        raise Exception('No plugin options found '
                        'in the output of "nodeos --help"')


def cleos_help_test(args):
    help_text = subprocess.check_output(["./programs/cleos/cleos"] + args)

    if b'Options:' not in help_text:
        raise Exception('No options found in the '
                        'output of "cleos ' + ' '.join(args))

    if b'Subcommands:' not in help_text:
        raise Exception('No subcommands found in the '
                        'output of cleos ' + ' '.join(args))


def cli11_bugfix_test():
    completed_process = subprocess.run(
        ['./programs/cleos/cleos', '-u', 'http://localhost:0/',
         'push', 'action', 'accout', 'action', '["data"]', '-p', 'wallet'],
        check=False,
        capture_output=True)

    if not completed_process.returncode:
        raise Exception('Test command unexpectedly succeeded')

    if b'Failed to connect to nodeos' not in completed_process.stderr:
        raise Exception('Regression failure: cleos '
                        'failed to parse command line')


try:
    nodeos_help_test()

    cleos_help_test(['--help'])
    cleos_help_test(['system', '--help'])
    cleos_help_test(['version', '--help'])
    cleos_help_test(['wallet', '--help'])

    cli11_bugfix_test()
except Exception as e:
    Utils.errorExit("cli_test: " + str(e))
