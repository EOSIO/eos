#!/usr/bin/env bash
set -ieo pipefail
# Load eosio specific helper functions
. ./scripts/helpers/eosio.bash
echo "+++ Extracting build directory"
[[ -f build.tar.gz ]] && tar -xzf build.tar.gz
execute ls -l build
PATH=$PATH:$MONGODB_LINK_DIR/bin # nodeos_run_test-mongodb: no such file in directory 'mongo'
[[ $ARCH == "Darwin" ]] && execute cd /data/job/eos/build || execute cd /data/job/build # I don't know why, but if we use cd ./build here it won't work...
if [[ -f $MONGODB_BIN ]]; then
    echo "+++ Killing old MongoDB"
    $(pgrep mongod | xargs kill -9) || true
    echo "+++ Starting MongoDB"
    execute $MONGODB_BIN --fork --dbpath $MONGODB_DATA_DIR -f $MONGODB_CONF --logpath ./mongod.log
fi
echo "+++ Running tests"
# Counting tests available and if they get disabled for some reason, throw a failure
TEST_COUNT=$($CTEST_BIN -N -LE _tests | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
[[ $TEST_COUNT > 0 ]] && echo "$TEST_COUNT tests found." || (echo "ERROR: No tests registered with ctest! Exiting..." && exit 1)
set +e # defer ctest error handling to end
# We cannot use interactive mode for ctest nonparallelizable tests
execute $CTEST_BIN -L nonparallelizable_tests --output-on-failure -T Test
EXIT_STATUS=$?
[[ $EXIT_STATUS == 0 ]] && set -e
echo "+++ Uploading artifacts"
mv -f ./Testing/$(ls ./Testing/ | grep '20' | tail -n 1)/Test.xml test-results.xml
execute buildkite-agent artifact upload test-results.xml
[[ -f $MONGODB_BIN ]] && execute buildkite-agent artifact upload mongod.log
# ctest error handling
if [[ $EXIT_STATUS != 0 ]]; then
    echo "Failing due to non-zero exit status from ctest: $EXIT_STATUS"
    execute buildkite-agent artifact upload config.ini
    execute buildkite-agent artifact upload genesis.json
    exit $EXIT_STATUS
fi