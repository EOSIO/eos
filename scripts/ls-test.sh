#!/bin/bash
set -eo pipefail

[[ -z "$JOBS" ]] && export JOBS=$(getconf _NPROCESSORS_ONLN)
GIT_ROOT="$(dirname $BASH_SOURCE[0])/.."
if [[ "$(uname)" == 'Linux' ]]; then
    . /etc/os-release
    if [[ "$ID" == 'centos' ]]; then
        [[ -f /opt/rh/rh-python36/enable ]] && source /opt/rh/rh-python36/enable
    fi
fi
cd $GIT_ROOT/build

# run tests
pip3 install requests
echo "ready to execute: ctest -L ls_tests -j $JOBS"
ctest -L ls_tests -j $JOBS

EXIT_STATUS=$?
echo 'Done running launcher service related tests.'

