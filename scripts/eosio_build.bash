#!/usr/bin/env bash
set -eo pipefail

VERSION=3.0 # Build script version (change this to re-build the CICD image)
##########################################################################
# This is the EOSIO automated install script for Linux and Mac OS.
# This file was downloaded from https://github.com/EOSIO/eos
#
# Copyright (c) 2017, Respective Authors all rights reserved.
#
# After June 1, 2018 this software is available under the following terms:
#
# The MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# https://github.com/EOSIO/eos/blob/master/LICENSE
##########################################################################

function usage() {
   printf "Usage: $0 OPTION...
  -P          Build with pinned clang and libcxx
  -o TYPE     Build <Debug|Release|RelWithDebInfo|MinSizeRel> (default: Release)
  -s NAME     Core Symbol Name <1-7 characters> (default: SYS)
  -b DIR      Use pre-built boost in DIR
  -i DIR      Directory to use for installing dependencies & EOSIO (default: $HOME)
  -y          Noninteractive mode (answers yes to every prompt)
  -c          Enable Code Coverage
  -d          Generate Doxygen
   \\n" "$0" 1>&2
   exit 1
}

TIME_BEGIN=$( date -u +%s )
if [ $# -ne 0 ]; then
   while getopts "o:s:b:i:ycdhmP" opt; do
      case "${opt}" in
         o )
            options=( "Debug" "Release" "RelWithDebInfo" "MinSizeRel" )
            if [[ "${options[*]}" =~ "${OPTARG}" ]]; then
               CMAKE_BUILD_TYPE=$OPTARG
            else
               echo  "Invalid argument: ${OPTARG}" 1>&2
               usage
            fi
         ;;
         s )
            if [ "${#OPTARG}" -gt 7 ] || [ -z "${#OPTARG}" ]; then
               echo "Invalid argument: ${OPTARG}" 1>&2
               usage
            else
               CORE_SYMBOL_NAME=$OPTARG
            fi
         ;;
         b )
            BOOST_LOCATION=$OPTARG
         ;;
         i )
            INSTALL_LOCATION=$OPTARG
         ;;
         y )
            NONINTERACTIVE=true
            PROCEED=true
         ;;
         c )
            ENABLE_COVERAGE_TESTING=true
         ;;
         d )
            ENABLE_DOXYGEN=true
         ;;
         m )
            ENABLE_MONGO=true
         ;;
         P )
            PIN_COMPILER=true
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

# Load eosio specific helper functions
. ./scripts/helpers/eosio.bash

echo "Beginning build version: ${VERSION}"
echo "$( date -u )"
CURRENT_USER=${CURRENT_USER:-$(whoami)}
echo "User: ${CURRENT_USER}"
# echo "git head id: %s" "$( cat .git/refs/heads/master )"
echo "Current branch: $( execute git rev-parse --abbrev-ref HEAD 2>/dev/null )"

# Prevent a non-git clone from running
ensure-git-clone
# If the same version has already been installed...
previous-install-prompt
# Handle clang/compiler
ensure-compiler
echo $CXX
echo $CC
# Prompt user and asks if we should install mongo or not
prompt-mongo-install
# Setup directories and envs we need (must come last)
setup

execute cd $REPO_ROOT

# Submodules need to be up to date
ensure-submodules-up-to-date

# Use existing cmake on system (either global or specific to eosio)
# Setup based on architecture
echo "Architecture: ${ARCH}"
if [ "$ARCH" == "Linux" ]; then
   export CMAKE=${CMAKE:-${EOSIO_HOME}/bin/cmake}
   [[ $CURRENT_USER == "root" ]] || ensure-sudo
   OPENSSL_ROOT_DIR=/usr/include/openssl
   [[ ! -e /etc/os-release ]] && print_supported_linux_distros_and_exit
   case $NAME in
      "Amazon Linux AMI" | "Amazon Linux")
         FILE="${REPO_ROOT}/scripts/eosio_build_amazonlinux.bash"
      ;;
      "CentOS Linux")
         FILE="${REPO_ROOT}/scripts/eosio_build_centos.bash"
      ;;
      "Ubuntu")
         FILE="${REPO_ROOT}/scripts/eosio_build_ubuntu.bash"
      ;;
      *) print_supported_linux_distros_and_exit;;
   esac
fi

if [ "$ARCH" == "Darwin" ]; then
   # opt/gettext: cleos requires Intl, which requires gettext; it's keg only though and we don't want to force linking: https://github.com/EOSIO/eos/issues/2240#issuecomment-396309884
   # EOSIO_HOME/lib/cmake: mongo_db_plugin.cpp:25:10: fatal error: 'bsoncxx/builder/basic/kvp.hpp' file not found
   LOCAL_CMAKE_FLAGS="-DCMAKE_PREFIX_PATH='/usr/local/opt/gettext;$EOSIO_HOME/lib/cmake' ${LOCAL_CMAKE_FLAGS}" 
   FILE="${SCRIPT_DIR}/eosio_build_darwin.bash"
   OPENSSL_ROOT_DIR=/usr/local/opt/openssl
   export CMAKE=${CMAKE}
