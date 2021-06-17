#!/bin/bash
set -eo pipefail
VERSION=1
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl jq  || :
pip3 install requests
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
./bootstrap.sh --prefix=/usr/local
sudo -E ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install
cd ..
sudo rm -rf boost_1_72_0.tar.bz2 boost_1_72_0

# build libqp and postgres
curl -L https://github.com/postgres/postgres/archive/refs/tags/REL_13_3.tar.gz | tar zxvf -
cd postgres-REL_13_3
./configure && make && sudo make install
cd .. && rm -rf postgres-REL_13_3

export PostgreSQL_ROOT=/usr/local/pgsql
export PKG_CONFIG_PATH=/usr/local/pgsql/lib/pkgconfig:/usr/local/lib64/pkgconfig
export PATH="/usr/local/pgsql/bin:${PATH}"
#build libpqxx from source
curl -L https://github.com/jtv/libpqxx/archive/7.2.1.tar.gz | tar zxvf -
cd  libpqxx-7.2.1
cmake -DCMAKE_INSTALL_PREFIX=/usr/local  -DSKIP_BUILD_TEST=ON  -DPostgreSQL_INCLUDE_DIR=/usr/local/pgsql/include  -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/local/pgsql/include  -DPostgreSQL_LIBRARY_DIR=/usr/local/pgsql/lib  -DPostgreSQL_LIBRARY=libpq.a   -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build && cmake --install build
cd .. && rm -rf libpqxx-7.2.1

# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
# initialize postgres configuration files
sudo rm -rf /usr/local/var/postgres
initdb --locale=C -E UTF-8 /usr/local/var/postgres
export PGDATA=/usr/local/var/postgres
