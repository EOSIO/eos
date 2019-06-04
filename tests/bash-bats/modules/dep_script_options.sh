#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing Options" {
    mkdir -p $HOME/test/tmp
    run bash -c "./$SCRIPT_LOCATION -y -P -i $HOME/test -b /boost_tmp -m"
    [[ ! -z $(echo "${output}" | grep "CMAKE_INSTALL_PREFIX='${HOME}/test/eosio/${EOSIO_VERSION}") ]] || exit
    [[ ! -z $(echo "${output}" | grep "@ /boost_tmp") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Ensuring MongoDB installation") ]] || exit
    [[ ! -z $(echo "${output}" | grep "MongoDB C driver successfully installed") ]] || exit
    if [[ $ARCH == "Linux" ]]; then
        ( [[ ! -z $(echo "${output}" | grep "ENABLE_SNAPPY=OFF -DCMAKE_TOOLCHAIN_FILE='$BUILD_DIR/pinned_toolchain.cmake'") ]] ) || exit # MySQL install
    fi
    [[ ! -z $(echo "${output}" | grep "CMAKE_TOOLCHAIN_FILE='$BUILD_DIR/pinned_toolchain.cmake' -DCMAKE_PREFIX_PATH") ]] || exit # cmake build
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
}