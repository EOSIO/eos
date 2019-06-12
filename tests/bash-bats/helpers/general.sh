#!/usr/bin/env bats

# You can add `load helpers/general` to the .bats files you create to include anything in this file.
# DO NOT REMOVE
export DRYRUN=true
export VERBOSE=true
export BATS_RUN=true
export CURRENT_USER=$(whoami)
export HOME="$BATS_TMPDIR/bats-eosio-user-home" # Ensure $HOME is available for all scripts
load helpers/functions

if [[ $NAME == "Ubuntu" ]]; then # Ubuntu won't find any packages until this runs + ensure update only runs once
  [[ -z $(find /tmp/apt-updated -mmin -60 2>/dev/null) ]] && apt-get update &>/dev/null
  [[ ! -f /tmp/apt-updated ]] && touch /tmp/apt-updated
else
  [[ $ARCH != "Darwin" ]] && yum -y update &>/dev/null
fi

# Ensure we're in the root directory to execute
if [[ ! -d "tests" ]] && [[ ! -f "README.md" ]]; then
  echo "You must navigate into the root directory to execute tests..." >&3
  exit 1
fi