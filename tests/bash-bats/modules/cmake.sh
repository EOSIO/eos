#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing CMAKE" {

    # Testing for if CMAKE already exists
    export CMAKE=${HOME}/cmake
    touch $CMAKE
    export CMAKE_CURRENT_VERSION=3.7.1
    run bash -c " ./$SCRIPT_LOCATION -y -P"
    if [[ $ARCH == "Darwin" ]]; then
        [[ "${output##*$'\n'}" =~ "($CMAKE_REQUIRED_VERSION). Cannot proceed" ]] || exit # Test the required version
    else
        [[ ! -z $(echo "${output}" | grep "We will be installing $CMAKE_VERSION") ]] || exit # Test the required version
        [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${BIN_DIR}/cmake") ]] || exit
    fi
    export CMAKE_CURRENT_VERSION=
    run bash -c " ./$SCRIPT_LOCATION -y -P"
    [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${HOME}/cmake") ]] || exit
    if [[ $ARCH != "Darwin" ]]; then
        [[ -z $(echo "${output}" | grep "CMAKE successfully installed") ]] || exit
    fi

    # Testing for if cmake doesn't exist to be sure it's set properly
    export CMAKE=
    export CMAKE_CURRENT_VERSION=3.6
    run bash -c "./$SCRIPT_LOCATION -y -P"
    if [[ $ARCH == "Darwin" ]]; then
        [[ "${output##*$'\n'}" =~ "($CMAKE_REQUIRED_VERSION). Cannot proceed" ]] || exit # Test the required version
    else
        [[ ! -z $(echo "${output}" | grep "We will be installing $CMAKE_VERSION") ]] || exit # Test the required version
        [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${BIN_DIR}/cmake") ]] || exit
        [[ ! -z $(echo "${output}" | grep "CMAKE successfully installed") ]] || exit
    fi
    export CMAKE_CURRENT_VERSION=
    run bash -c "./$SCRIPT_LOCATION -y -P"
    if [[ $ARCH == "Darwin" ]]; then
        [[ ! -z $(echo "${output}" | grep "Executing: bash -c /usr/local/bin/cmake -DCMAKE_BUILD") ]] || exit
    else
        [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${BIN_DIR}/cmake") ]] || exit
        [[ ! -z $(echo "${output}" | grep "CMAKE successfully installed") ]] || exit
    fi
}
