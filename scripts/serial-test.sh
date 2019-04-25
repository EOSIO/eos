#!/bin/bash
set -e # exit on failure of any "simple" command (excludes &&, ||, or | chains)
# prepare environment
PATH=$PATH:~/opt/mongodb/bin
echo "Extracting build directory..."
[[ -z "${1}" ]] && tar -zxf build.tar.gz || tar -xzf $1
echo "Starting MongoDB..."
mongod --fork --dbpath ~/data/mongodb -f ~/etc/mongod.conf --logpath "$(pwd)"/mongod.log
cd /data/job/build
# run tests
echo "Running tests..."
TEST_COUNT=$(ctest -N -L nonparallelizable_tests | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
[[ $TEST_COUNT > 0 ]] && echo "$TEST_COUNT tests found." || (echo "ERROR: No tests registered with ctest! Exiting..." && exit 1)
set +e # defer ctest error handling to end
echo "$ ctest -L nonparallelizable_tests --output-on-failure -T Test"
ctest -L nonparallelizable_tests --output-on-failure -T Test
EXIT_STATUS=$?
[[ "$EXIT_STATUS" == 0 ]] && set -e
echo "Done running non-parallelizable tests."
# upload artifacts
echo "Uploading artifacts..."
XML_FILENAME="test-results.xml"
mv $(pwd)/Testing/$(ls $(pwd)/Testing/ | grep '20' | tail -n 1)/Test.xml $XML_FILENAME
buildkite-agent artifact upload config.ini
buildkite-agent artifact upload genesis.json
cd ..
buildkite-agent artifact upload mongod.log
cd build
buildkite-agent artifact upload $XML_FILENAME
echo "Done uploading artifacts."
# ctest error handling
if [[ "$EXIT_STATUS" != 0 ]]; then
    echo "Failing due to non-zero exit status from ctest: $EXIT_STATUS"
    echo '    ^^^ scroll up for more information ^^^'
    exit $EXIT_STATUS
fi
