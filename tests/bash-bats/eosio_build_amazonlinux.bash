#!/usr/bin/env bats
load test_helper

SCRIPT_LOCATION="scripts/eosio_build.bash"
TEST_LABEL="[eosio_build_amazonlinux]"

[[ $ARCH == "Linux" ]] || exit 0 # Skip if we're not on linux
( [[ $NAME == "Amazon Linux AMI" ]] || [[ $NAME == "Amazon Linux" ]] ) || exit 0 # Exit 0 is required for pipeline

# A helper function is available to show output and status: `debug`

@test "${TEST_LABEL} > Testing Options" {
    run bash -c "./$SCRIPT_LOCATION -y -P -i /newhome -b /boost_tmp"
    [[ ! -z $(echo "${output}" | grep "CMAKE_INSTALL_PREFIX='/newhome/eosio/${EOSIO_VERSION}") ]] || exit
    [[ ! -z $(echo "${output}" | grep "@ /boost_tmp") ]] || exit
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
    [[ -z $(echo "${output}" | grep "Checking MongoDB installation") ]] || exit
    ## -m
    run bash -c "./$SCRIPT_LOCATION -y -m -P"
    [[ ! -z $(echo "${output}" | grep "Checking MongoDB installation") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Installing MongoDB C++ driver...") ]] || exit
    [[ ! -z $(echo "${output}" | grep "CMAKE_TOOLCHAIN_FILE=$BUILD_DIR/pinned_toolchain.cmake") ]] || exit # Required -P
}

@test "${TEST_LABEL} > Testing Prompts" {
    ## All yes pass
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
    ## First no shows "aborting"  
    run bash -c "printf \"n\n%.0s\" {1..2} | ./$SCRIPT_LOCATION -P"
    [[ "${output##*$'\n'}" =~ "- User aborted installation of required dependencies." ]] || exit
}

@test "${TEST_LABEL} > Testing CMAKE Install" {
    export CMAKE="$HOME/cmake" # file just needs to exist
    touch $CMAKE
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "CMAKE found @ ${CMAKE}") ]] || exit
    rm -f $CMAKE
    export CMAKE=
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "Installing CMAKE") ]] || exit
}

@test "${TEST_LABEL} > Testing CLANG" {
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    ## CLANG already exists (c++/default)
    [[ ! -z $(echo "${output}" | grep "DCMAKE_CXX_COMPILER='c++'") ]] || exit
    [[ ! -z $(echo "${output}" | grep "DCMAKE_C_COMPILER='cc'") ]] || exit
    ## CLANG
    uninstall-package clang 1>/dev/null
    run bash -c "./$SCRIPT_LOCATION -y -P"
    [[ ! -z $(echo "${output}" | grep "Checking Clang support") ]] || exit
    [[ ! -z $(echo "${output}" | grep -E "Clang.*successfully installed @ ${CLANG_ROOT}") ]] || exit
    ## CXX doesn't exist
    export CXX=c2234
    export CC=ewwqd
    run bash -c "./$SCRIPT_LOCATION -y"
    [[ ! -z $(echo "${output}" | grep "Unable to find compiler c2234") ]] || exit
}

@test "${TEST_LABEL} > Testing Executions" {
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    ### Make sure deps are loaded properly
    [[ ! -z $(echo "${output}" | grep "Executing: cd ${SRC_LOCATION}") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Starting EOSIO Dependency Install") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Executing: /usr/bin/yum -y update") ]] || exit
    if $NAME == "Amazon Linux" ]]; then
        [[ ! -z $(echo "${output}" | grep "libstdc++.*found!") ]] || exit
    elif [[ $NAME == "Amazon Linux AMI" ]]; then
        [[ ! -z $(echo "${output}" | grep "make.*found!") ]] || exit
    fi
    [[ ! -z $(echo "${output}" | grep "sudo.*NOT.*found.") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Installing CMAKE") ]] || exit
    [[ ! -z $(echo "${output}" | grep ${HOME}.*/src/boost) ]] || exit
    [[ ! -z $(echo "${output}" | grep "Starting EOSIO Build") ]] || exit
    [[ ! -z $(echo "${output}" | grep "make -j${CPU_CORES}") ]] || exit
    [[ -z $(echo "${output}" | grep " Checking MongoDB installation") ]] || exit # Mongo is off
    # Ensure PIN_COMPILER=false uses proper flags for the various installs
    run bash -c "./$SCRIPT_LOCATION -y"
    [[ ! -z $(echo "${output}" | grep " -G \"Unix Makefiles\"") ]] || exit # CMAKE
    [[ ! -z $(echo "${output}" | grep " --with-iostreams --with-date_time") ]] || exit # BOOST
}

@test "${TEST_LABEL} > Testing root user run" {
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "User: root") ]] || exit
    export CURRENT_USER=test
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "User: test") ]] || exit
}