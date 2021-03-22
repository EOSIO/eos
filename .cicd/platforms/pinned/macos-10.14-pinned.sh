#!/bin/bash
set -eo pipefail
VERSION=1
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl@1.1 jq libpq postgres || :
# libpqxx 7.3+ installations on mojave try to import libs not present in the sdk. pin to libpqxx 7.2.1 instead.
curl -LO  https://raw.githubusercontent.com/Homebrew/homebrew-core/d14398187084e1d3fd1763ec13cea1044946a51f/Formula/libpqxx.rb
brew install -f ./libpqxx.rb
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
## Boost Fix: eosio/install/bin/../include/c++/v1/stdlib.h:94:15: fatal error: 'stdlib.h' file not found
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
curl -LO https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.bz2
tar -xjf boost_1_72_0.tar.bz2
cd boost_1_72_0
./bootstrap.sh --prefix=/usr/local
sudo SDKROOT="$SDKROOT" ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install
cd ..
sudo rm -rf boost_1_72_0.tar.bz2 boost_1_72_0
# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
# initialize postgres configuration files
sudo rm -rf /usr/local/var/postgres
initdb --locale=C -E UTF-8 /usr/local/var/postgres
