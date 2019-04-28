#!/usr/bin/env bats
load test_helper

SCRIPT_LOCATION="scripts/eosio_build.bash"
TEST_LABEL="[eosio_build_darwin]"

[[ $ARCH == "Darwin" ]] || exit 0 # Exit 0 is required for pipeline
[[ $NAME == "Mac OS X" ]] || exit 0 # Exit 0 is required for pipeline

# A helper function is available to show output and status: `debug`

@test "${TEST_LABEL} > Testing Options" {
    run bash -c "./$SCRIPT_LOCATION -y -i /tmp -b /boost_tmp"
    [[ ! -z $(echo "${output}" | grep "CMAKE_INSTALL_PREFIX='/tmp/eosio") ]] || exit
    [[ ! -z $(echo "${output}" | grep "@ /boost_tmp") ]] || exit
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
    [[ -z $(echo "${output}" | grep "Checking MongoDB installation") ]] || exit
    ## -m
    run bash -c "./$SCRIPT_LOCATION -y -m"
    [[ ! -z $(echo "${output}" | grep "Checking MongoDB installation") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Installing MongoDB C++ driver...") ]] || exit
}

@test "${TEST_LABEL} > Testing Prompts" {
  ## All yes pass
  run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
  [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
  ## Brew no shows abort
  run bash -c "printf \"n\n\" | ./$SCRIPT_LOCATION" # This will fail if you've got the brew packages installed on the machine running these tests!
  [[ "${output##*$'\n'}" =~ "- User aborted installation of required dependencies." ]] || exit
}

@test "${TEST_LABEL} > Testing Executions" {
  export CMAKE=${BATS_TMPDIR}/cmake # Testing for if CMAKE already exists
  touch $CMAKE
  run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
  ### Make sure deps are loaded properly
  [[ ! -z $(echo "${output}" | grep "Starting EOSIO Dependency Install") ]] || exit
  [[ ! -z $(echo "${output}" | grep "Executing: /usr/bin/xcode-select --install") ]] || exit
  ## Testing if formula is installed
  [[ ! -z $(echo "${output}" | grep "cmake.*found") ]] || exit
  [[ ! -z $(echo "${output}" | grep "automake.*NOT.*found") ]] || exit
  rm -f $CMAKE
  [[ ! -z $(echo "${output}" | grep "[Updating HomeBrew]") ]] || exit
  [[ ! -z $(echo "${output}" | grep "brew tap eosio/eosio") ]] || exit
  [[ ! -z $(echo "${output}" | grep "brew install.*llvm@4.*") ]] || exit
  [[ ! -z $(echo "${output}" | grep ${HOME}.*/src/boost) ]] || exit
  [[ ! -z $(echo "${output}" | grep "Starting EOSIO Build") ]] || exit
  [[ ! -z $(echo "${output}" | grep "Executing: bash -c ${CMAKE}") ]] || exit
  ## Testing for if cmake doesn't exist to be sure it's set properly
  export CMAKE=
  run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
  [[ ! -z $(echo "${output}" | grep "Executing: bash -c /usr/local/bin/cmake -DCMAKE_BUILD") ]] || exit
  ## CLANG
  run bash -c "./$SCRIPT_LOCATION -y -P"
  [[ ! -z $(echo "${output}" | grep "Checking Clang support") ]] || exit
  [[ ! -z $(echo "${output}" | grep -E "Clang.*successfully installed @ ${CLANG_ROOT}") ]] || exit
  ## CLANG already exists (c++/default)
  run bash -c "./$SCRIPT_LOCATION -y"
  [[ ! -z $(echo "${output}" | grep "DCMAKE_CXX_COMPILER='c++'") ]] || exit
  [[ ! -z $(echo "${output}" | grep "DCMAKE_C_COMPILER='cc'") ]] || exit
  ## CXX doesn't exist
  export CXX=c2234
  export CC=ewwqd
  run bash -c "./$SCRIPT_LOCATION -y"
  [[ ! -z $(echo "${output}" | grep "Unable to find compiler c2234") ]] || exit
}