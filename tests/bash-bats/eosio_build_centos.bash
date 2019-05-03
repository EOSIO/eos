#!/usr/bin/env bats
load helpers/general

export SCRIPT_LOCATION="scripts/eosio_build.bash"
export TEST_LABEL="[eosio_build_centos]"

[[ $ARCH == "Linux" ]] || exit 0 # Exit 0 is required for pipeline
[[ $NAME == "CentOS Linux" ]] || exit 0 # Exit 0 is required for pipeline

install-package which 1>/dev/null || true
install-package gcc-c++ 1>/dev/null || true

# A helper function is available to show output and status: `debug`

# Testing Root user
./tests/bash-bats/modules/root-user.bash
# Testing Options
./tests/bash-bats/modules/options.bash
# Testing CMAKE
./tests/bash-bats/modules/cmake.bash
# Testing Clang
./tests/bash-bats/modules/clang.bash
# Testing Prompts
./tests/bash-bats/modules/prompts.bash

## Needed to load eosio_build_ files properly; it can be empty
@test "${TEST_LABEL} > General" { setup-bats-dirs # setup is required for anything outside of 
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    ### Make sure deps are loaded properly
    [[ ! -z $(echo "${output}" | grep "Starting EOSIO Dependency Install") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Executing: /usr/bin/yum -y update") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Centos Software Collections Repository installed successfully") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Centos devtoolset installed successfully") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Centos devtoolset-7 successfully enabled") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Python36 successfully enabled") ]] || exit
    [[ ! -z $(echo "${output}" | grep "python.*found!") ]] || exit
    [[ ! -z $(echo "${output}" | grep "sudo.*NOT.*found.") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Installing CMAKE") ]] || exit
    [[ ! -z $(echo "${output}" | grep ${HOME}.*/src/boost) ]] || exit
    [[ ! -z $(echo "${output}" | grep "Starting EOSIO Build") ]] || exit
    [[ ! -z $(echo "${output}" | grep "make -j${CPU_CORES}") ]] || exit
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
}

