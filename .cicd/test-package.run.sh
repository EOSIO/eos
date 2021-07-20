#!/bin/bash
set -euo pipefail

function perform {
    echo "$ $1"
    eval $1
}

echo '+++ :minidisc: Installing EOSIO'

if [[ $(apt-get --version 2>/dev/null) ]]; then # debian family
    perform 'apt-get update'
    perform 'apt-get install -y /eos/*.deb'
elif [[ $(yum --version 2>/dev/null) ]]; then # RHEL family
    perform 'yum check-update || :'
    perform 'yum install -y /eos/*.rpm'
elif [[ "$OSTYPE" == "darwin"* ]]; then
    if [[ ! $(brew --version 2>/dev/null) ]]; then # macos
        perform 'echo | /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"'
        perform 'brew analytics off'
    fi
    perform 'brew update'
    perform 'mkdir homebrew-eosio'
    perform 'git init homebrew-eosio'
    perform 'cp *.rb homebrew-eosio'
    perform "sed -i.bk -e 's/url \".*\"/url \"http:\/\/127.0.0.1:7800\"/' homebrew-eosio/*.rb"
    perform "pushd homebrew-eosio && git add *.rb && git commit -m 'test it!' && popd"
    perform "brew tap eosio/eosio homebrew-eosio"
    perform 'python3 -m http.server 7800 &'
    perform 'sleep 20s'
    perform 'brew install eosio'
else
    echo 'ERROR: Package manager not detected!'
    exit 3
fi

nodeos --full-version
