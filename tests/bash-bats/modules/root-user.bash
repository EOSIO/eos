#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing root user run" {
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "User: $(whoami)") ]] || exit
    if [[ $ARCH == "Linux" ]]; then
        [[ -z $(echo "${output}" | grep "/usr/bin/sudo -E") ]] || exit
    fi
    export CURRENT_USER=test
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "User: test") ]] || exit
    if [[ $ARCH == "Linux" ]]; then
        [[ ! -z $(echo "${output}" | grep "/usr/bin/sudo -E") ]] || exit
    fi
}