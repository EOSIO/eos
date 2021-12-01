#!/bin/bash
set -eo pipefail
echo '+++ :guide_dog: New Job Ensurer'
echo 'These steps are load testing the new Buildkite job ensurer:'
printf "\033]1339;url=https://github.com/EOSIO/auto-buildkite-job-runner;content=https://github.com/EOSIO/auto-buildkite-job-runner\a\n"
export SLEEP_COMMAND="sleep ${$SLEEP:-60}"
echo "$ $SLEEP_COMMAND"
eval $SLEEP_COMMAND
echo '--- :white_check_mark: Done!'
