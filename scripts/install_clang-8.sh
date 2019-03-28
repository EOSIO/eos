#! /bin/bash

mkdir $TEMP_DIR/clang8
pushd $TEMP_DIR/clang8 &> /dev/null

wget https://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz

tar -xvzf clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz

mkdir $OPT_LOCATION/clang-8
mv clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04 $OPT_LOCATION/clang-8
