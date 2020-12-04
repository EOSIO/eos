#!/bin/bash
set -eo pipefail
VERSION=1
brew update
brew install git cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl@1.1 jq boost libpq libpqxx || :
# install nvm for ship_test
cd ~ && brew install nvm && mkdir -p ~/.nvm && echo "export NVM_DIR=$HOME/.nvm" >> ~/.bash_profile && echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium