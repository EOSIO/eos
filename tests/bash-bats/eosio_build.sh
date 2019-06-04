#!/usr/bin/env bats
load helpers/general

SCRIPT_LOCATION="scripts/eosio_build.sh"
TEST_LABEL="[eosio_build]"

###################################################################
# ON MAC YOU NEED TO FULLY UNINSTALL EOSIO BEFORE THESE WILL PASS #
###################################################################

# A helper function is available to show output and status: `debug`
@test "${TEST_LABEL} > Testing Arguments & Options" {

    if [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]]; then
        # which package isn't installed
        uninstall-package which WETRUN &>/dev/null
        run bash -c "printf \"n\nn\n\" | ./scripts/eosio_build.sh"
        [[ ! -z $(echo "${output}" | grep "EOSIO compiler checks require the 'which'") ]] || exit
        [[ ! -z $(echo "${output}" | grep "Please install the 'which'") ]] || exit
    fi

    if [[ $ARCH == "Linux" ]]; then
        if [[ $NAME == "CentOS Linux" ]]; then # Centos has the SCL prompt before checking for the compiler
            # No c++!
            run bash -c "printf \"y\ny\nn\n\" | ./${SCRIPT_LOCATION}"
        else
            # No c++!
            run bash -c "printf \"y\nn\nn\n\" | ./${SCRIPT_LOCATION}"
        fi
        [[ ! -z $(echo "${output}" | grep "Unable to find compiler") ]] || exit
    fi 

    # -P with -y
    run bash -c "./$SCRIPT_LOCATION -y -P"
    [[ ! -z $(echo "${output}" | grep "PIN_COMPILER: true") ]] || exit
    [[ "${output}" =~ -DCMAKE_TOOLCHAIN_FILE=\'.*/scripts/../build/pinned_toolchain.cmake\' ]] || exit
    [[ "${output}" =~ "Clang 8 successfully installed" ]] || exit
    # -P with prompts
    run bash -c "printf \"n\nn\nn\n\" | ./$SCRIPT_LOCATION -P"
    [[ "${output}" =~ .*User.aborted.* ]] || exit
    # lack of -m
    [[ ! -z $(echo "${output}" | grep "ENABLE_MONGO: false") ]] || exit
    [[ ! -z $(echo "${output}" | grep "INSTALL_MONGO: false") ]] || exit
    # lack of -i
    [[ ! -z $(echo "${output}" | grep "INSTALL_LOCATION: ${HOME}") ]] || exit
    [[ ! -z $(echo "${output}" | grep "EOSIO_INSTALL_DIR: ${HOME}/eosio/${EOSIO_VERSION}") ]] || exit
    ## -o
    run bash -c "printf \"y\nn\nn\n\" | ./$SCRIPT_LOCATION -o Debug -P"
    [[ ! -z $(echo "${output}" | grep "CMAKE_BUILD_TYPE: Debug") ]] || exit
    ## -s
    run bash -c "printf \"y\nn\nn\n\" | ./$SCRIPT_LOCATION -s EOS2 -P"
    [[ ! -z $(echo "${output}" | grep "CORE_SYMBOL_NAME: EOS2") ]] || exit
    ## -b
    run bash -c "printf \"y\nn\nn\n\" | ./$SCRIPT_LOCATION -b /test -P"
    [[ ! -z $(echo "${output}" | grep "BOOST_LOCATION: /test") ]] || exit
    ## -i
    run bash -c "printf \"y\nn\nn\n\"| ./$SCRIPT_LOCATION -i /NEWPATH -P"
    [[ ! -z $(echo "${output}" | grep "INSTALL_LOCATION: /NEWPATH") ]] || exit
    [[ ! -z $(echo "${output}" | grep "TEMP_DIR: /NEWPATH/tmp") ]] || exit
    ## -c
    run bash -c "printf \"y\nn\nn\n\"| ./$SCRIPT_LOCATION -c -P"
    [[ ! -z $(echo "${output}" | grep "ENABLE_COVERAGE_TESTING: true") ]] || exit
    ## -d
    run bash -c "printf \"y\nn\nn\n\" | ./$SCRIPT_LOCATION -d -P"
    [[ ! -z $(echo "${output}" | grep "ENABLE_DOXYGEN: true") ]] || exit
    ## -m
    run bash -c "printf \"y\nn\nn\n\" | ./$SCRIPT_LOCATION -m -y -P"
    [[ ! -z $(echo "${output}" | grep "ENABLE_MONGO: true") ]] || exit
    [[ ! -z $(echo "${output}" | grep "INSTALL_MONGO: true") ]] || exit
    ## -h / -anythingwedon'tsupport
    run bash -c "./$SCRIPT_LOCATION -z"
    [[ ! -z $(echo "${output}" | grep "Invalid Option!") ]] || exit
    run bash -c "./$SCRIPT_LOCATION -h"
    [[ ! -z $(echo "${output}" | grep "Usage:") ]] || exit
}