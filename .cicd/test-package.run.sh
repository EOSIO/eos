#!/bin/bash
set -euo pipefail

. "${0%/*}/libfunctions.sh"

echo '+++ :minidisc: Installing EOSIO'

if [[ $(apt-get --version 2>/dev/null) ]]; then # debian family packaging
    perform 'apt-get update'
    perform 'apt-get install -y /eos/*.deb'
elif [[ $(yum --version 2>/dev/null) ]]; then # RHEL family packaging
    perform 'yum check-update || :'
    perform 'yum install -y /eos/*.rpm'
elif [[ $(brew --version 2>/dev/null) ]]; then # homebrew packaging
    perform 'brew update'
    perform 'mkdir homebrew-eosio'
    perform 'git init homebrew-eosio'
    perform 'cp *.rb homebrew-eosio'
    perform "sed -i.bk -e 's/url \".*\"/url \"http:\/\/127.0.0.1:7800\"/' homebrew-eosio/*.rb"
    perform "pushd homebrew-eosio && git add *.rb && git commit -m 'test it!' && popd"
    perform "brew tap eosio/eosio homebrew-eosio"
    perform '{ python3 -m http.server 7800 & } && export HTTP_SERVER_PID=$!'
    perform 'sleep 20s'
    perform 'brew install eosio'
    perform 'kill $HTTP_SERVER_PID'
else
    echo 'ERROR: Package manager not detected!'
    exit 3
fi

nodeos --full-version
