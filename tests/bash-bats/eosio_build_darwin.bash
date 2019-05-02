#!/usr/bin/env bats
load helpers/general

export SCRIPT_LOCATION="scripts/eosio_build.bash"
export TEST_LABEL="[eosio_build_darwin]"

[[ $ARCH == "Darwin" ]] || exit 0 # Exit 0 is required for pipeline
[[ $NAME == "Mac OS X" ]] || exit 0 # Exit 0 is required for pipeline

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
@test "${TEST_LABEL} > General" { setup-bats-dirs
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    ### Make sure deps are loaded properly
    [[ ! -z $(echo "${output}" | grep "Starting EOSIO Dependency Install") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Executing: /usr/bin/xcode-select --install") ]] || exit
    [[ ! -z $(echo "${output}" | grep "automake.*NOT.*found") ]] || exit
    rm -f $CMAKE
    [[ ! -z $(echo "${output}" | grep "[Updating HomeBrew]") ]] || exit
    [[ ! -z $(echo "${output}" | grep "brew tap eosio/eosio") ]] || exit
    [[ ! -z $(echo "${output}" | grep "brew install.*llvm@4.*") ]] || exit
    [[ ! -z $(echo "${output}" | grep "LLVM successfully linked from /usr/local/opt/llvm@4") ]] || exit
    [[ ! -z $(echo "${output}" | grep ${HOME}.*/src/boost) ]] || exit
    [[ ! -z $(echo "${output}" | grep "Starting EOSIO Build") ]] || exit
    [[ ! -z $(echo "${output}" | grep " --with-iostreams --with-date_time") ]] || exit # BOOST
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
}