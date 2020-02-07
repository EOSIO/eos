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

echo "starting launcher service: ./programs/launcher-service/launcher-service --http-server-address=0.0.0.0:1234 --http-threads=4 --genesis-json=./genesis.json > launcher_service.log 2>&1 &"
./programs/launcher-service/launcher-service --http-server-address=0.0.0.0:1234 --http-threads=4 --genesis-json=./genesis.json > launcher_service.log 2>&1 &

sleep 1

echo "platform ID is $ID"
echo "IMAGE_TAG is $IMAGE_TAG"
echo "python3 version is $(python3 --version)"

if [[ "$IMAGE_TAG" == "ubuntu-16.04-pinned" ]]; then
    ln -sf $(which python3.6) $(which python3)
    echo "Using python3.6 as python3 for Ubuntu 16.04"
    echo "python3 version is now $(python3 --version)"
fi

set +e # defer ctest error handling to end

export PYTHONIOENCODING=UTF-8
echo "PYTHONIOENCODING is $PYTHONIOENCODING"
echo "ready to execute: ctest -L ls_tests --output-on-failure -j 32 -T Test"
ctest -L ls_tests --output-on-failure -j 32 -T Test

EXIT_STATUS=$?

echo "killing launcher-service"
pkill -9 launcher-service

echo 'Done running launcher service related tests.'
exit $EXIT_STATUS
