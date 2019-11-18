#!/bin/bash
set -eo pipefail

[[ -z "$JOBS" ]] && export JOBS=$(getconf _NPROCESSORS_ONLN)
GIT_ROOT="$(dirname $BASH_SOURCE[0])/.."
if [[ "$(uname)" == 'Linux' ]]; then
    . /etc/os-release
    if [[ "$ID" == 'centos' ]]; then
        [[ -f /opt/rh/rh-python36/enable ]] && source /opt/rh/rh-python36/enable
    fi
fi
cd $GIT_ROOT/build

echo '{ "initial_timestamp": "2018-06-01T12:00:00.000", "initial_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "initial_configuration": { "max_block_net_usage": 1048576, "target_block_net_usage_pct": 1000, "max_transaction_net_usage": 524288, "base_per_transaction_net_usage": 12, "net_usage_leeway": 500, "context_free_discount_net_usage_num": 20, "context_free_discount_net_usage_den": 100, "max_block_cpu_usage": 200000, "target_block_cpu_usage_pct": 1000, "max_transaction_cpu_usage": 150000, "min_transaction_cpu_usage": 100, "max_transaction_lifetime": 3600, "deferred_trx_expiration_window": 600, "max_transaction_delay": 3888000, "max_inline_action_size": 4096, "max_inline_action_depth": 4, "max_authority_depth": 6 } }' > genesis.json

./programs/launcher-service/launcher-service --http-server-address=0.0.0.0:1234 --http-threads=4 --genesis-file=./genesis.json &

sleep 1

echo "platform ID is $ID"
if [[ "$ID" == 'centos' ]]; then
    echo "try to install python3.7 on centos"
    yum -y update && yum -y install python37 python37-pip
fi

if [[ "$ID" == 'ubuntu' ]]; then
    echo "try to install python3.7 on ubuntu"
    apt update && apt -y install python3 python3-pip python3.7
fi

python3.7 -m pip install requests
python3.7 -m pip install dataclasses

echo "ready to execute: ctest -L ls_tests -V -j $JOBS"
ctest -L ls_tests -V -j $JOBS

EXIT_STATUS=$?
echo 'Done running launcher service related tests.'

