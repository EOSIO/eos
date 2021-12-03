#!/bin/bash
set -eo pipefail
echo '+++ :guide_dog: New Job Ensurer'
echo 'These steps are load testing the new Buildkite job ensurer:'
printf "\033]1339;url=https://github.com/EOSIO/auto-buildkite-job-runner;content=https://github.com/EOSIO/auto-buildkite-job-runner\a\n"
echo 'sleeping for 60s'
sleep 60
echo '--- :white_check_mark: Done!'
