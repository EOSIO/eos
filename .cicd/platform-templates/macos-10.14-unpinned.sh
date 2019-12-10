#!/bin/bash
set -eo pipefail
EOSIO_LOCATION=/Users/anka/eosio
EOSIO_INSTALL_LOCATION=/Users/anka/eosio/install
VERSION=1
# Commands from the documentation are inserted right below this line
# Anything below here is exclusive to our CI/CD
## Cleanup eosio directory (~ 600MB)
rm -rf ${EOSIO_LOCATION}
## Needed for State History Plugin Test
brew install nvm && mkdir ~/.nvm && echo "export NVM_DIR=~/.nvm" >> ~/.bash_profile && echo "source \$(brew --prefix nvm)/nvm.sh" >> ~/.bash_profile && cat ~/.bash_profile && source ~/.bash_profile && echo $NVM_DIR && nvm install --lts=dubnium
## Needed for sha1sum
brew install md5sha1sum