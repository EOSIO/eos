#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing Options" {
    mkdir -p $HOME/test/tmp
    run bash -c "./$SCRIPT_LOCATION -y -P -i $HOME/test -b /boost_tmp -m"
    debug
    [[ ! -z $(echo "${output}" | grep "CMAKE_INSTALL_PREFIX='${HOME}/test/eosio/${EOSIO_VERSION}") ]] || exit
    [[ ! -z $(echo "${output}" | grep "@ /boost_tmp") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Checking MongoDB installation") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Installing MongoDB C++ driver...") ]] || exit
    [[ ! -z $(echo "${output}" | grep "CMAKE_TOOLCHAIN_FILE=$BUILD_DIR/pinned_toolchain.cmake") ]] || exit # Required -P
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
}