#!/bin/bash
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
# https://github.com/EOSIO/eos/blob/master/LICENSE.txt
##########################################################################

CWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ "${CWD}" != "${PWD}" ]; then
   printf "\\nPlease cd into directory %s to run this script.\\n Exiting now.\\n\\n" "${CWD}"
   exit 1
fi

OPT_LOCATION=$HOME/opt
BIN_LOCATION=$HOME/bin
LIB_LOCATION=$HOME/lib
mkdir -p $LIB_LOCATION

BUILD_DIR="${PWD}/build"
CMAKE_BUILD_TYPE=Release
TIME_BEGIN=$( date -u +%s )
INSTALL_PREFIX=$OPT_LOCATION/eosio
VERSION=1.2

txtbld=$(tput bold)
bldred=${txtbld}$(tput setaf 1)
txtrst=$(tput sgr0)

create_symlink() {
   pushd $BIN_LOCATION &> /dev/null
   printf " ln -sf ${OPT_LOCATION}/eosio/bin/${1} ${BIN_LOCATION}/${1}\\n"
   ln -sf $OPT_LOCATION/eosio/bin/$1 ${BIN_LOCATION}/${1}
   popd &> /dev/null
}

create_cmake_symlink() {
   mkdir -p $LIB_LOCATION/cmake/eosio
   pushd $LIB_LOCATION/cmake/eosio &> /dev/null
   ln -sf ../../../eosio/lib/cmake/eosio/$1 $1
   popd &> /dev/null
}

install_symlinks() {
   printf "\\nInstalling EOSIO Binary Symlinks...\\n"
   create_symlink "cleos"
   create_symlink "eosio-launcher"
   create_symlink "keosd"
   create_symlink "nodeos"
   printf " - Installed binaries into ${BIN_LOCATION}!"
}

if [ ! -d $BUILD_DIR ]; then
   printf "\\nError, eosio_build.sh has not ran.  Please run ./eosio_build.sh first!\\n\\n"
   exit -1
fi

${PWD}/scripts/clean_old_install.sh
if [ $? -ne 0 ]; then
   printf "\\nError occurred while trying to remove old installation!\\n\\n"
   exit -1
fi

if ! pushd "${BUILD_DIR}"
then
   printf "Unable to enter build directory %s.\\n Exiting now.\\n" "${BUILD_DIR}"
   exit 1;
fi

if ! make install
then
   printf "\\n>>>>>>>>>>>>>>>>>>>> MAKE installing EOSIO has exited with the above error.\\n\\n"
   exit -1
fi
popd &> /dev/null 

install_symlinks   
create_cmake_symlink "eosio-config.cmake"

printf "\n _______  _______  _______ _________ _______\n"
printf '(  ____ \(  ___  )(  ____ \\\\__   __/(  ___  )\n'
printf "| (    \/| (   ) || (    \/   ) (   | (   ) |\n"
printf "| (__    | |   | || (_____    | |   | |   | |\n"
printf "|  __)   | |   | |(_____  )   | |   | |   | |\n"
printf "| (      | |   | |      ) |   | |   | |   | |\n"
printf "| (____/\| (___) |/\____) |___) (___| (___) |\n"
printf "(_______/(_______)\_______)\_______/(_______)\n\n"

printf "==============================================================================================\\n"
printf "Please execute (and append to .bash_profile) the following:\\n"
printf "${bldred}${txtbld}export LD_LIBRARY_PATH=\"${LIB_LOCATION}:$\LD_LIBRARY_PATH\"\\n"
printf "export PATH=$BIN_LOCATION:\$PATH${txtrst}\\n"
printf "==============================================================================================\\n\\n"

printf "For more information:\\n"
printf "EOSIO website: https://eos.io\\n"
printf "EOSIO Telegram channel @ https://t.me/EOSProject\\n"
printf "EOSIO resources: https://eos.io/resources/\\n"
printf "EOSIO Stack Exchange: https://eosio.stackexchange.com\\n"
printf "EOSIO wiki: https://github.com/EOSIO/eos/wiki\\n\\n\\n"
