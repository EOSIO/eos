#!/bin/bash
brew update
brew install boost, cmake, python@2, python, libtool, libusb, graphviz, automake, wget, gmp, llvm@4, llvm@7, pkgconfig, doxygen
git clone https://git.llvm.org/git/llvm.git --branch release_80 --recursive
cd llvm
mkdir build
cd build
cmake ..
make -j $(getconf _NPROCESSORS_ONLN)
make install
cd ../..
rm -rf llvm