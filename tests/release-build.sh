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
# check for xxd
if ! $(xxd --version 2>/dev/null); then
    echo 'ERROR: Test requires xxd, but xxd was not found in your PATH!'
    echo ''
    echo 'The xxd hex dump tool can be installed as part of the vim-common package on most operating systems.'
    exit 1
fi
# find nodeos
[[ $(git --version) ]] && cd "$(git rev-parse --show-toplevel)/build" || cd "$(dirname "${BASH_SOURCE[0]}")/.."
if [[ ! -f programs/nodeos/nodeos ]]; then
    echo 'ERROR: nodeos binary not found!'
    echo ''
    echo 'I looked here...'
    echo "$ ls -la \"$(pwd)/programs/nodeos\""
    ls -la "$(pwd)/programs/nodeos"
    echo '...which I derived from one of these paths:'
    echo '$ echo "$(git rev-parse --show-toplevel)/build"'
    echo "$(git rev-parse --show-toplevel)/build"
    echo '$ echo "$(dirname "${BASH_SOURCE[0]}")/.."'
    echo "$(dirname "${BASH_SOURCE[0]}")/.."
    echo 'Release build test not run.'
    exit 2
fi
# run nodeos to generate state files
mkdir release-build-test
programs/nodeos/nodeos --config-dir "$(pwd)/release-build-test/config" --data-dir "$(pwd)/release-build-test/data" 1>/dev/null 2>/dev/null &
sleep 10
kill $! # kill nodeos gracefully, by PID
if [[ ! -f release-build-test/data/state/shared_memory.bin ]]; then
    echo 'ERROR: nodeos state not found!'
    echo ''
    echo 'Looked for shared_memory.bin in the following places:'
    echo "$ ls -la \"$(pwd)/release-build-test/data/state\""
    ls -la "$(pwd)/release-build-test/data/state"
    echo 'Release build test not run.'
    rm -rf release-build-test
    exit 3
fi
# test state files for debug flag
export DEBUG_BYTE="$(xxd -seek 9 -l 1 release-build-test/data/state/shared_memory.bin | awk '{print $2}')"
if [[ "$DEBUG_BYTE" == '00' ]]; then
    echo 'PASS: Debug flag is not set.'
    echo ''
    rm -rf release-build-test
    exit 0
fi
echo 'FAIL: Debug flag is set!'
echo "Debug Byte = 0x$DEBUG_BYTE"
echo ''
echo 'First kilobyte of shared_memory.bin:'
echo '$ xxd -l 1024 shared_memory.bin'
xxd -l 1024 release-build-test/data/state/shared_memory.bin
rm -rf release-build-test
exit 4