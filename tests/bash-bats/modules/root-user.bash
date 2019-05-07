#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing root user run" {
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "User: $(whoami)") ]] || exit
    [[ -z $(echo "${output}" | grep "/usr/bin/sudo -E") ]] || exit
    export CURRENT_USER=test
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "User: test") ]] || exit
    [[ ! -z $(echo "${output}" | grep "/usr/bin/sudo -E") ]] || exit
}