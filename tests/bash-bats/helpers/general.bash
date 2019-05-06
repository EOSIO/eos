#!/usr/bin/env bats

# You can add `load helpers/general` to the .bats files you create to include anything in this file.
# DO NOT REMOVE
export DRYRUN=true
export VERBOSE=true
export CURRENT_USER=$(whoami)
export HOME="$BATS_TMPDIR/bats-eosio-user-home" # Ensure $HOME is available for all scripts

load helpers/functions

# Ensure we're in the root directory to execute
if [[ ! -d "tests" ]] && [[ ! -f "README.md" ]]; then
  echo "You must navigate into the root directory to execute tests..." >&3
  exit 1
fi