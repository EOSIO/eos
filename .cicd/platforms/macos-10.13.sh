#!/bin/bash
set -eo pipefail
VERSION=1
brew update
brew install git cmake python@2 python libtool libusb graphviz automake wget gmp llvm@4 pkgconfig doxygen openssl || true
# install boost
cd ~
curl -LO https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2
tar -xjf boost_1_67_0.tar.bz2
cd boost_1_67_0
./bootstrap.sh --prefix=/Users/anka/boost_1_67_0
./b2 -q -j $(getconf _NPROCESSORS_ONLN) --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test install
cd ..
rm -f boost_1_67_0.tar.bz2
mkdir -p ~/opt
ln -s ~/boost_1_67_0 ~/opt/boost
# symlink llvm
ln -s /usr/local/opt/llvm@4 ~/opt/llvm
# install mongoDB
cd ~
curl -OL https://fastdl.mongodb.org/osx/mongodb-osx-ssl-x86_64-3.6.3.tgz
tar -xzf mongodb-osx-ssl-x86_64-3.6.3.tgz
rm -f mongodb-osx-ssl-x86_64-3.6.3.tgz
ln -s ~/mongodb-osx-x86_64-3.6.3 ~/mongodb
# install mongo-c-driver from source
cd /tmp
curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz
tar -xzf mongo-c-driver-1.13.0.tar.gz
cd mongo-c-driver-1.13.0
mkdir -p cmake-build
cd cmake-build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX='/usr/local' -DENABLE_BSON=ON -DENABLE_SSL=DARWIN -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SASL=OFF -DENABLE_SNAPPY=OFF ..
make -j $(getconf _NPROCESSORS_ONLN)
sudo make install
cd ../..
rm mongo-c-driver-1.13.0.tar.gz
# install mongo-cxx-driver from source
cd /tmp
curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz -o mongo-cxx-driver-r3.4.0.tar.gz
tar -xzf mongo-cxx-driver-r3.4.0.tar.gz
cd mongo-cxx-driver-r3.4.0/build
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX='/usr/local' ..
make -j $(getconf _NPROCESSORS_ONLN) VERBOSE=1
sudo make install
cd ../..
rm -f mongo-cxx-driver-r3.4.0.tar.gz