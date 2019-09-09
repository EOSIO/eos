#!/bin/bash
set -e
# test name
echo ''
echo '                         ##### Release Build Test #####'
echo ''
# environment
[[ -z "$EOSIO_ROOT" && $(git --version) ]] && export EOSIO_ROOT="$(git rev-parse --show-toplevel)"
[[ -z "$EOSIO_ROOT" ]] && export EOSIO_ROOT="$(echo $(pwd)/ | grep -ioe '.*/eos/' -e '.*/eosio/' -e '.*/build/' | sed 's,/build/,/,')"
[[ -f "$EOSIO_ROOT/build/bin/nodeos" ]] && cd "$EOSIO_ROOT/build/bin" || cd "$EOSIO_ROOT/build/programs/nodeos"
# test
./nodeos --config-dir $(pwd)/config --data-dir $(pwd)/data & # run nodeos in background
sleep 10
kill $! # kill nodeos gracefully, by PID
if [[ "$(xxd -seek 9 -l 1 data/state/shared_memory.bin | awk '{print $2}')" == '00' ]]; then
    echo 'pass'
    exit 0
fi
echo 'FAIL'
exit 1