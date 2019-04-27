#!/usr/bin/env bats
load test_helper

# A helper function is available to show output and status: `debug`

# Load helpers (BE CAREFUL)
. ./scripts/helpers/general.bash

TEST_LABEL="[helpers]"

@test "${TEST_LABEL} > execute > dryrun" {
  ## DRYRUN WORKS (true, false, and empty)
  run execute exit 1
  ( [[ $output =~ "Executing: exit 1" ]] && [[ $status -eq 0 ]] ) || exit
  DRYRUN=false
  run execute exit 1
  ( [[ $output =~ "Executing: exit 1" ]] && [[ $status -eq 1 ]] ) || exit
  DRYRUN=
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

@test "${TEST_LABEL} > previous install prompt" {
  NONINTERACTIVE=true
  PROCEED=true
  # Doesn't exists, no output
  run previous-install-prompt
  [[ -z $(echo "${output}") ]] || exit
  # Exists, prompt
  mkdir -p $EOSIO_HOME
  run previous-install-prompt
  [[ ! -z $(echo "${output}" | grep "EOSIO has already been installed into ${EOSIO_HOME}") ]] || exit
  rm -rf $EOSIO_HOME
}