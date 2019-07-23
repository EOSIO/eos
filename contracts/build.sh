#! /bin/bash
set -e # exit on failure of any "simple" command (excludes &&, ||, or | chains)
printf "\t=========== Building eosio.contracts ===========\n\n"
RED='\033[0;31m'
NC='\033[0m'
CPU_CORES=$(getconf _NPROCESSORS_ONLN)
mkdir -p build
pushd build &> /dev/null
cmake ../
make -j $CPU_CORES
popd &> /dev/null
