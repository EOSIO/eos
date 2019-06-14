#!/usr/bin/env bats
load helpers/general

# A helper function is available to show output and status: `debug`

# Load helpers (BE CAREFUL)
. ./scripts/helpers/general.sh

TEST_LABEL="[helpers]"

@test "${TEST_LABEL} > execute > dryrun" {
  ## DRYRUN WORKS (true, false, and empty)
  run execute exit 1
  ( [[ $output =~ "Executing: exit 1" ]] && [[ $status -eq 0 ]] ) || exit
  DRYRUN=false
  run execute exit 1
  ( [[ $output =~ "Executing: exit 1" ]] && [[ $status -eq 1 ]] ) || exit
}

@test "${TEST_LABEL} > execute > verbose" {
  ## VERBOSE WORKS (true, false, and empty)
  run execute echo "VERBOSE!"
  ( [[ $output =~ "Executing: echo VERBOSE!" ]] && [[ $status -eq 0 ]] ) || exit
  VERBOSE=false
  run execute echo "VERBOSE!"
  ( [[ ! $output =~ "Executing: echo VERBOSE!" ]] && [[ $status -eq 0 ]] ) || exit
  VERBOSE=
  ( [[ ! $output =~ "Executing: echo VERBOSE!" ]] && [[ $status -eq 0 ]] ) || exit
}

@test "${TEST_LABEL} > install directory prompt" {
  NONINTERACTIVE=true
  PROCEED=true
  run install-directory-prompt
  # Use default location
  [[ ! -z $(echo "${output}" | grep "home") ]] || exit
  NONINTERACTIVE=false
  PROCEED=false
  INSTALL_LOCATION="/etc/eos"
  run install-directory-prompt
  # Function received given input.
  [[ ! -z $(echo "${output}") ]] || exit
}

@test "${TEST_LABEL} > previous install prompt" {
  NONINTERACTIVE=true
  PROCEED=true
  # Doesn't exists, no output
  run previous-install-prompt
  [[ -z $(echo "${output}") ]] || exit
  # Exists, prompt
  mkdir -p $EOSIO_INSTALL_DIR
  run previous-install-prompt
  [[ ! -z $(echo "${output}" | grep "EOSIO has already been installed into ${EOSIO_INSTALL_DIR}") ]] || exit
  rm -rf $EOSIO_INSTALL_DIR
}

@test "${TEST_LABEL} > TEMP_DIR" {
  run setup
  [[ -z $(echo "${output}" | grep "Executing: rm -rf ${REPO_ROOT}/../tmp/*") ]] || exit
  [[ -z $(echo "${output}" | grep "Executing: mkdir -p ${REPO_ROOT}/../tmp") ]] || exit
}