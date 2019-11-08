#!/bin/sh

set -e -v

$CXX --version

if [[ $TRAVIS_OS_NAME == "osx" ]]; then
  export CMAKE_URL="https://cmake.org/files/v3.7/cmake-3.7.2-Darwin-x86_64.tar.gz";
  export LLVM_URL="https://releases.llvm.org/4.0.0/clang+llvm-4.0.0-x86_64-apple-darwin.tar.xz";
else
  export CMAKE_URL="https://cmake.org/files/v3.7/cmake-3.7.2-Linux-x86_64.tar.gz";
  export LLVM_URL="https://releases.llvm.org/4.0.0/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz";
fi


# Download a newer version of cmake than is available in Travis's whitelisted apt sources.
mkdir cmake
cd cmake
wget --quiet -O ./cmake.tar.gz ${CMAKE_URL}
tar --strip-components=1 -xzf ./cmake.tar.gz
export PATH=`pwd`/bin:${PATH}
cd ..
cmake --version

# Download a binary build of LLVM4 (also not available in Travis's whitelisted apt sources)
mkdir llvm4
cd llvm4
wget --quiet -O ./llvm.tar.xz ${LLVM_URL}
tar --strip-components=1 -xf ./llvm.tar.xz
export LLVM_DIR=`pwd`/lib/cmake/llvm
cd ..

# Build and test a release build of WAVM.
mkdir release
cd release
cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DLLVM_DIR=${LLVM_DIR}
make
ctest -V
cd ..

# Build and test a debug build of WAVM.
mkdir debug
cd debug
cmake .. -DCMAKE_BUILD_TYPE=DEBUG -DLLVM_DIR=${LLVM_DIR}
make
ASAN_OPTIONS=detect_leaks=0 ctest -V
cd ..
