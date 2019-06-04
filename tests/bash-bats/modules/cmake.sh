#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing CMAKE" {
    # Testing for if CMAKE already exists
    export CMAKE=${HOME}/cmake
    touch $CMAKE
    run bash -c " ./$SCRIPT_LOCATION -y -P"
    [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${HOME}/cmake") ]] || exit
    # Testing for if cmake doesn't exist to be sure it's set properly
    export CMAKE=
    run bash -c "./$SCRIPT_LOCATION -y -P"
    if [[ $ARCH == "Darwin" ]]; then
        [[ ! -z $(echo "${output}" | grep "Executing: bash -c /usr/local/bin/cmake -DCMAKE_BUILD") ]] || exit
    else
        [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${BIN_DIR}/cmake") ]] || exit
        [[ ! -z $(echo "${output}" | grep "CMAKE successfully installed") ]] || exit
    fi
}