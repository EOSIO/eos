#!/bin/bash
set -e
# test name and purpose
echo ''
echo '                        ##### Release Build Test #####'
echo ''
echo 'The purpose of this test is to ensure that nodeos was built without debugging'
echo 'symbols. Debugging symbols enable software engineers to inspect and control a'
echo 'running program with a debugging tool, but they significantly slow down'
echo 'performance-critical applications like nodeos. Anyone intending to build and'
echo 'install nodeos from source should perform a "release build," which excludes'
echo 'debugging symbols to generate faster and lighter binaries.'
echo ''
# environment
[[ -z "$EOSIO_ROOT" && $(git --version) ]] && export EOSIO_ROOT="$(git rev-parse --show-toplevel)"
[[ -z "$EOSIO_ROOT" ]] && export EOSIO_ROOT="$(echo $(pwd)/ | grep -ioe '.*/eos/' -e '.*/eosio/' -e '.*/build/' | sed 's,/build/,/,')"
if [[ ! -f "$EOSIO_ROOT/build/bin/nodeos" && ! -f "$EOSIO_ROOT/build/programs/nodeos/nodeos" ]]; then
    echo 'ERROR: nodeos binary not found!'
    echo 'Looked in the following places:'
    echo "$ ls -la \"$EOSIO_ROOT/build/bin\""
    ls -la "$EOSIO_ROOT/build/bin"
    echo "$ ls -la \"$EOSIO_ROOT/build/programs/nodeos\""
    ls -la "$EOSIO_ROOT/build/programs/nodeos"
    echo 'Release build test not run.'
    exit 1
fi
[[ -f "$EOSIO_ROOT/build/bin/nodeos" ]] && cd "$EOSIO_ROOT/build/bin" || cd "$EOSIO_ROOT/build/programs/nodeos"
# setup
./nodeos --config-dir "$(pwd)/config" --data-dir "$(pwd)/data" & # run nodeos in background
sleep 10
kill $! # kill nodeos gracefully, by PID
if [[ ! -f data/state/shared_memory.bin ]]; then
    echo 'ERROR: nodeos state not found!'
    echo 'Looked for shared_memory.bin in the following places:'
    echo "$ ls -la \"$(pwd)/data/state\""
    ls -la "$(pwd)/data/state"
    echo 'Release build test not run.'
    exit 2
fi
# test
export DEBUG_BYTE="$(xxd -seek 9 -l 1 data/state/shared_memory.bin | awk '{print $2}')"
if [[ "$DEBUG_BYTE" == '00' ]]; then
    echo 'PASS: Debug byte not set.'
    exit 0
fi
echo 'FAIL: Debug byte is set!'
echo "Debug Byte = 0x$DEBUG_BYTE"
echo 'First kilobyte of shared_memory.bin:'
echo '$ xxd -l 1024 shared_memory.bin'
xxd -l 1024 data/state/shared_memory.bin
exit 3