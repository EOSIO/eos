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
echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running WASM Spec Tests"
TEST_COUNT=$(ctest -N -L wasm_spec_tests | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
if [[ $TEST_COUNT > 0 ]]; then
    echo "$TEST_COUNT tests found."
else
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests registered with ctest! Exiting..."
    exit 1
fi
# run tests
set +e # defer ctest error handling to end
echo "$ ctest -j $JOBS -L wasm_spec_tests --output-on-failure -T Test"
ctest -j $JOBS -L wasm_spec_tests --output-on-failure -T Test
EXIT_STATUS=$?
echo 'Done running WASM spec tests.'
exit $EXIT_STATUS