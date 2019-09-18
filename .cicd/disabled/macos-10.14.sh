#!/bin/bash
set -eo pipefail
VERSION=1
brew update
brew install git cmake python@2 python graphviz automake wget llvm@7 pkgconfig doxygen jq || :
pip3 install conan
if [[ ! $PINNED == false || $UNPINNED == true ]]; then
    # install clang from source
    git clone --single-branch --branch release_80 https://git.llvm.org/git/llvm.git clang8
    cd clang8
    git checkout 18e41dc
    cd tools
    git clone --single-branch --branch release_80 https://git.llvm.org/git/lld.git
    cd lld
    git checkout d60a035
    cd ../
    git clone --single-branch --branch release_80 https://git.llvm.org/git/polly.git
    cd polly
    git checkout 1bc06e5
    cd ../
    git clone --single-branch --branch release_80 https://git.llvm.org/git/clang.git clang
    cd clang
    git checkout a03da8b
    cd tools
    mkdir extra
    cd extra
    git clone --single-branch --branch release_80 https://git.llvm.org/git/clang-tools-extra.git
    cd clang-tools-extra
    git checkout 6b34834
    cd ../../../../../projects/
    git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxx.git
    cd libcxx
    git checkout 1853712
    cd ../
    git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxxabi.git
    cd libcxxabi
    git checkout d7338a4
    cd ../
    git clone --single-branch --branch release_80 https://git.llvm.org/git/libunwind.git
    cd libunwind
    git checkout 57f6739
    cd ../
    git clone --single-branch --branch release_80 https://git.llvm.org/git/compiler-rt.git
    cd compiler-rt
    git checkout 5bc7979
    mkdir ../../build
    cd ../../build
    cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX='/usr/local' -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_LIBCXX=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release ..
    make -j $(getconf _NPROCESSORS_ONLN)
    sudo make install
    cd ../..
    rm -rf clang8
fi