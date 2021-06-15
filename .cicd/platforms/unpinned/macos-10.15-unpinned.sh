#!/bin/bash
set -eo pipefail
VERSION=1
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl jq boost  || :
pip3 install requests

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
cmake -DCMAKE_INSTALL_PREFIX=/usr/local  -DSKIP_BUILD_TEST=ON  -DPostgreSQL_INCLUDE_DIR=/usr/local/pgsql/include  -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/local/pgsql/include  -DPostgreSQL_LIBRARY_DIR=/usr/local/pgsql/lib  -DPostgreSQL_LIBRARY=libpq.a   -DCMAKE_BUILD    _TYPE=Release -S . -B build
cmake --build build && cmake --install build
cd .. && rm -rf libpqxx-7.2.1

# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
# initialize postgres configuration files
sudo useradd -m postgres && sudo mkdir /usr/local/pgsql/data && sudo chown postgres:postgres /usr/local/pgsql/data &&  su - postgres -c "/usr/local/pgsql/bin/initdb -D /usr/local/pgsql/data/"
export PGDATA=/usr/local/pgsql/data
