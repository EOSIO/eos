#!/usr/bin/env bash
set -ieo pipefail
# Load eosio specific helper functions
. ./scripts/helpers/eosio.bash
echo "+++ Extracting build directory"
[[ -f build.tar.gz ]] && execute tar -xzf build.tar.gz
execute ls -l build
execute cd build
echo "+++ Running tests"
# Counting tests available and if they get disabled for some reason, throw a failure
TEST_COUNT=$($CTEST_BIN -N -LE _tests | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
[[ $TEST_COUNT > 0 ]] && echo "$TEST_COUNT tests found." || (echo "ERROR: No tests registered with ctest! Exiting..." && exit 1)
set +e # defer ctest error handling to end
CORES=$(getconf _NPROCESSORS_ONLN)
# interactive mode is necessary for our NP tests to function
execute $CTEST_BIN -j $CORES -LE _tests --output-on-failure -T Test
EXIT_STATUS=$?
[[ "$EXIT_STATUS" == 0 ]] && set -e
# Prepare tests for artifact upload
echo "+++ Uploading artifacts"
execute mv -f ./Testing/$(ls ./Testing/ | grep '20' | tail -n 1)/Test.xml test-results.xml
execute buildkite-agent artifact upload test-results.xml
# ctest error handling
if [[ $EXIT_STATUS != 0 ]]; then
    echo "Failing due to non-zero exit status from ctest: $EXIT_STATUS"
    execute buildkite-agent artifact upload config.ini
    execute buildkite-agent artifact upload genesis.json
    exit $EXIT_STATUS
fi