#!/bin/bash
set -eo pipefail
VERSION=1
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl jq rabbitmq postgres || :
# install request and requests_unixsocket module
pip3 install requests requests_unixsocket
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
curl -LO https://boostorg.jfrog.io/artifactory/main/release/1.72.0/source/boost_1_72_0.tar.bz2
tar -xjf boost_1_72_0.tar.bz2
cd boost_1_72_0
# apply patch to fix CVE-2016-9840
BEAST_FIX_URL=https://raw.githubusercontent.com/boostorg/beast/3fd090af3b7e69ed7871c64a4b4b86fae45e98da/include/boost/beast/zlib/detail/inflate_stream.ipp
curl -Lo boost/beast/zlib/detail/inflate_stream.ipp ${BEAST_FIX_URL}
./bootstrap.sh --prefix=/usr/local
sudo -E ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install
cd ..
sudo rm -rf boost_1_72_0.tar.bz2 boost_1_72_0

# install libpqxx from source
curl -L https://github.com/jtv/libpqxx/archive/7.2.1.tar.gz | tar zxvf - 
cd  libpqxx-7.2.1  
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DPostgreSQL_ROOT=/usr/local/opt/libpq  -DSKIP_BUILD_TEST=ON -DCMAKE_BUILD_TYPE=Release -S . -B build 
cmake --build build && cmake --install build 
cd .. && rm -rf libpqxx-7.2.1

# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
# add sbin to path from rabbitmq-server
echo "export PATH=$PATH:/usr/local/sbin" >> ~/.bash_profile
# initialize postgres configuration files
sudo rm -rf /usr/local/var/postgres
initdb --locale=C -E UTF-8 /usr/local/var/postgres
