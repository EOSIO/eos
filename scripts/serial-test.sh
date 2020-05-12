#!/bin/bash
set -eo pipefail
# variables
echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':evergreen_tree: ')Configuring Environment"
GIT_ROOT="$(dirname $BASH_SOURCE[0])/.."
[[ -z "$TEST" ]] && export TEST=$1
if [[ "$(uname)" == 'Linux' ]]; then
    . /etc/os-release
    if [[ "$ID" == 'centos' ]]; then
        [[ -f /opt/rh/rh-python36/enable ]] && source /opt/rh/rh-python36/enable
    fi
    cd $GIT_ROOT && npm install
else
    npm install
fi
cd $GIT_ROOT/build
# tests
if [[ -z "$TEST" ]]; then # run all serial tests
    # count tests
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running Non-Parallelizable Tests"
    TEST_COUNT=$(ctest -N -L nonparallelizable_tests | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
    if [[ $TEST_COUNT > 0 ]]; then
        echo "$TEST_COUNT tests found."
        # run tests
        set +e # defer ctest error handling to end
        echo '$ ctest -L nonparallelizable_tests --output-on-failure -T Test'
        ctest -L nonparallelizable_tests --output-on-failure -T Test
        EXIT_STATUS=$?
        echo 'Done running non-parallelizable tests.'
    else
        echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests registered with ctest! Exiting..."
        EXIT_STATUS='1'
    fi
else # run specific serial test
    # ensure test exists
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running $TEST"
    TEST_COUNT=$(ctest -N -R ^$TEST$ | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
    if [[ $TEST_COUNT > 0 ]]; then
        echo "$TEST found."
        # run tests
        set +e # defer ctest error handling to end
        echo "$ ctest -R ^$TEST$ --output-on-failure -T Test"
        ctest -R ^$TEST$ --output-on-failure -T Test
        EXIT_STATUS=$?
        echo "Done running $TEST."
    else
        echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests matching \"$TEST\" registered with ctest! Exiting..."
        EXIT_STATUS='1'
    fi
fi
exit $EXIT_STATUS