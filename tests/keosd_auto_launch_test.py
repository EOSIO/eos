#!/usr/bin/env python3

# This script tests that cleos launches keosd automatically when keosd is not
# running yet.

import subprocess


def run_cleos_wallet_command(command: str, no_auto_keosd: bool):
    """Run the given cleos command and return subprocess.CompletedProcess."""
    args = ['./programs/cleos/cleos']

    if no_auto_keosd:
        args.append('--no-auto-keosd')

    args += 'wallet', command

    return subprocess.run(args,
                          check=False,
                          stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL)


def stop_keosd():
    """Stop the default keosd instance."""
    run_cleos_wallet_command('stop', no_auto_keosd=True)


def keosd_auto_launch_test():
    """Test that keos auto-launching works but can be optionally inhibited."""
    stop_keosd()

    # Make sure that when '--no-auto-keosd' is given, keosd is not started by
    # cleos.
    assert run_cleos_wallet_command('list',
                                    no_auto_keosd=True).returncode != 0

    # Verify that keosd auto-launching works.
    assert run_cleos_wallet_command('list',
                                    no_auto_keosd=False).returncode == 0


try:
    keosd_auto_launch_test()
finally:
    stop_keosd()
