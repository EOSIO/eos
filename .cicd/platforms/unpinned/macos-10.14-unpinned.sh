#!/bin/bash
set -eo pipefail
VERSION=1
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl@1.1 jq boost libpq postgres || :
# libpqxx 7.3+ installations on mojave try to import libs not present in the sdk. pin to libpqxx 7.2.1 instead.
curl -LO  https://raw.githubusercontent.com/Homebrew/homebrew-core/d14398187084e1d3fd1763ec13cea1044946a51f/Formula/libpqxx.rb
brew install -f ./libpqxx.rb
# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
# initialize postgres configuration files
sudo rm -rf /usr/local/var/postgres
initdb --locale=C -E UTF-8 /usr/local/var/postgres
