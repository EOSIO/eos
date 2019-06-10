#!/bin/bash
set -e # exit on failure of any "simple" command (excludes &&, ||, or | chains)
cd /eosio.contracts
./build.sh
cd build
tar -pczf /artifacts/contracts.tar.gz *
