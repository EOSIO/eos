#!/usr/bin/env bash

##########################################################################
# This is EOS bootstrapper script for Linux and OS X.
# This file was downloaded from https://github.com/EOSIO/eos
# Feel free to change this file to fit your needs.
##########################################################################

VERSION=1.0
ulimit -u
# Define directories.
WORK_DIR=$PWD
BUILD_DIR=${WORK_DIR}/build
TEMP_DIR=/tmp

# Target architectures
ARCH=$1
TARGET_ARCHS="ubuntu darwin"

# Check ARCH
if [[ $# > 2 ]]; then
  echo ""
  echo "Error: too many arguments"
  exit 1
fi

if [[ $# < 1 ]]; then
  echo ""
  echo "Usage: bash build.sh TARGET [full|build]"
  echo ""
  echo "Targets: $TARGET_ARCHS"
  exit 1
fi

if [[ $ARCH =~ [[:space:]] || ! $TARGET_ARCHS =~ (^|[[:space:]])$ARCH([[:space:]]|$) ]]; then
  echo ""
  echo ">>> WRONG ARCHITECTURE \"$ARCH\""
  exit 1
fi

if [ -z "$2" ]; then
  INSTALL_DEPS=1
else
  if [ "$2" == "full" ]; then
      INSTALL_DEPS=1
  elif [ "$2" == "build" ]; then
      INSTALL_DEPS=0
  else
      echo ">>> WRONG mode use full or build"
      exit 1
  fi
fi

echo ""
echo ">>> ARCHITECTURE \"$ARCH\""

if [ $ARCH == "ubuntu" ]; then
    BOOST_ROOT=${HOME}/opt/boost_1_64_0
    BINARYEN_BIN=${HOME}/opt/binaryen/bin
    OPENSSL_ROOT_DIR=/usr/local/opt/openssl
    OPENSSL_LIBRARIES=/usr/local/opt/openssl/lib
    WASM_LLVM_CONFIG=${HOME}/opt/wasm/bin/llvm-config
fi

if [ $ARCH == "darwin" ]; then
    OPENSSL_ROOT_DIR=/usr/local/opt/openssl
    OPENSSL_LIBRARIES=/usr/local/opt/openssl/lib
    BINARYEN_BIN=/usr/local/binaryen/bin/
    WASM_LLVM_CONFIG=/usr/local/wasm/bin/llvm-config
fi

# Debug flags
COMPILE_EOS=1
COMPILE_CONTRACTS=1

# Define default arguments.
CMAKE_BUILD_TYPE=RelWithDebugInfo

# Install dependencies
if [ ${INSTALL_DEPS} == "1" ]; then

  echo ">> Install dependencies"
  . ${WORK_DIR}/scripts/install_dependencies.sh

fi

# Create the build dir
cd ${WORK_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

CXX_COMPILER=clang++-4.0
C_COMPILER=clang-4.0

if [ $ARCH == "darwin" ]; then
  CXX_COMPILER=clang++
  C_COMPILER=clang
fi

# Build EOS
cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_LLVM_CONFIG=${WASM_LLVM_CONFIG} -DBINARYEN_BIN=${BINARYEN_BIN} -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} ..
make -j10
