#!/bin/bash
##########################################################################
# This is EOS bootstrapper script for Linux and OS X.
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

	VERSION=1.1
	ULIMIT=$( ulimit -u )

	# Define directories.
	WORK_DIR=$PWD
	BUILD_DIR=${WORK_DIR}/build
	TEMP_DIR=/tmp
	ARCH=$(uname)
	DISK_MIN=20
	
	txtbld=$(tput bold)
	bldred=${txtbld}$(tput setaf 1)
	txtrst=$(tput sgr0)

	printf "\n\tARCHITECTURE ${ARCH}\n"

	if [ $ARCH == "Linux" ]; then
		
		if [ ! -e /etc/os-release ]; then
			printf "EOSIO currently supports Ubuntu, Red Hat & Fedora Linux only.\n"
			printf "Please install on the latest version of one of these Linux distributions.\n"
			printf "https://www.ubuntu.com/\n"
			printf "https://start.fedoraproject.org/en/\n"
			printf "Exiting now.\n"
			exit 1
		fi
	
		OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )
	
		case $OS_NAME in
			"Ubuntu")
				FILE=${WORK_DIR}/scripts/eosio_build_ubuntu.sh
				CXX_COMPILER=clang++-4.0
				C_COMPILER=clang-4.0
			;;
			"Fedora")
				FILE=${WORK_DIR}/scripts/eosio_build_fedora.sh
				CXX_COMPILER=g++
				C_COMPILER=gcc
			;;
			"Amazon Linux AMI")
				FILE=${WORK_DIR}/scripts/eosio_build_amazon.sh
				CMAKE=${HOME}/opt/cmake/bin/cmake
				CXX_COMPILER=g++
				C_COMPILER=gcc
				export LLVM_DIR=${HOME}/opt/wasm/lib/cmake/llvm
			;;
			*)
				printf "\n\tUnsupported Linux Distribution. Exiting now.\n\n"
				exit 1
		esac
			
		export BOOST_ROOT=${HOME}/opt/boost_1_66_0
		export OPENSSL_ROOT_DIR=/usr/include/openssl
		export OPENSSL_LIBRARIES=/usr/include/openssl
                export WASM_ROOT=${HOME}/opt/wasm
	
	 . $FILE
	
	fi

	if [ $ARCH == "Darwin" ]; then
		OPENSSL_ROOT_DIR=/usr/local/opt/openssl
		OPENSSL_LIBRARIES=/usr/local/opt/openssl/lib
		export WASM_ROOT=/usr/local/wasm
		CXX_COMPILER=clang++
		C_COMPILER=clang

	  . scripts/eosio_build_darwin.sh
	fi

	printf "\n\n>>>>>>>> ALL dependencies sucessfully found or installed . Installing EOS.IO\n\n"

	# Debug flags
	COMPILE_EOS=1
	COMPILE_CONTRACTS=1

	# Define default arguments.
	CMAKE_BUILD_TYPE=RelWithDebugInfo

	# Create the build dir
	cd ${WORK_DIR}
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}

	if [ -z $CMAKE ]; then
		CMAKE=$( which cmake )
	fi
	
	# Build EOS
	$CMAKE -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
  -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_ROOT=${WASM_ROOT} \
  -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} \
	-DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} ..
	if [ $? -ne 0 ]; then
		printf "\n\t>>>>>>>>>>>>>>>>>>>> CMAKE building EOSIO has exited with the above error.\n\n"
		exit -1
	fi

	make -j${CPU_CORE} VERBOSE=0

	if [ $? -ne 0 ]; then
		printf "\n\t>>>>>>>>>>>>>>>>>>>> MAKE building EOSIO has exited with the above error.\n\n"
		exit -1
	fi

	printf "\n\t>>>>>>>>>>>>>>>>>>>> EOSIO has been successfully installed.\n\n"
