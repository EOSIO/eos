#!/bin/bash
set -eo pipefail
VERSION=1
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl jq   || :
pip3 install requests

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

# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
# initialize postgres configuration files
sudo rm -rf /usr/local/var/postgres
initdb --locale=C -E UTF-8 /usr/local/var/postgres
export PGDATA=/usr/local/var/postgres
