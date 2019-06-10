#!/bin/bash
set -e # exit on failure of any "simple" command (excludes &&, ||, or | chains)
# expected places to find EOSIO CMAKE in the docker container, in ascending order of preference
[[ -e /usr/lib/eosio/lib/cmake/eosio/eosio-config.cmake ]] && export CMAKE_FRAMEWORK_PATH="/usr/lib/eosio"
[[ -e /opt/eosio/lib/cmake/eosio/eosio-config.cmake ]] && export CMAKE_FRAMEWORK_PATH="/opt/eosio"
[[ ! -z "$EOSIO_ROOT" && -e $EOSIO_ROOT/lib/cmake/eosio/eosio-config.cmake ]] && export CMAKE_FRAMEWORK_PATH="$EOSIO_ROOT"
# fail if we didn't find it
[[ -z "$CMAKE_FRAMEWORK_PATH" ]] && exit 1
# export variables
echo "" >> /eosio.contracts/docker/environment.Dockerfile # necessary if there is no '\n' at end of file
echo "ENV CMAKE_FRAMEWORK_PATH=$CMAKE_FRAMEWORK_PATH" >> /eosio.contracts/docker/environment.Dockerfile
echo "ENV EOSIO_ROOT=$CMAKE_FRAMEWORK_PATH" >> /eosio.contracts/docker/environment.Dockerfile