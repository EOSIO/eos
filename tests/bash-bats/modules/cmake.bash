#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing CMAKE" {
    # Testing for if CMAKE already exists
    export CMAKE=${HOME}/cmake
    touch $CMAKE
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${HOME}/cmake") ]] || exit
    # Testing for if cmake doesn't exist to be sure it's set properly
    export CMAKE=
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    if [[ $ARCH == "Darwin" ]]; then
        [[ ! -z $(echo "${output}" | grep "Executing: bash -c /usr/local/bin/cmake -DCMAKE_BUILD") ]] || exit
    else
        [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${BIN_LOCATION}/cmake") ]] || exit
    fi
}