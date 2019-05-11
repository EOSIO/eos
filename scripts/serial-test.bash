#!/usr/bin/env bash
set -ieo pipefail
# Load eosio specific helper functions
. ./scripts/helpers/eosio.bash

execute cd ./build

execute $CTEST_BIN -L nonparallelizable_tests --output-on-failure -T Test
