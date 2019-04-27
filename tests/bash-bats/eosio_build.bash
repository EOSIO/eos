#!/usr/bin/env bats
load test_helper

SCRIPT_LOCATION="scripts/eosio_build.bash"
TEST_LABEL="[eosio_build]"

# A helper function is available to show output and status: `debug`

@test "${TEST_LABEL} > Testing arguments/options" {
    # # FOR LOOP EACH PROMPT AND TEST THE SAME SET OF TESTS
    # ## -o
    # run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -o Debug"
    # [[ ! -z $(echo "${output}" | grep "CMAKE_BUILD_TYPE: Debug") ]] || exit
    # ## -i
    # run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -s EOS2"
    # [[ ! -z $(echo "${output}" | grep "CORE_SYMBOL_NAME: EOS2") ]] || exit
    # ## -b
    # run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -b /test"
    # [[ ! -z $(echo "${output}" | grep "BOOST_LOCATION: /test") ]] || exit
    # ## -i
    # run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    # [[ ! -z $(echo "${output}" | grep "INSTALL_LOCATION: ${HOME}") ]] || exit
    # [[ ! -z $(echo "${output}" | grep "EOSIO_HOME: ${HOME}/eosio/${EOSIO_VERSION}") ]] || exit
    # run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -i /NEWPATH"
    # [[ ! -z $(echo "${output}" | grep "INSTALL_LOCATION: /NEWPATH") ]] || exit
    # ## -y
    # run bash -c "./$SCRIPT_LOCATION -y"
    # [[ ! -z $(echo "${output}" | grep "NONINTERACTIVE: true") ]] || exit
    # ## -c
    # run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -c"
    # [[ ! -z $(echo "${output}" | grep "ENABLE_COVERAGE_TESTING: true") ]] || exit
    # ## -d
    # run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -d"
    # [[ ! -z $(echo "${output}" | grep "DOXYGEN: true") ]] || exit
    ## -m
    run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -m"
    debug
    [[ ! -z $(echo "${output}" | grep "DOXYGEN: true") ]] || exit
    # ## -P
    # run bash -c "printf \"n\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    # [[ ! -z $(echo "${output}" | grep "DOXYGEN: true") ]] || exit
    ## -h / -anythingwedon'tsupport
    # run bash -c "./$SCRIPT_LOCATION -z"
    # [[ ! -z $(echo "${output}" | grep "Invalid Option!") ]] || exit
    # run bash -c "./$SCRIPT_LOCATION -h"
    # [[ ! -z $(echo "${output}" | grep "Usage:") ]] || exit
}