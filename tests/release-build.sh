#!/bin/bash
# test name and purpose
echo ''
echo '                         ##### Release Build Test #####'
echo ''
echo '    The purpose of this test is to ensure that nodeos was built with compiler'
echo 'optimizations enabled. While there is no way to programmatically determine that'
echo 'given one binary, we do set a debug flag in nodeos when it is built with'
echo 'asserts. This test checks that debug flag. Anyone intending to build and install'
echo 'nodeos from source should perform a "release build" which excludes asserts and'
echo 'debugging symbols, and performs compiler optimizations.'
echo ''
# check for jq
if ! $(jq --version 1>/dev/null); then
    echo 'ERROR: Test requires jq, but jq was not found in your PATH!'
    echo ''
    exit 1
fi
# find nodeos
[[ -z "$EOSIO_ROOT" && $(git --version) ]] && export EOSIO_ROOT="$(git rev-parse --show-toplevel)"
[[ -z "$EOSIO_ROOT" ]] && export EOSIO_ROOT="$(echo $(pwd)/ | grep -ioe '.*/eos/' -e '.*/eosio/' -e '.*/build/' | sed 's,/build/,/,')"
if [[ ! -f "$EOSIO_ROOT/build/bin/nodeos" && ! -f "$EOSIO_ROOT/build/programs/nodeos/nodeos" ]]; then
    echo 'ERROR: nodeos binary not found!'
    echo ''
    echo 'Looked in the following places:'
    echo "$ ls -la \"$EOSIO_ROOT/build/bin\""
    ls -la "$EOSIO_ROOT/build/bin"
    echo "$ ls -la \"$EOSIO_ROOT/build/programs/nodeos\""
    ls -la "$EOSIO_ROOT/build/programs/nodeos"
    echo 'Release build test not run.'
    exit 1
fi
[[ -f "$EOSIO_ROOT/build/bin/nodeos" ]] && cd "$EOSIO_ROOT/build/bin" || cd "$EOSIO_ROOT/build/programs/nodeos"
# run nodeos to generate state files
./nodeos --extract-build-info build-info.json 1>/dev/null 2>/dev/null
if [[ ! -f build-info.json ]]; then
    echo 'ERROR: Build info JSON file not found!'
    echo ''
    echo 'Looked in the following places:'
    echo "$ ls -la \"$(pwd)\""
    ls -la "$(pwd)"
    echo 'Release build test not run.'
    exit 2
fi
# test state files for debug flag
if [[ "$(cat build-info.json | jq .debug)" == 'false' ]]; then
    echo 'PASS: Debug flag is not set.'
    echo ''
    exit 0
fi
echo 'FAIL: Debug flag is set!'
echo ''
echo '$ cat build-info.json | jq .'
cat build-info.json | jq .
exit 3