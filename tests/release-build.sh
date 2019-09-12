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
[[ $(git --version) ]] && cd "$(git rev-parse --show-toplevel)/build/programs/nodeos" || cd "$(dirname "${BASH_SOURCE[0]}")/../programs/nodeos"
if [[ ! -f nodeos ]]; then
    echo 'ERROR: nodeos binary not found!'
    echo ''
    echo 'Looked in the following places:'
    echo '$ ls -la "$(git rev-parse --show-toplevel)/build/programs/nodeos"'
    ls -la "$(git rev-parse --show-toplevel)/build/programs/nodeos"
    echo '$ ls -la "$(dirname "${BASH_SOURCE[0]}")/../programs/nodeos"'
    ls -la "$(dirname "${BASH_SOURCE[0]}")/../programs/nodeos"
    echo 'Release build test not run.'
    exit 1
fi
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