#!/bin/bash
set -e # exit on failure of any "simple" command (excludes &&, ||, or | chains)
CPU_CORES=$(getconf _NPROCESSORS_ONLN)
echo "$CPU_CORES cpu cores detected."
cd /eosio.contracts/build/tests
TEST_COUNT=$(ctest -N | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
[[ $TEST_COUNT > 0 ]] && echo "$TEST_COUNT tests found." || (echo "ERROR: No tests registered with ctest! Exiting..." && exit 1)
echo "$ ctest -j $CPU_CORES --output-on-failure -T Test"
set +e # defer ctest error handling to end
ctest -j $CPU_CORES --output-on-failure -T Test
EXIT_STATUS=$?
[[ "$EXIT_STATUS" == 0 ]] && set -e
mv /eosio.contracts/build/tests/Testing/$(ls /eosio.contracts/build/tests/Testing/ | grep '20' | tail -n 1)/Test.xml /artifacts/Test.xml
# ctest error handling
if [[ "$EXIT_STATUS" != 0 ]]; then
    echo "Failing due to non-zero exit status from ctest: $EXIT_STATUS"
    echo '    ^^^ scroll up for more information ^^^'
    exit $EXIT_STATUS
fi