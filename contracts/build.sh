#!/usr/bin/env bash
set -eo pipefail

function usage() {
   printf "Usage: $0 OPTION...
  -e DIR      Directory where EOSIO is installed. (Default: $HOME/eosio/X.Y)
  -c DIR      Directory where EOSIO.CDT is installed. (Default: /usr/local/eosio.cdt)
  -t          Build unit tests.
  -y          Noninteractive mode (Uses defaults for each prompt.)
  -h          Print this help menu.
   \\n" "$0" 1>&2
   exit 1
}

BUILD_TESTS=false

if [ $# -ne 0 ]; then
  while getopts "e:c:tyh" opt; do
    case "${opt}" in
      e )
        EOSIO_DIR_PROMPT=$OPTARG
      ;;
      c )
        CDT_DIR_PROMPT=$OPTARG
      ;;
      t )
        BUILD_TESTS=true
      ;;
      y )
        NONINTERACTIVE=true
        PROCEED=true
      ;;
      h )
        usage
      ;;
      ? )
        echo "Invalid Option!" 1>&2
        usage
      ;;
      : )
        echo "Invalid Option: -${OPTARG} requires an argument." 1>&2
        usage
      ;;
      * )
        usage
      ;;
    esac
  done
fi

# Source helper functions and variables.
. ./scripts/.environment
. ./scripts/helper.sh

if [[ ${BUILD_TESTS} == true ]]; then
   # Prompt user for location of eosio.
   eosio-directory-prompt
fi

# Prompt user for location of eosio.cdt.
cdt-directory-prompt

# Include CDT_INSTALL_DIR in CMAKE_FRAMEWORK_PATH
echo "Using EOSIO.CDT installation at: $CDT_INSTALL_DIR"
export CMAKE_FRAMEWORK_PATH="${CDT_INSTALL_DIR}:${CMAKE_FRAMEWORK_PATH}"

if [[ ${BUILD_TESTS} == true ]]; then
   # Ensure eosio version is appropriate.
   nodeos-version-check

   # Include EOSIO_INSTALL_DIR in CMAKE_FRAMEWORK_PATH
   echo "Using EOSIO installation at: $EOSIO_INSTALL_DIR"
   export CMAKE_FRAMEWORK_PATH="${EOSIO_INSTALL_DIR}:${CMAKE_FRAMEWORK_PATH}"
fi

printf "\t=========== Building rem.contracts ===========\n\n"
RED='\033[0;31m'
NC='\033[0m'
CPU_CORES=$(getconf _NPROCESSORS_ONLN)
mkdir -p build
pushd build &> /dev/null
cmake -DBUILD_TESTS=${BUILD_TESTS} ../
make -j $CPU_CORES
popd &> /dev/null
