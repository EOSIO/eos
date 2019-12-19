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
# mongoDB
if [[ ! -z "$(pgrep mongod)" ]]; then
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':leaves: ')Killing old MongoDB"
    $(pgrep mongod | xargs kill -9) || :
fi
# do not start mongod if it does not exist
if [[ -x $(command -v mongod) ]]; then
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':leaves: ')Starting new MongoDB"
    [[ ! -d ~/data/mongodb && ! -d mongodata ]] && mkdir mongodata
    echo "$ mongod --fork --logpath $(pwd)/mongod.log $([[ -d ~/data/mongodb ]] && echo '--dbpath ~/data/mongodb' || echo "--dbpath $(pwd)/mongodata") $(if [[ -f ~/etc/mongod.conf ]]; then echo '-f ~/etc/mongod.conf'; elif [[ -f /usr/local/etc/mongod.conf ]]; then echo '-f /usr/local/etc/mongod.conf'; fi)"
    eval mongod --fork --logpath $(pwd)/mongod.log $([[ -d ~/data/mongodb ]] && echo '--dbpath ~/data/mongodb' || echo "--dbpath $(pwd)/mongodata") $(if [[ -f ~/etc/mongod.conf ]]; then echo '-f ~/etc/mongod.conf'; elif [[ -f /usr/local/etc/mongod.conf ]]; then echo '-f /usr/local/etc/mongod.conf'; fi)
else
    echo "Could not find mongod binary... Ensure it's in the PATH! Continuing without starting it."
fi
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
if [[ ! -z "$(pgrep mongod)" ]]; then
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':leaves: ')Killing MongoDB"
    $(pgrep mongod | xargs kill -9) || :
fi
exit $EXIT_STATUS