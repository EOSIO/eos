#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing CLANG" {

    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"

    if [[ $NAME == "Darwin" ]]; then
        ## CLANG already exists (c++/default)
        [[ ! -z $(echo "${output}" | grep "DCMAKE_CXX_COMPILER='c++'") ]] || exit
        [[ ! -z $(echo "${output}" | grep "DCMAKE_C_COMPILER='cc'") ]] || exit
    elif [[ $NAME == "Ubuntu" ]]; then
        ## CLANG already exists (c++/default) (Ubuntu doesn't have clang>7, so we need to make sure it installs Clang 8)
        [[ ! -z $(echo "${output}" | grep "Unable to find C++17 support") ]] || exit
        [[ ! -z $(echo "${output}" | grep "Installing Clang 8") ]] || exit
        [[ ! -z $(echo "${output}" | grep "$CLANG_ROOT") ]] || exit
        ## CLANG
        uninstall-package clang 1>/dev/null
        run bash -c "./$SCRIPT_LOCATION -y -P"
        [[ ! -z $(echo "${output}" | grep "Checking Clang support") ]] || exit
        [[ ! -z $(echo "${output}" | grep -E "Clang.*successfully installed @ ${CLANG_ROOT}") ]] || exit
    fi
    ## CXX doesn't exist
    export CXX=c2234
    export CC=ewwqd
    run bash -c "./$SCRIPT_LOCATION -y"
    [[ ! -z $(echo "${output}" | grep "Unable to find compiler \"c2234\"") ]] || exit
    
}