fi

( [[ -z "${CMAKE}" ]] && [[ ! -z $(command -v cmake 2>/dev/null) ]] ) && export CMAKE=$(command -v cmake 2>/dev/null)

echo "${COLOR_CYAN}====================================================================================="
echo "======================= ${COLOR_WHITE}Starting EOSIO Dependency Install${COLOR_CYAN} ===========================${COLOR_NC}"
execute cd $SRC_LOCATION
$BUILD_CLANG && export PINNED_TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE='${BUILD_DIR}/pinned_toolchain.cmake'"
. $FILE # Execute OS specific build file
execute cd $REPO_ROOT
echo ""
echo "${COLOR_CYAN}========================================================================"
echo "======================= ${COLOR_WHITE}Starting EOSIO Build${COLOR_CYAN} ===========================${COLOR_NC}"
execute mkdir -p $BUILD_DIR
execute cd $BUILD_DIR

# Find and replace OPT_LOCATION in pinned_toolchain.cmake, then move it into build dir
execute bash -c "sed -e 's~@~$OPT_LOCATION~g' $SCRIPT_DIR/pinned_toolchain.cmake &> $BUILD_DIR/pinned_toolchain.cmake"

# LOCAL_CMAKE_FLAGS
$ENABLE_MONGO && LOCAL_CMAKE_FLAGS="-DBUILD_MONGO_DB_PLUGIN=true ${LOCAL_CMAKE_FLAGS}" # Enable Mongo DB Plugin if user has enabled -m
if $PIN_COMPILER; then
   LOCAL_CMAKE_FLAGS="${PINNED_TOOLCHAIN} -DCMAKE_PREFIX_PATH='${OPT_LOCATION}/llvm4' ${LOCAL_CMAKE_FLAGS}"
else
   LOCAL_CMAKE_FLAGS="-DCMAKE_CXX_COMPILER='${CXX}' -DCMAKE_C_COMPILER='${CC}' ${LOCAL_CMAKE_FLAGS}"
fi
$ENABLE_DOXYGEN && LOCAL_CMAKE_FLAGS="-DBUILD_DOXYGEN='${DOXYGEN}' ${LOCAL_CMAKE_FLAGS}"
$ENABLE_COVERAGE_TESTING && LOCAL_CMAKE_FLAGS="-DENABLE_COVERAGE_TESTING='${ENABLE_COVERAGE_TESTING}' ${LOCAL_CMAKE_FLAGS}"

execute bash -c "$CMAKE -DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}' -DCORE_SYMBOL_NAME='${CORE_SYMBOL_NAME}' -DOPENSSL_ROOT_DIR='${OPENSSL_ROOT_DIR}' -DCMAKE_INSTALL_PREFIX='${EOSIO_HOME}' ${LOCAL_CMAKE_FLAGS} ${REPO_ROOT}"
execute make -j$JOBS
execute cd $REPO_ROOT 1>/dev/null

TIME_END=$(( $(date -u +%s) - $TIME_BEGIN ))

echo " _______  _______  _______ _________ _______"
echo "(  ____ \(  ___  )(  ____   __   __ (  ___  )"
echo "| (    \/| (   ) || (    \/   ) (   | (   ) |"
echo "| (__    | |   | || (_____    | |   | |   | |"
echo "|  __)   | |   | |(_____  )   | |   | |   | |"
echo "| (      | |   | |      ) |   | |   | |   | |"
echo "| (____/\| (___) |/\____) |___) (___| (___) |"
echo "(_______/(_______)\_______)\_______/(_______)"
echo "=============================================${COLOR_NC}"

echo "${COLOR_GREEN}EOSIO has been successfully built. $(($TIME_END/3600)):$(($TIME_END%3600/60)):$(($TIME_END%60))"
echo "${COLOR_GREEN}You can now install using: ./scripts/eosio_install.bash${COLOR_NC}"
echo "${COLOR_YELLOW}Uninstall with: ./scripts/eosio_uninstall.bash${COLOR_NC}"

echo ""
echo "${COLOR_CYAN}If you wish to perform tests to ensure functional code:${COLOR_NC}"
$ENABLE_MONGO && echo "${BIN_LOCATION}/mongod --dbpath ${MONGODB_DATA_LOCATION} -f ${MONGODB_CONF} --logpath ${MONGODB_LOG_LOCATION}/mongod.log &"
echo "cd ./build && PATH=\$PATH:$OPT_LOCATION/mongodb/bin make test" # PATH is set as currently 'mongo' binary is required for the mongodb test

echo ""
resources
