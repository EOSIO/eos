#!/bin/bash
set -eo pipefail
VERSION=1
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl jq || :
# install clang from source
git clone --single-branch --branch llvmorg-10.0.0 https://github.com/llvm/llvm-project clang10
mkdir clang10/build
cd clang10/build
cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX='/usr/local' -DLLVM_ENABLE_PROJECTS='lld;polly;clang;clang-tools-extra;libcxx;libcxxabi;libunwind;compiler-rt' -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_TARGETS_TO_BUILD=host -DCMAKE_BUILD_TYPE=Release ../llvm && \
make -j $(getconf _NPROCESSORS_ONLN)
sudo make install
cd ../..
rm -rf clang10
# install boost from source
curl -LO https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.bz2
tar -xjf boost_1_72_0.tar.bz2
cd boost_1_72_0
./bootstrap.sh --prefix=/usr/local
sudo -E ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install
cd ..
sudo rm -rf boost_1_72_0.tar.bz2 boost_1_72_0
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
# install pip dependencies.
cd ~ && sudo python3 -m pip install --upgrade pip
sudo python3 -m pip install requests
# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
