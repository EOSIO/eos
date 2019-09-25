#!/bin/bash
set -eo pipefail

apt-get update && apt-get upgrade -y
apt-get install -y python3 python3-dev python3-pip clang llvm-7-dev git curl automake
pip3 install conan
curl -LO https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh
chmod +x cmake-3.15.3-Linux-x86_64.sh 
mkdir /cmake && ./cmake-3.15.3-Linux-x86_64.sh --skip-license --prefix=/cmake
export PATH=$PATH:/cmake/bin
cd /workdir/build
cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang' -DLLVM_DIR='/usr/lib/llvm-7/lib/cmake/llvm' -DUSE_CONAN=true ..
ctest -L nonparallelizable_tests --output-on-failure -T Test