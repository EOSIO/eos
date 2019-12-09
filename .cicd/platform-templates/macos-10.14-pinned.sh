#!/bin/bash
set -eo pipefail
EOSIO_LOCATION=/Users/anka/eosio
EOSIO_INSTALL_LOCATION=/Users/anka/eosio/install
VERSION=1
# Commands from the documentation are inserted right below this line
# Anything below here is exclusive to our CI/CD
## Cleanup eosio directory (~ 600MB)
rm -rf ${EOSIO_LOCATION}