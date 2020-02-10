#!/bin/bash
set -eo pipefail
# variables
echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':evergreen_tree: ')Configuring Environment"
[[ -z "$JOBS" ]] && export JOBS=$(getconf _NPROCESSORS_ONLN)
GIT_ROOT="$(dirname $BASH_SOURCE[0])/.."
if [[ "$(uname)" == 'Linux' ]]; then
    . /etc/os-release
    if [[ "$ID" == 'centos' ]]; then
        [[ -f /opt/rh/rh-python36/enable ]] && source /opt/rh/rh-python36/enable
    fi
fi
cd $GIT_ROOT/build
# count tests
echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running Launcher Service Tests"
TEST_COUNT=$(ctest -N -L ls_tests | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
if [[ $TEST_COUNT > 0 ]]; then
    echo "$TEST_COUNT tests found."
else
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests registered with ctest! Exiting..."
    exit 1
fi
# prep tests
echo '{ "initial_timestamp": "2018-06-01T12:00:00.000", "initial_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "initial_configuration": { "max_block_net_usage": 1048576, "target_block_net_usage_pct": 1000, "max_transaction_net_usage": 524288, "base_per_transaction_net_usage": 12, "net_usage_leeway": 500, "context_free_discount_net_usage_num": 20, "context_free_discount_net_usage_den": 100, "max_block_cpu_usage": 200000, "target_block_cpu_usage_pct": 1000, "max_transaction_cpu_usage": 150000, "min_transaction_cpu_usage": 100, "max_transaction_lifetime": 3600, "deferred_trx_expiration_window": 600, "max_transaction_delay": 3888000, "max_inline_action_size": 4096, "max_inline_action_depth": 4, "max_authority_depth": 6 } }' > genesis.json
echo "starting launcher service: ./programs/launcher-service/launcher-service --http-server-address=0.0.0.0:1234 --http-threads=4 --genesis-json=./genesis.json > launcher_service.log 2>&1 &"
./programs/launcher-service/launcher-service --http-server-address=0.0.0.0:1234 --http-threads=4 --genesis-json=./genesis.json > launcher_service.log 2>&1 &
sleep 1
echo "platform ID is $ID"
echo "IMAGE_TAG is $IMAGE_TAG"
echo "python3 version is $(python3 --version)"
export PYTHONIOENCODING=UTF-8
echo "PYTHONIOENCODING is $PYTHONIOENCODING"
# run tests
set +e # defer ctest error handling to end
echo "$ ctest -j $JOBS -L ls_tests --output-on-failure -T Test"
ctest -j $JOBS -L ls_tests --output-on-failure -T Test
EXIT_STATUS=$?
echo "killing launcher-service"
pkill -9 launcher-service
echo 'Done running launcher service related tests.'
exit $EXIT_STATUS