#!/bin/bash
set -eo pipefail
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
    cd "$GIT_ROOT"
fi
echo "$ npm install"
npm install
cd "$GIT_ROOT/build"
# tests
if [[ -z "$TEST" ]]; then # run all serial tests
    echo "$ $RABBITMQ_SERVER_DETACHED"
    eval $RABBITMQ_SERVER_DETACHED
    sleep 30
    # count tests
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running Non-Parallelizable Tests"
    TEST_COUNT=$(ctest -N -L 'nonparallelizable_tests' | grep -i 'Total Tests: ' | cut -d ':' -f '2' | awk '{print $1}')
    if [[ "$TEST_COUNT" > '0' ]]; then
        echo "$TEST_COUNT tests found."
        # run tests
        set +e # defer ctest error handling to end
        CTEST_COMMAND="ctest -L 'nonparallelizable_tests' --output-on-failure -T 'Test'"
        echo "$ $CTEST_COMMAND"
        eval $CTEST_COMMAND
        EXIT_STATUS=$?
        echo 'Done running non-parallelizable tests.'
    else
        echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests registered with ctest! Exiting..."
        EXIT_STATUS='1'
    fi
else # run specific serial test
    if [[ "$(echo "$TEST" | grep -ci 'rabbit')" != '0' ]]; then
        echo "$ $RABBITMQ_SERVER_DETACHED"
        eval $RABBITMQ_SERVER_DETACHED
        sleep 30
    fi
    # ensure test exists
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running $TEST"
    TEST_COUNT=$(ctest -N -R ^$TEST$ | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
    if [[ "$TEST_COUNT" > '0' ]]; then
        echo "$TEST found."
        # run tests
        set +e # defer ctest error handling to end
        CTEST_COMMAND="ctest -R '^$TEST$' -V -T 'Test' 2>&1 | tee 'ctest-output.log'"
        echo "$ $CTEST_COMMAND"
        eval $CTEST_COMMAND
        EXIT_STATUS=$?
        echo "Done running $TEST."
        [[ "$EXIT_STATUS" == '0' ]] && echo "$TEST PASSED" || echo "$TEST FAILED - see ctest-output.log in the artifacts tab"
        echo 'Late blocks:'
        LATE_BLOCKS="grep -n 'produced the following blocks late' 'ctest-output.log'"
        echo "$ $LATE_BLOCKS"
        eval $LATE_BLOCKS
    else
        echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests matching \"$TEST\" registered with ctest! Exiting..."
        EXIT_STATUS='1'
    fi
fi
exit $EXIT_STATUS
