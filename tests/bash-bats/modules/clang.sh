#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing CLANG" {

    if [[ $NAME == "Darwin" ]]; then
        run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -i /NEWPATH"
        ## CLANG already exists (c++/default)
        [[ ! -z $(echo "${output}" | grep "PIN_COMPILER: true") ]] || exit
        [[ ! -z $(echo "${output}" | grep "DCMAKE_CXX_COMPILER='c++'") ]] || exit
        [[ ! -z $(echo "${output}" | grep "DCMAKE_C_COMPILER='cc'") ]] || exit
    elif [[ $NAME == "Ubuntu" ]]; then
        install-package build-essential WETRUN 1>/dev/null # ubuntu 18 build-essential will be high enough, 16 won't and has a version < 7
        run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -i /NEWPATH"
        ## CLANG already exists (c++/default) (Ubuntu doesn't have clang>7, so we need to make sure it installs Clang 8)
        [[ ! -z $(echo "${output}" | grep "PIN_COMPILER: false") ]] || exit
        # if [[ $VERSION_ID == "16.04" ]]; then
        #     [[ ! -z $(echo "${output}" | grep "Unable to find compiler") ]] || exit
        #     [[ ! -z $(echo "${output}" | grep "Clang 8 successfully installed") ]] || exit
        #     [[ ! -z $(echo "${output}" | grep "$CLANG_ROOT") ]] || exit
        # fi
        ## CLANG
        apt autoremove build-essential -y 1>/dev/null
        run bash -c "./$SCRIPT_LOCATION -y -P"
        [[ ! -z $(echo "${output}" | grep "PIN_COMPILER: true") ]] || exit
        [[ ! -z $(echo "${output}" | grep "Clang 8 successfully installed") ]] || exit
        [[ ! -z $(echo "${output}" | grep -E "Clang.*successfully installed @ ${CLANG_ROOT}") ]] || exit
    fi
    ## CXX doesn't exist
    export CXX=c2234
    export CC=ewwqd
    run bash -c "./$SCRIPT_LOCATION -y"
    [[ ! -z $(echo "${output}" | grep "Unable to find .* compiler") ]] || exit
    
}