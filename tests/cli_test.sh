#!/bin/sh

# This script tests that the compiled binaries produce expected output in
# response to the `--help` option. It also contains a couple of additional
# CLI-related checks as well as test cases for CLI bugfixes.

set -e

script_name="`basename "$0"`"

tmp_file="`mktemp`"

cleanup_and_exit() {
    exit_code=$1

    rm -f "$tmp_file"

    trap - EXIT
    exit $exit_code
}

error_exit() {
    test -n "$2" && exit_code="$2" || exit_code=1
    echo "$script_name: $1" 1>&2
    echo 'Last output:' 1>&2
    cat "$tmp_file" 1>&2
    cleanup_and_exit "$exit_code"
}

trap 'cleanup_and_exit 1' EXIT
trap 'cleanup_and_exit 130' INT
trap 'cleanup_and_exit 143' TERM

nodeos_help_test() {
    (
        ./programs/nodeos/nodeos --help > "$tmp_file"

        while read line; do
            case "$line" in
            'Application'*'Options:') app_opt_found=true ;;
            *'Options for '*'_plugin:') plug_opt_found=true
            esac
        done < "$tmp_file"

        if test -z "$app_opt_found"; then
            error_exit 'No application options found in "nodeos --help"'
        fi

        if test -z "$plug_opt_found"; then
            error_exit 'No plugin options found in "nodeos --help"'
        fi
    )
}

cleos_help_test() {
    (
        ./programs/cleos/cleos "$@" > "$tmp_file"

        while read line; do
            case "$line" in
            'Options:') opt_found=true ;;
            'Subcommands:') subcommands_found=true
            esac
        done < "$tmp_file"

        if test -z "$opt_found"; then
            error_exit "No options found in the output of 'cleos $*'"
        fi

        if test -z "$subcommands_found"; then
            error_exit "No subcommands found in the output of 'cleos $*'"
        fi
    )
}

cli11_bugfix_test() {
    if ./programs/cleos/cleos -u http://localhost:0/ push action \
            accout action '["data"]' -p wallet > "$tmp_file" 2>&1; then
        error_exit 'Test command unexpectedly succeeded'
    fi
    if ! grep -q 'Failed to connect to nodeos' "$tmp_file"; then
        error_exit 'Regression failure: cleos failed to parse command line'
    fi
}

nodeos_help_test
cleos_help_test '--help'
cleos_help_test 'system' '--help'
cleos_help_test 'version' '--help'
cleos_help_test 'wallet' '--help'
cli11_bugfix_test

cleanup_and_exit 0
