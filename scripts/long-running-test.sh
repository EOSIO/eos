#!/bin/bash
set -eo pipefail

. "${0%/*}/helpers/general.sh"

# variables
echo "--- $([[ "$BUILDKITE" == 'true' ]] && echo ':evergreen_tree: ')Configuring Environment"
GIT_ROOT="$(dirname $BASH_SOURCE[0])/.."
RABBITMQ_SERVER_DETACHED='rabbitmq-server -detached'
[[ -z "$TEST" ]] && export TEST="$1"
if [[ "$(uname)" == 'Linux' ]]; then
    . /etc/os-release
    if [[ "$ID" == 'centos' ]]; then
        [[ -f /opt/rh/rh-python36/enable ]] && source /opt/rh/rh-python36/enable
    fi
fi
cd "$GIT_ROOT/build"
# tests
if [[ -z "$TEST" ]]; then # run all serial tests
    perform "$RABBITMQ_SERVER_DETACHED"
    perform "sleep 30"
    # count tests
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running Long-Running Tests"
    TEST_COUNT=$(ctest -N -L 'long_running_tests' | grep -i 'Total Tests: ' | cut -d ':' -f '2' | awk '{print $1}')
    if [[ "$TEST_COUNT" > '0' ]]; then
        printdt "$TEST_COUNT tests found."
        # run tests
        set +e # defer ctest error handling to end
        perform "ctest -L 'long_running_tests' --output-on-failure -T 'Test'"
        EXIT_STATUS=$?
        printdt 'Done running long-running tests.'
    else
        echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests registered with ctest! Exiting..."
        EXIT_STATUS='1'
    fi
else # run specific serial test
    if [[ "$(echo "$TEST" | grep -ci 'rabbit')" != '0' ]]; then
        perform "$RABBITMQ_SERVER_DETACHED"
        perform "sleep 30"
    fi
    # ensure test exists
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running $TEST"
    TEST_COUNT=$(ctest -N -R "^$TEST$" | grep -i 'Total Tests: ' | cut -d ':' -f '2' | awk '{print $1}')
    if [[ "$TEST_COUNT" > '0' ]]; then
        printdt "$TEST found."
        # run tests
        set +e # defer ctest error handling to end
        perform "ctest -R '^$TEST$' --output-on-failure -T 'Test'"
        EXIT_STATUS=$?
        printdt "Done running $TEST."
    else
        echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests matching \"$TEST\" registered with ctest! Exiting..."
        EXIT_STATUS='1'
    fi
fi
exit $EXIT_STATUS
