#!/bin/bash
set -eo pipefail
VERSION=1
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl jq boost libpq libpqxx postgres rabbitmq || :
# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
# add sbin to path from rabbitmq-server
echo "export PATH=$PATH:/usr/local/sbin" >> ~/.bash_profile
# initialize postgres configuration files
initdb --locale=C -E UTF-8 /usr/local/var/postgres
