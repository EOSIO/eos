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

	VERSION=1.1
	ULIMIT=$( ulimit -u )
	WORK_DIR=$PWD
	BUILD_DIR=${WORK_DIR}/build
	TEMP_DIR=/tmp
	ARCH=$( uname )
	DISK_MIN=20
	
	txtbld=$(tput bold)
	bldred=${txtbld}$(tput setaf 1)
	txtrst=$(tput sgr0)

	printf "\n\tARCHITECTURE: ${ARCH}\n"

	if [ $ARCH == "Linux" ]; then
		
		if [ ! -e /etc/os-release ]; then
			printf "\n\tEOSIO currently supports Amazon, Centos, Fedora, Mint & Ubuntu Linux only.\n"
			printf "\tPlease install on the latest version of one of these Linux distributions.\n"
			printf "\thttps://aws.amazon.com/amazon-linux-ami/\n"
			printf "\thttps://www.centos.org/\n"
			printf "\thttps://start.fedoraproject.org/\n"
			printf "\thttps://linuxmint.com/\n"
			printf "\thttps://www.ubuntu.com/\n"
			printf "\tExiting now.\n"
			exit 1
		fi
	
		OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )
	
		case $OS_NAME in
			"Amazon Linux AMI")
				FILE=${WORK_DIR}/scripts/eosio_build_amazon.sh
				CXX_COMPILER=g++
				C_COMPILER=gcc
				MONGOD_CONF=${HOME}/opt/mongodb/mongod.conf
				export LLVM_DIR=${HOME}/opt/wasm/lib/cmake/llvm
				export CMAKE=${HOME}/opt/cmake/bin/cmake
				export PATH=${HOME}/opt/mongodb/bin:$PATH
			;;
			"CentOS Linux")
				FILE=${WORK_DIR}/scripts/eosio_build_centos.sh
				CXX_COMPILER=g++
				C_COMPILER=gcc
				MONGOD_CONF=${HOME}/opt/mongodb/mongod.conf
				export LLVM_DIR=${HOME}/opt/wasm/lib/cmake/llvm
				export CMAKE=${HOME}/opt/cmake/bin/cmake
				export PATH=${HOME}/opt/mongodb/bin:$PATH
			;;
			"Fedora")
				FILE=${WORK_DIR}/scripts/eosio_build_fedora.sh
				CXX_COMPILER=g++
				C_COMPILER=gcc
				MONGOD_CONF=/etc/mongod.conf
				export LLVM_DIR=${HOME}/opt/wasm/lib/cmake/llvm
			;;
			"Linux Mint")
				FILE=${WORK_DIR}/scripts/eosio_build_ubuntu.sh
				CXX_COMPILER=clang++-4.0
				C_COMPILER=clang-4.0
				MONGOD_CONF=/etc/mongod.conf
			;;
			"Ubuntu")
				FILE=${WORK_DIR}/scripts/eosio_build_ubuntu.sh
				CXX_COMPILER=clang++-4.0
				C_COMPILER=clang-4.0
				MONGOD_CONF=/etc/mongod.conf
			;;
			*)
				printf "\n\tUnsupported Linux Distribution. Exiting now.\n\n"
				exit 1
		esac
		
		export BOOST_ROOT=${HOME}/opt/boost_1_66_0
		export OPENSSL_ROOT_DIR=/usr/include/openssl
		export OPENSSL_LIBRARIES=/usr/include/openssl
		export WASM_ROOT=${HOME}/opt/wasm
	fi

	if [ $ARCH == "Darwin" ]; then
		FILE=${WORK_DIR}/scripts/eosio_build_darwin.sh
		CXX_COMPILER=clang++
		C_COMPILER=clang
		MONGOD_CONF=/usr/local/etc/mongod.conf
		OPENSSL_ROOT_DIR=/usr/local/opt/openssl
		OPENSSL_LIBRARIES=/usr/local/opt/openssl/lib
		export WASM_ROOT=/usr/local/wasm
	fi

	. $FILE

	printf "\n\n>>>>>>>> ALL dependencies sucessfully found or installed . Installing EOS.IO\n\n"

	COMPILE_EOS=1
	COMPILE_CONTRACTS=1
	CMAKE_BUILD_TYPE=RelWithDebInfo

	cd ${WORK_DIR}
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}

	if [ -z $CMAKE ]; then
		CMAKE=$( which cmake )
	fi
	
	$CMAKE -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
	-DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_ROOT=${WASM_ROOT} \
	-DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DBUILD_MONGO_DB_PLUGIN=true \
	-DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} ..
	
	if [ $? -ne 0 ]; then
		printf "\n\t>>>>>>>>>>>>>>>>>>>> CMAKE building EOSIO has exited with the above error.\n\n"
		exit -1
	fi

	make -j${CPU_CORE}

	if [ $? -ne 0 ]; then
		printf "\n\t>>>>>>>>>>>>>>>>>>>> MAKE building EOSIO has exited with the above error.\n\n"
		exit -1
	fi

	printf "\n\t>>>>>>>>>>>>>>>>>>>> EOSIO has been successfully built.\n\n"

	printf "\n\tVerifying MongoDB is running.\n"
	MONGODB_PID=$( pgrep -x mongod )
	if [ -z $MONGODB_PID ]; then
		printf "\tMongoDB is not currently running.\n"
		printf "\tStarting MongoDB.\n"
		mongod -f ${MONGOD_CONF} &
		if [ $? -ne 0 ]; then
			printf "\n\tUnable to start MongoDB.\nExiting now.\n\n"
			exit -1
		fi
		MONGODB_PID=$( pgrep -x mongod )
		printf "\n\tSuccessfully started MongoDB PID = ${MONGODB_PID}.\n"
	else
		printf "\n\tMongoDB is running PID=${MONGODB_PID}.\n"
	fi
	
   if [ "x${EOSIO_BUILD_PACKAGE}" != "x" ]; then
      # Build eos.io package
      $CMAKE -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
      -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_ROOT=${WASM_ROOT} \
      -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} \
      -DCMAKE_INSTALL_PREFIX=/usr ..

      if [ $? -ne 0 ]; then
         printf "\n\t>>>>>>>>>>>>>>>>>>>> CMAKE building eos.io package has exited with the above error.\n\n"
         exit -1
      fi

      make -j${CPU_CORE} VERBOSE=0 package

      if [ $? -ne 0 ]; then
         printf "\n\t>>>>>>>>>>>>>>>>>>>> MAKE building eos.io package has exited with the above error.\n\n"
         exit -1
      fi

      printf "\n\t>>>>>>>>>>>>>>>>>>>> eos.io package has been successfully built.\n\n"
   fi