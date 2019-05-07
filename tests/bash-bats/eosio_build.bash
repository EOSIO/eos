#!/usr/bin/env bats
load helpers/general

SCRIPT_LOCATION="scripts/eosio_build.bash"
TEST_LABEL="[eosio_build]"

# A helper function is available to show output and status: `debug`
@test "${TEST_LABEL} > Testing arguments/options" {

    # Testing if no cpp or clang, since we need it to build cmake
    if [[ $ARCH == "Linux" ]]; then
        ( [[ $NAME == "CentOS Linux" ]] || [[ $NAME =~ "Amazon" ]] ) && uninstall-package gcc-c++ &>/dev/null # If c++ exists, it will fail
        [[ $NAME =~ "Ubuntu" ]] && uninstall-package clang &>/dev/null
        run bash -c "./$SCRIPT_LOCATION"
        [[ ! -z $(echo "${output}" | grep "Unable to find compiler \"c++\"! Pass in the -P option if you wish for us to install it") ]] || exit
    fi

    # -P with -y
    run bash -c "./$SCRIPT_LOCATION -y -P"
    [[ ! -z $(echo "${output}" | grep "PIN_COMPILER: true") ]] || exit
    [[ ! -z $(echo "${output}" | grep "BUILD_CLANG: true") ]] || exit
    [[ "${output}" =~ -DCMAKE_TOOLCHAIN_FILE=\'.*/scripts/../build/pinned_toolchain.cmake\' ]] || exit
    # -P with prompts
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ "${output}" =~ .*User.aborted.* ]] || exit
    # lack of -m
    [[ ! -z $(echo "${output}" | grep "ENABLE_MONGO: false") ]] || exit
    [[ ! -z $(echo "${output}" | grep "INSTALL_MONGO: false") ]] || exit
    # lack of -i
    [[ ! -z $(echo "${output}" | grep "INSTALL_LOCATION: ${HOME}") ]] || exit
    [[ ! -z $(echo "${output}" | grep "EOSIO_HOME: ${HOME}/eosio/${EOSIO_VERSION}") ]] || exit
    ## -o
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -o Debug -P"
    [[ ! -z $(echo "${output}" | grep "CMAKE_BUILD_TYPE: Debug") ]] || exit
    ## -s
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -s EOS2 -P"
    [[ ! -z $(echo "${output}" | grep "CORE_SYMBOL_NAME: EOS2") ]] || exit
    ## -b
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -b /test -P"
    [[ ! -z $(echo "${output}" | grep "BOOST_LOCATION: /test") ]] || exit
    ## -i
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -i /NEWPATH -P"
    [[ ! -z $(echo "${output}" | grep "INSTALL_LOCATION: /NEWPATH") ]] || exit
    [[ ! -z $(echo "${output}" | grep "TEMP_DIR: /NEWPATH/tmp") ]] || exit
    ## -c
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -c -P"
    [[ ! -z $(echo "${output}" | grep "ENABLE_COVERAGE_TESTING: true") ]] || exit
    ## -d
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -d -P"
    [[ ! -z $(echo "${output}" | grep "ENABLE_DOXYGEN: true") ]] || exit
    ## -m
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -m -y -P"
    [[ ! -z $(echo "${output}" | grep "ENABLE_MONGO: true") ]] || exit
    [[ ! -z $(echo "${output}" | grep "INSTALL_MONGO: true") ]] || exit
    ## -h / -anythingwedon'tsupport
    run bash -c "./$SCRIPT_LOCATION -z"
    [[ ! -z $(echo "${output}" | grep "Invalid Option!") ]] || exit
    run bash -c "./$SCRIPT_LOCATION -h"
    [[ ! -z $(echo "${output}" | grep "Usage:") ]] || exit
}