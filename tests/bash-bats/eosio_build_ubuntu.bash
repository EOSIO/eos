#!/usr/bin/env bats
load test_helper

SCRIPT_LOCATION="scripts/eosio_build.bash"
TEST_LABEL="[eosio_build_ubuntu]"

[[ $ARCH == "Linux" ]] || exit 0 # Exit 0 is required for pipeline
[[ $NAME == "Ubuntu" ]] || exit 0 # Exit 0 is required for pipeline
( [[ $VERSION_ID == "18.04" ]] || [[ $VERSION_ID == "16.04" ]] ) || exit 0 # Exit 0 is required for pipeline

# A helper function is available to show output and status: `debug`

@test "${TEST_LABEL} > Testing Options" {
    run bash -c "./$SCRIPT_LOCATION -y -i /tmp -b /boost_tmp"
    [[ ! -z $(echo "${output}" | grep "CMAKE_INSTALL_PREFIX=/tmp/eosio") ]] || exit
    [[ ! -z $(echo "${output}" | grep "@ /boost_tmp") ]] || exit
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
}

@test "${TEST_LABEL} > Testing Prompts" {
    ## All yes pass
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
    ## First no shows "aborting"  
    run bash -c "printf \"n\n%.0s\" {1..2} | ./$SCRIPT_LOCATION"
    [[ "${output##*$'\n'}" =~ "- User aborted installation of required dependencies." ]] || exit
}

@test "${TEST_LABEL} > Testing CMAKE Install" {
    export CMAKE="$HOME/cmake" # file just needs to exist
    touch $CMAKE
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    [[ ! -z $(echo "${output}" | grep "CMAKE found @ ${CMAKE}") ]] || exit
    rm -f $CMAKE
    export CMAKE=
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    [[ ! -z $(echo "${output}" | grep "Installing CMAKE") ]] || exit
}

@test "${TEST_LABEL} > Testing Executions" {
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    [[ ! -z $(echo "${output}" | grep "Starting EOSIO Dependency Install") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Executing: /usr/bin/apt-get update") ]] || exit
    [[ ! -z $(echo "${output}" | grep "apt.*found!") ]] || exit
    [[ ! -z $(echo "${output}" | grep "make.*NOT.*found.") ]] || exit
    [[ ! -z $(echo "${output}" | grep "mongodb-linux-x86_64-ubuntu.*04-") ]] || exit
    [[ ! -z $(echo "${output}" | grep ${HOME}.*/src/boost) ]] || exit
    [[ ! -z $(echo "${output}" | grep "Starting EOSIO Build") ]] || exit
    [[ ! -z $(echo "${output}" | grep "make -j${CPU_CORES}") ]] || exit
}

@test "${TEST_LABEL} > Testing root user run" {
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    [[ ! -z $(echo "${output}" | grep "User: root") ]] || exit
    export CURRENT_USER=test
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    [[ ! -z $(echo "${output}" | grep "User: test") ]] || exit
}