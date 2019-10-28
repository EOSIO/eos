# Installation

There are two options to install `cleos`. You can either install via binaries or build `cleos` from source code. For both options `cleos` is distributed with `eos` repository

Installation via binaries is recommended as it will save your time and avoid encouter build errors

## Installation cleos via binaries

### Mac OS X Brew Install

```bash
brew tap eosio/eosio
brew install eosio
```

### Ubuntu 18.04 Debian Package Install

```bash
wget https://github.com/EOSIO/eos/releases/download/v1.7.0/eosio_1.7.0-1-ubuntu-18.04_amd64.deb
sudo apt install ./eosio_1.7.0-1-ubuntu-18.04_amd64.deb
```

### Ubuntu 16.04 Debian Package Install

```bash
wget https://github.com/EOSIO/eos/releases/download/v1.7.0/eosio_1.7.0-1-ubuntu-16.04_amd64.deb
sudo apt install ./eosio_1.7.0-1-ubuntu-16.04_amd64.deb
```

### CentOS RPM Package Install

```bash
wget https://github.com/EOSIO/eos/releases/download/v1.7.0/eosio-1.7.0-1.el7.x86_64.rpm
sudo yum install ./eosio-1.7.0-1.el7.x86_64.rpm
```

## Build from Source Code

Clone the eos repository and its submodules.

```bash
git clone --recursive https://github.com/eosio/eos
```

Execute the command below:

```bash
cd scripts/
./eosio_build.sh
```