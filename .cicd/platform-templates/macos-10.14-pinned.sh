#!/bin/bash
set -eo pipefail
EOSIO_LOCATION=/root/eosio
EOSIO_INSTALL_LOCATION=/root/eosio/install
VERSION=1
# Commands from the documentation are inserted right below this line
# Anything below here is exclusive to our CI/CD
## Remove the INSTALL LOCATION bin (set in the docs) and build dir so we don't mess with CI
rm -rf ${EOSIO_LOCATION}/build