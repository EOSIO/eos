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
		printf "\\n\\tPlease cd into directory %s to run this script.\\n \\tExiting now.\\n\\n" "${CWD}"
		exit 1
	fi
	if [ -f "${PWD}/CMakeCache.txt" ]; then
		printf "\\n\\tPlease remove file %s/CMakeCache.txt before building EOSIO.\\n \\tExiting now.\\n\\n" "${PWD}"
		exit 1
	fi

   	function usage()
   	{ 
		printf "\\tUsage: %s \\n\\t[Build Option -o <Debug|Release|RelWithDebInfo|MinSizeRel>] \\n\\t[CodeCoverage -c ] \\n\\t[Doxygen -d] \\n\\t[CoreSymbolName -s <1-7 characters>]\\n\\n" "$0" 1>&2
		exit 1
	}

	ARCH=$( uname )
	BUILD_DIR="${PWD}/build"
	CMAKE_BUILD_TYPE=Release
	DISK_MIN=20
	DOXYGEN=false
	ENABLE_COVERAGE_TESTING=false
	CORE_SYMBOL_NAME="SYS"
	TEMP_DIR="/tmp"
	TIME_BEGIN=$( date -u +%s )
	VERSION=1.2

	txtbld=$(tput bold)
	bldred=${txtbld}$(tput setaf 1)
	txtrst=$(tput sgr0)

	if [ $# -ne 0 ]; then
		while getopts ":cdo:s:" opt; do
			case "${opt}" in
				o )
					options=( "Debug" "Release" "RelWithDebInfo" "MinSizeRel" )
					if [[ "${options[*]}" =~ "${OPTARG}" ]]; then
						CMAKE_BUILD_TYPE="${OPTARG}"
					else
						printf "\\n\\tInvalid argument: %s\\n" "${OPTARG}" 1>&2
						usage
						exit 1
					fi
				;;
				c )
					ENABLE_COVERAGE_TESTING=true
				;;
				d )
					DOXYGEN=true
				;;
				s)
					if [ "${#OPTARG}" -gt 6 ] || [ -z "${#OPTARG}" ]; then
						printf "\\n\\tInvalid argument: %s\\n" "${OPTARG}" 1>&2
						usage
						exit 1
					else
						CORE_SYMBOL_NAME="${OPTARG}"
					fi
				;;
				\? )
					printf "\\n\\tInvalid Option: %s\\n" "-${OPTARG}" 1>&2
					usage
					exit 1
				;;		
				: )
					printf "\\n\\tInvalid Option: %s requires an argument.\\n" "-${OPTARG}" 1>&2
					usage
					exit 1
				;;
				* )
					usage
					exit 1
				;;
			esac
		done
	fi

	if [ ! -d .git ]; then
		printf "\\n\\tThis build script only works with sources cloned from git\\n"
		printf "\\tPlease clone a new eos directory with 'git clone https://github.com/EOSIO/eos --recursive'\\n"
		printf "\\tSee the wiki for instructions: https://github.com/EOSIO/eos/wiki\\n"
		exit 1
	fi

	STALE_SUBMODS=$(( $(git submodule status | grep -c "^[+\-]") ))
	if [ $STALE_SUBMODS -gt 0 ]; then
		printf "\\n\\tgit submodules are not up to date.\\n"
		printf "\\tPlease run the command 'git submodule update --init --recursive'.\\n"
		exit 1
	fi

	printf "\\n\\tBeginning build version: %s\\n" "${VERSION}"
	printf "\\t%s\\n" "$( date -u )"
	printf "\\tUser: %s\\n" "$( whoami )"
	printf "\\tgit head id: %s\\n" "$( cat .git/refs/heads/master )"
	printf "\\tCurrent branch: %s\\n" "$( git rev-parse --abbrev-ref HEAD )"
	printf "\\n\\tARCHITECTURE: %s\\n" "${ARCH}"

	if [ "$ARCH" == "Linux" ]; then
		
		if [ ! -e /etc/os-release ]; then
			printf "\\n\\tEOSIO currently supports Amazon, Centos, Fedora, Mint & Ubuntu Linux only.\\n"
			printf "\\tPlease install on the latest version of one of these Linux distributions.\\n"
			printf "\\thttps://aws.amazon.com/amazon-linux-ami/\\n"
			printf "\\thttps://www.centos.org/\\n"
			printf "\\thttps://start.fedoraproject.org/\\n"
			printf "\\thttps://linuxmint.com/\\n"
			printf "\\thttps://www.ubuntu.com/\\n"
			printf "\\tExiting now.\\n"
			exit 1
		fi
	
		OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )
	
		case "$OS_NAME" in
			"Amazon Linux AMI")
				FILE="${PWD}/scripts/eosio_build_amazon.sh"
				CXX_COMPILER=g++
				C_COMPILER=gcc
				MONGOD_CONF=${HOME}/opt/mongodb/mongod.conf
				export LLVM_DIR=${HOME}/opt/wasm/lib/cmake/llvm
				export CMAKE=${HOME}/opt/cmake/bin/cmake
				export PATH=${HOME}/opt/mongodb/bin:$PATH
			;;
			"CentOS Linux")
				FILE="${PWD}/scripts/eosio_build_centos.sh"
				CXX_COMPILER=g++
				C_COMPILER=gcc
				MONGOD_CONF=${HOME}/opt/mongodb/mongod.conf
				export LLVM_DIR=${HOME}/opt/wasm/lib/cmake/llvm
				export CMAKE=${HOME}/opt/cmake/bin/cmake
				export PATH=${HOME}/opt/mongodb/bin:$PATH
			;;
			"elementary OS")
				FILE="${PWD}/scripts/eosio_build_ubuntu.sh"
				CXX_COMPILER=clang++-4.0
				C_COMPILER=clang-4.0
				MONGOD_CONF=${HOME}/opt/mongodb/mongod.conf
				export PATH=${HOME}/opt/mongodb/bin:$PATH
			;;
			"Fedora")
				FILE="${PWD}/scripts/eosio_build_fedora.sh"
				CXX_COMPILER=g++
				C_COMPILER=gcc
				MONGOD_CONF=/etc/mongod.conf
				export LLVM_DIR=${HOME}/opt/wasm/lib/cmake/llvm
			;;
			"Linux Mint")
				FILE="${PWD}/scripts/eosio_build_ubuntu.sh"
				CXX_COMPILER=clang++-4.0
				C_COMPILER=clang-4.0
				MONGOD_CONF=${HOME}/opt/mongodb/mongod.conf
				export PATH=${HOME}/opt/mongodb/bin:$PATH
			;;
			"Ubuntu")
				FILE="${PWD}/scripts/eosio_build_ubuntu.sh"
				CXX_COMPILER=clang++-4.0
				C_COMPILER=clang-4.0
				MONGOD_CONF=${HOME}/opt/mongodb/mongod.conf
				export PATH=${HOME}/opt/mongodb/bin:$PATH
			;;
			*)
				printf "\\n\\tUnsupported Linux Distribution. Exiting now.\\n\\n"
				exit 1
		esac

		export BOOST_ROOT="${HOME}/opt/boost"
		OPENSSL_ROOT_DIR=/usr/include/openssl
		WASM_ROOT="${HOME}/opt/wasm"
	fi

	if [ "$ARCH" == "Darwin" ]; then
		FILE="${PWD}/scripts/eosio_build_darwin.sh"
		CXX_COMPILER=clang++
		C_COMPILER=clang
		MONGOD_CONF=/usr/local/etc/mongod.conf
		OPENSSL_ROOT_DIR=/usr/local/opt/openssl
		WASM_ROOT=/usr/local/wasm
	fi

	. "$FILE"

	printf "\\n\\n>>>>>>>> ALL dependencies sucessfully found or installed . Installing EOSIO\\n\\n"
	printf ">>>>>>>> CMAKE_BUILD_TYPE=%s\\n" "${CMAKE_BUILD_TYPE}"
	printf ">>>>>>>> ENABLE_COVERAGE_TESTING=%s\\n" "${ENABLE_COVERAGE_TESTING}"
	printf ">>>>>>>> DOXYGEN=%s\\n\\n" "${DOXYGEN}"

	if [ ! -d "${BUILD_DIR}" ]; then
		if ! mkdir -p "${BUILD_DIR}"
		then
			printf "Unable to create build directory %s.\\n Exiting now.\\n" "${BUILD_DIR}"
			exit 1;
		fi
	fi
	
	if ! cd "${BUILD_DIR}"
	then
		printf "Unable to enter build directory %s.\\n Exiting now.\\n" "${BUILD_DIR}"
		exit 1;
	fi
	
	if [ -z "$CMAKE" ]; then
		CMAKE=$( command -v cmake )
	fi
	
	if ! "${CMAKE}" -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" \
		-DCMAKE_C_COMPILER="${C_COMPILER}" -DWASM_ROOT="${WASM_ROOT}" -DCORE_SYMBOL_NAME="${CORE_SYMBOL_NAME}" \
		-DOPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}" -DBUILD_MONGO_DB_PLUGIN=true \
		-DENABLE_COVERAGE_TESTING="${ENABLE_COVERAGE_TESTING}" -DBUILD_DOXYGEN="${DOXYGEN}" ..
	then
		printf "\\n\\t>>>>>>>>>>>>>>>>>>>> CMAKE building EOSIO has exited with the above error.\\n\\n"
		exit -1
	fi

	if ! make -j"${CPU_CORE}"
	then
		printf "\\n\\t>>>>>>>>>>>>>>>>>>>> MAKE building EOSIO has exited with the above error.\\n\\n"
		exit -1
	fi
	
	TIME_END=$(( $(date -u +%s) - ${TIME_BEGIN} ))

	printf "\n\n${bldred}\t _______  _______  _______ _________ _______\n"
	printf '\t(  ____ \(  ___  )(  ____ \\\\__   __/(  ___  )\n'
	printf "\t| (    \/| (   ) || (    \/   ) (   | (   ) |\n"
	printf "\t| (__    | |   | || (_____    | |   | |   | |\n"
	printf "\t|  __)   | |   | |(_____  )   | |   | |   | |\n"
	printf "\t| (      | |   | |      ) |   | |   | |   | |\n"
	printf "\t| (____/\| (___) |/\____) |___) (___| (___) |\n"
	printf "\t(_______/(_______)\_______)\_______/(_______)\n${txtrst}"

	printf "\\n\\tEOSIO has been successfully built. %02d:%02d:%02d\\n\\n" $(($TIME_END/3600)) $(($TIME_END%3600/60)) $(($TIME_END%60))
	printf "\\tTo verify your installation run the following commands:\\n"
	
	print_instructions

	printf "\\tFor more information:\\n"
	printf "\\tEOSIO website: https://eos.io\\n"
	printf "\\tEOSIO Telegram channel @ https://t.me/EOSProject\\n"
	printf "\\tEOSIO resources: https://eos.io/resources/\\n"
	printf "\\tEOSIO wiki: https://github.com/EOSIO/eos/wiki\\n\\n\\n"
				
	if [ "x${EOSIO_BUILD_PACKAGE}" != "x" ]; then
	  # Build eos.io package
		if ! "$CMAKE" -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" \
			-DCMAKE_C_COMPILER="${C_COMPILER}" -DWASM_ROOT="${WASM_ROOT}" \
			-DOPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}" -DCMAKE_INSTALL_PREFIX="/usr" ..
		then
			printf "\\n\\t>>>>>>>>>>>>>>>>>>>> CMAKE building eos.io package has exited with the above error.\\n\\n"
			exit -1
		fi

		if ! make -j${CPU_CORE} VERBOSE=0 package
		then
			printf "\\n\\t>>>>>>>>>>>>>>>>>>>> MAKE building eos.io package has exited with the above error.\\n\\n"
			exit -1
		fi

		printf "\\n\\t>>>>>>>>>>>>>>>>>>>> eos.io package has been successfully built.\\n\\n"
	fi
