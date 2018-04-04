## EOS.IO - The Most Powerful Infrastructure for Decentralized Applications

[![Build Status](https://jenkins.eos.io/buildStatus/icon?job=eosio/master)](https://jenkins.eos.io/job/eosio/job/master/)

Welcome to the EOS.IO source code repository!  EOS.IO software enables developers to create and deploy high-performance, horizontally scalable, blockchain infrastructure upon which decentralized applications can be built.

This code is currently alpha-quality and under rapid development. That said, there is plenty early experimenters can do. This includes running a private multi-node test network and developing applications (smart contracts).

The public testnet described in the [wiki](https://github.com/EOSIO/eos/wiki/Testnet%3A%20Public) is running the `dawn-2.x` branch.  The `master` branch is no longer compatible with the public testnet.  Instructions are provided below for building a local testnet using the `master` branch.  This document will be updated later with instructions for running on the `dawn-3.x` public testnet.

### Supported Operating Systems
EOS.IO currently supports the following operating systems:  
1. Amazon 2017.09 and higher.  
2. Centos 7.  
3. Fedora 25 and higher (Fedora 27 recommended).  
4. Mint 18.  
5. Ubuntu 16.04 (Ubuntu 16.10 recommended).  
6. MacOS Darwin 10.12 and higher (MacOS 10.13.x recommended).  

# Resources
1. [EOS.IO Website](https://eos.io)
2. [Documentation](https://eosio.github.io/eos/)
3. [Blog](https://steemit.com/@eosio)
4. [Community Telegram Group](https://t.me/EOSProject)
5. [Developer Telegram Group](https://t.me/joinchat/EaEnSUPktgfoI-XPfMYtcQ)
6. [White Paper](https://github.com/EOSIO/Documentation/blob/master/TechnicalWhitePaper.md)
7. [Roadmap](https://github.com/EOSIO/Documentation/blob/master/Roadmap.md)
8. [Wiki](https://github.com/EOSIO/eos/wiki)

# Table of contents

1. [Getting Started](#gettingstarted)
2. [Setting up a build/development environment](#setup)
   1. [Getting the code](#getcode)
   2. [Building the executables](#build)
      1. [Automated build script](#autobuild)
      2. [Manual build](#manualbuild)
   3. [Build validation](#basicvalidation)
   4. [Install the executables](#installexecutables)
3. [Running a node](#runanode)
   1. [Creating and launching a single-node testnet](#singlenode)
   2. [Running a multi-node local testnet](#localtestnet)
   3. [Running on the public testnet](#publictestnet)
   4. [Next steps](#nextsteps)
4. [Example Currency Contract Walkthrough](#smartcontracts)
   1. [Example Contracts](#smartcontractexample)
   2. [Setting up a wallet and importing account key](#walletimport)
   3. [Creating accounts for your smart contracts](#createaccounts)
   4. [Upload sample contract to blockchain](#uploadsmartcontract)
   5. [Pushing a message to a sample contract](#pushamessage)
   6. [Reading Currency Contract Balance](#readingcontract)
5. [Doxygen documentation](#doxygen)
6. [Running EOS in Docker](#docker)
7. [Manual installation of the dependencies](#manualdep)
   1. [Clean install Amazon 2017.09 and higher](#manualdepamazon)
   2. [Clean install Centos 7 and higher](#manualdepcentos)
   3. [Clean install Fedora 25 and higher](#manualdepfedora)
   4. [Clean install Mint 18](#manualdepubuntu)
   5. [Clean install Ubuntu 16](#manualdepubuntu)
   6. [Clean install MacOS Sierra 10.12 and higher](#manualdepmacos)

<a name="gettingstarted"></a>
## Getting Started
The following instructions detail the process of getting the software, building it, running a simple test network that produces blocks, account creation and uploading a sample contract to the blockchain.

<a name="setup"></a>
## Setting up a build/development environment

<a name="getcode"></a>
### Getting the code

To download all of the code, clone the eos repository and its submodules.

```bash
git clone https://github.com/eosio/eos --recursive
```

If a repo is cloned without the `--recursive` flag, the submodules can be retrieved after the fact by running this command from within the repo:

```bash
git submodule update --init --recursive
```

<a name="build"></a>
### Building the executables

EOS comes with a number of programs:

* nodeos - server-side blockchain node component
* cleos - command line interface to interact with the blockchain
* keosd - EOS wallet
* eosio-launcher - application for nodes network composing and deployment; [more on eosio-launcher](https://github.com/EOSIO/eos/blob/master/testnet.md)

The build places content in the `eos/build` folder.  The executables can be found in subfolders within the `eos/build/programs` folder.

<a name="autobuild"></a>
#### Automated build script

There is an automated build script that can install all dependencies and build EOS.  We are working on supporting other Linux/Unix distributions in future releases.

Run the build script.

```bash
cd eos
./eosio_build.sh
```

<a name="manualbuild"></a>
#### Manual build

To manually build, use the following steps to create a `build` folder within your `eos` folder and then perform the build.  The steps below assume the `eos` repository was cloned into your home (i.e., `~`) folder.

```bash
cd ~
mkdir -p ~/eos/build && cd ~/eos/build
cmake -DBINARYEN_BIN=~/binaryen/bin -DWASM_ROOT=~/wasm-compiler/llvm -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib ..
make -j$( nproc )
```

Out-of-source builds are also supported. To override clang's default choice in compiler, add these flags to the CMake command:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.

To run the test suite after building, run the `chain_test` executable in the `tests` folder.

<a name="basicvalidation"></a>
### Build validation 

Optionally, a set of tests can be run against your build to perform some basic validation.

```bash
cd build
make test
```

<a name="installexecutables"></a>
### Install the executables 
 
For ease of contract development, content can be installed in the `/usr/local` folder using the `make install` target.  This step is run from the `build` folder.  Adequate permission is required to install.

```bash
cd build
sudo make install
```

<a name="runanode"></a>
## Running a node

<a name="singlenode"></a>
### Running a single-node testnet

After successfully building the project, the `nodeos` binary should be present in the `build/programs/nodeos` folder.  `nodeos` can be run directly from the `build` folder using `programs/nodeos/nodeos`.

`nodeos` uses a custom configuration folder.  The location of this folder is determined by your system.

- Mac OS: `~/Library/Application Support/eosio/nodeos/config`
- Linux: `~/.local/share/eosio/nodeos/config`

The build seeds this folder with a default `genesis.json` file.  Alternatively, a configuration folder can be specified using the `--config-dir` command line argument to `nodeos`.  If you use this option, you will need to manually copy a genesis.json file to your config folder.

`nodeos` will need a properly configured `config.ini` file in order to do meaningful work.  On startup, `nodeos` looks in the config folder for `config.ini`.  If one is not found, a default `config.ini` file is created.  If you do not
already have a `config.ini` file ready to use, run `nodeos` and then close it immediately with <kbd>Ctrl-C</kbd>.  A default configuration (`config.ini`) will have been created in the config folder.  Edit the `config.ini` file, adding/updating the following settings to the defaults already in place:

```
# Load the testnet genesis state, which creates some initial block producers with the default key
genesis-json = /path/to/eos/source/genesis.json
# Enable production on a stale chain, since a single-node test chain is pretty much always stale
enable-stale-production = true
# Enable block production with the testnet producers
producer-name = eosio
# Load the block producer plugin, so you can produce blocks
plugin = eosio::producer_plugin
# Wallet plugin
plugin = eosio::wallet_api_plugin
# As well as API and HTTP plugins
plugin = eosio::chain_api_plugin
plugin = eosio::http_plugin
```

Now it should be possible to run `nodeos` and see it begin producing blocks.

When running `nodeos` you should get log messages similar to below. It means the blocks are successfully produced.

```
1575001ms thread-0   chain_controller.cpp:235      _push_block          ] initm #1 @2017-09-04T04:26:15  | 0 trx, 0 pending, exectime_ms=0
1575001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initm generated block #1 @ 2017-09-04T04:26:15 with 0 trxs  0 pending
1578001ms thread-0   chain_controller.cpp:235      _push_block          ] initc #2 @2017-09-04T04:26:18  | 0 trx, 0 pending, exectime_ms=0
1578001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initc generated block #2 @ 2017-09-04T04:26:18 with 0 trxs  0 pending
...
```

`nodeos` stores runtime data (e.g., shared memory and log content) in a custom data folder.  The location of this folder is determined by your system.

- Mac OS: `~/Library/Application Support/eosio/nodeos/data`
- Linux: `~/.local/share/eosio/nodeos/data`

A data folder can be specified using the `--data-dir` command line argument to `nodeos`.

<a name="localtestnet"></a>
### Running a multi-node local testnet

To run a local testnet you can use the `eosio-launcher` application provided in the `~/eos/build/programs/eosio-launcher` folder.

For testing purposes you will run two local production nodes talking to each other.

```bash
cd ~/eos/build
cp ../genesis.json ./
./programs/eosio-launcher/eosio-launcher -p2 --skip-signature
```

This command will generate two data folders for each instance of the node: `tn_data_00` and `tn_data_01`.

You should see the following response:

```bash
spawning child, programs/nodeos/nodeos --skip-transaction-signatures --data-dir tn_data_0
spawning child, programs/nodeos/nodeos --skip-transaction-signatures --data-dir tn_data_1
```

To confirm the nodes are running, run the following `cleos` commands:
```bash
~/eos/build/programs/cleos
./cleos -p 8888 get info
./cleos -p 8889 get info
```

For each command, you should get a JSON response with blockchain information.

You can read more on eosio-launcher and its settings [here](https://github.com/EOSIO/eos/blob/master/testnet.md)

<a name="publictestnet"></a>
### Running a local node connected to the public testnet

The `master` branch is no longer compatible with the `dawn-2.x` public testnet.  To run on the public testnet, please see [dawn-2.x/eos/README.md](https://github.com/EOSIO/eos/blob/dawn-2.x/README.md)

## Next Steps
<a name="nextsteps"></a>

Further documentation is available in the [wiki](https://github.com/EOSIO/eos/wiki). Wiki pages include detailed reference documentation for all programs and tools and the database schema and API. The wiki also includes a section describing smart contract development. A simple walkthrough of the "currency" contract follows.

<a name="smartcontracts"></a>
## Example "Currency" Contract Walkthrough

EOS comes with example contracts that can be uploaded and run for testing purposes. Next we demonstrate how to upload and interact with the sample contract "currency".

<a name="smartcontractexample"></a>
### Example smart contracts

First, run the node

```bash
cd ~/eos/build/programs/nodeos/
./nodeos
```

<a name="walletimport"></a>
### Setting up a wallet

Every contract requires an associated account, so first you need to create a wallet.
To create a wallet, you need to have the wallet_plugin loaded into the nodeos process which can be accomplished in one of two ways:
* Via a plugin entry in the config.ini file (i.e. `plugin = eosio::wallet_plugin`)
* Via a plugin command-line option when invoking nodeos (i.e. `--plugin eosio::wallet_plugin`)

Use the wallet create command of the cleos tool, to create a wallet.

```bash
cd ~/eos/build/programs/cleos/
./cleos wallet create # Outputs a password that you need to save to be able to lock/unlock the wallet
```

<a name="createaccounts"></a>
### Creating accounts for sample "currency" contract

First, generate some public/private key pairs that will be later assigned as the `owner_key` and the `active_key`.

```bash
cd ~/eos/build/programs/cleos/
./cleos create key # owner-key
./cleos create key # active-key
```

This will output two pairs of public and private keys

```
Private key: XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Public key: EOSXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

**Important:**
Save the values for future reference.

Run the `cleos create account` command where `eosio` is the account authorizing the creation of the `currency` account and `public-owner-key` and `public-active-key` are the values generated above by the two invocations of the `cleos create key` command

```bash
./cleos create account eosio currency public-owner-key public-active-key
```

You should then get a JSON response back with a transaction ID confirming it was executed successfully.

You can verify that the account was successfully created

```bash
./cleos get account currency
```

If all has gone well, you will receive an output similar to the following:

```json
{
  "account_name": "currency",
  "eos_balance": "0.0000 EOS",
  "staked_balance": "0.0001 EOS",
  "unstaking_balance": "0.0000 EOS",
  "last_unstaking_time": "2035-10-29T06:32:22",
   ...
}
```

Now import the active private key generated previously in the wallet:

```bash
./cleos wallet import XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

<a name="uploadsmartcontract"></a>
### Upload sample "currency" contract to blockchain

Before uploading a contract, verify that there is no current contract:

```bash
./cleos get code currency
code hash: 0000000000000000000000000000000000000000000000000000000000000000
```

With an account for a contract created, upload a sample contract:

```bash
./cleos set contract currency ../../contracts/currency/currency.wast ../../contracts/currency/currency.abi
```

As a response you should get a JSON with a `transaction_id` field. Your contract was successfully uploaded!

You can also verify that the code has been set with the following command:

```bash
./cleos get code currency
```

It will return something like:
```bash
code hash: 9b9db1a7940503a88535517049e64467a6e8f4e9e03af15e9968ec89dd794975
```

Before using the currency contract, you must issue the currency.

```bash
./cleos push action currency issue '{"to":"currency","quantity":"1000.0000 CUR","memo":""}' --permission currency@active
```

Next verify the currency contract has the proper initial balance:

```bash
./cleos get table currency currency account
{
  "rows": [{
     "currency": 1381319428,
     "balance": 10000000
     }
  ],
  "more": false
}
```

<a name="pushamessage"></a>
### Transfering funds with the sample "currency" contract

Anyone can send any message to any contract at any time, but the contracts may reject messages which are not given necessary permission. Messages are not sent "from" anyone, they are sent "with permission of" one or more accounts and permission levels. The following commands show a "transfer" message being sent to the "currency" contract.

The content of the message is `'{"from":"currency","to":"eosio","quantity":"20.0000 CUR","memo":"any string"}'`. In this case we are asking the currency contract to transfer funds from itself to someone else. This requires the permission of the currency contract.

```bash
./cleos push action currency transfer '{"from":"currency","to":"eosio","quantity":"20.0000 CUR","memo":"my first transfer"}' --permission currency@active
```

Below is a generalization that shows the `currency` account is only referenced once, to specify which contract to deliver the `transfer` message to.

```bash
./cleos push action currency transfer '{"from":"${usera}","to":"${userb}","quantity":"20.0000 CUR","memo":""}' --permission ${usera}@active
```

As confirmation of a successfully submitted transaction, you will receive JSON output that includes a `transaction_id` field.

<a name="readingcontract"></a>
### Reading sample "currency" contract balance

So now check the state of both of the accounts involved in the previous transaction.

```bash
./cleos get table eosio currency account
{
  "rows": [{
      "currency": 1381319428,
      "balance": 200000
       }
    ],
  "more": false
}
./cleos get table currency currency account
{
  "rows": [{
      "currency": 1381319428,
      "balance": 9800000
    }
  ],
  "more": false
}
```

As expected, the receiving account **eosio** now has a balance of **20** tokens, and the sending account now has **20** less tokens than its initial supply.

<a name="doxygen"></a>
## Doxygen documentation

You can find more detailed API documentation in the Doxygen reference. 
 
For the `master` branch: https://eosio.github.io/eos/  
For the public testnet branch: http://htmlpreview.github.io/?https://github.com/EOSIO/eos/blob/dawn-2.x/docs/index.html  

<a name="docker"></a>
## Running EOS in Docker

You can find up to date information about EOS Docker in the [Docker Readme](https://github.com/EOSIO/eos/blob/master/Docker/README.md)

<a name="manualdep"></a>
## Manual installation of the dependencies

If you prefer to manually build dependencies, follow the steps below.

This project is written primarily in C++14 and uses CMake as its build system. An up-to-date Clang and the latest version of CMake is recommended.

Dependencies:

* Clang 4.0.0
* CMake 3.5.1
* Boost 1.66
* OpenSSL
* LLVM 4.0
* [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git)

<a name="manualdepamazon"></a>
### Clean install Amazon 2017.09 and higher

Install the development toolkit:

```bash
sudo yum update
sudo yum install git gcc72.x86_64 gcc72-c++.x86_64 autoconf automake libtool make bzip2 \
				 bzip2-devel.x86_64 openssl-devel.x86_64 gmp.x86_64 gmp-devel.x86_64 \
				 libstdc++72.x86_64 python36-devel.x86_64 libedit-devel.x86_64 \
				 ncurses-devel.x86_64 swig.x86_64 gettext-devel.x86_64

```

Install CMake 3.10.2:

```bash
cd ~
curl -L -O https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz
tar xf cmake-3.10.2.tar.gz
rm -f cmake-3.10.2.tar.gz
ln -s cmake-3.10.2/ cmake
cd cmake
./bootstrap
make
sudo make install
```

Install Boost 1.66:

```bash
cd ~
curl -L https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2 > boost_1.66.0.tar.bz2
tar xf boost_1.66.0.tar.bz2
echo "export BOOST_ROOT=$HOME/boost_1_66_0" >> ~/.bash_profile
source ~/.bash_profile
cd boost_1_66_0/
./bootstrap.sh "--prefix=$BOOST_ROOT"
./b2 install
```
Install [MongoDB (mongodb.org)](https://www.mongodb.com):

```bash
mkdir ${HOME}/opt
cd ${HOME}/opt
curl -OL https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-amazon-3.6.3.tgz
tar xf mongodb-linux-x86_64-amazon-3.6.3.tgz
rm -f mongodb-linux-x86_64-amazon-3.6.3.tgz
ln -s ${HOME}/opt/mongodb-linux-x86_64-amazon-3.6.3/ ${HOME}/opt/mongodb
mkdir ${HOME}/opt/mongodb/data
mkdir ${HOME}/opt/mongodb/log
touch ${HOME}/opt/mongodb/log/mongod.log
		
tee > /dev/null ${HOME}/opt/mongodb/mongod.conf <<mongodconf
systemLog:
 destination: file
 path: ${HOME}/opt/mongodb/log/mongod.log
 logAppend: true
 logRotate: reopen
net:
 bindIp: 127.0.0.1,::1
 ipv6: true
storage:
 dbPath: ${HOME}/opt/mongodb/data
mongodconf

export PATH=${HOME}/opt/mongodb/bin:$PATH
mongod -f ${HOME}/opt/mongodb/mongod.conf
```
Install [mongo-cxx-driver (release/stable)](https://github.com/mongodb/mongo-cxx-driver):

```bash
cd ~
curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz
tar xf mongo-c-driver-1.9.3.tar.gz
cd mongo-c-driver-1.9.3
./configure --enable-static --enable-ssl=openssl --disable-automatic-init-and-cleanup --prefix=/usr/local
make -j$( nproc )
sudo make install
git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
cd mongo-cxx-driver/build
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
sudo make -j$( nproc )
```

Install [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git):

```bash
cd ~
git clone https://github.com/cryptonomex/secp256k1-zkp.git
cd secp256k1-zkp
./autogen.sh
./configure
make -j$( nproc )
sudo make install
```

By default LLVM and clang do not include the WASM build target, so you will have to build it yourself:

```bash
mkdir  ~/wasm-compiler
cd ~/wasm-compiler
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
cd llvm/tools
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
cd ..
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=.. -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
make -j$( nproc ) 
make install
```

Your environment is set up. Now you can <a href="#runanode">build EOS and run a node</a>.

<a name="manualdepcentos"></a>
### Clean install Centos 7 and higher

Install the development toolkit:
* Installation on Centos requires installing/enabling the Centos Software Collections
  Repository.
  [Centos SCL](https://wiki.centos.org/AdditionalResources/Repositories/SCL):

```bash
sudo yum --enablerepo=extras install centos-release-scl
sudo yum update
sudo yum install -y devtoolset-7
scl enable devtoolset-7 bash
sudo yum install -y python33.x86_64
scl enable python33 bash
sudo yum install git autoconf automake libtool make bzip2 \
				 bzip2-devel.x86_64 openssl-devel.x86_64 gmp-devel.x86_64 \
				 ocaml.x86_64 doxygen libicu-devel.x86_64 python-devel.x86_64 \
				 gettext-devel.x86_64

```

Install CMake 3.10.2:

```bash
cd ~
curl -L -O https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz
tar xf cmake-3.10.2.tar.gz
cd cmake-3.10.2
./bootstrap
make -j$( nproc )
sudo make install
```

Install Boost 1.66:

```bash
cd ~
curl -L https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2 > boost_1.66.0.tar.bz2
tar xf boost_1.66.0.tar.bz2
echo "export BOOST_ROOT=$HOME/boost_1_66_0" >> ~/.bash_profile
source ~/.bash_profile
cd boost_1_66_0/
./bootstrap.sh "--prefix=$BOOST_ROOT"
./b2 install
```

Install [MongoDB (mongodb.org)](https://www.mongodb.com):

```bash
mkdir ${HOME}/opt
cd ${HOME}/opt
curl -OL https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-amazon-3.6.3.tgz
tar xf mongodb-linux-x86_64-amazon-3.6.3.tgz
rm -f mongodb-linux-x86_64-amazon-3.6.3.tgz
ln -s ${HOME}/opt/mongodb-linux-x86_64-amazon-3.6.3/ ${HOME}/opt/mongodb
mkdir ${HOME}/opt/mongodb/data
mkdir ${HOME}/opt/mongodb/log
touch ${HOME}/opt/mongodb/log/mongod.log
		
tee > /dev/null ${HOME}/opt/mongodb/mongod.conf <<mongodconf
systemLog:
 destination: file
 path: ${HOME}/opt/mongodb/log/mongod.log
 logAppend: true
 logRotate: reopen
net:
 bindIp: 127.0.0.1,::1
 ipv6: true
storage:
 dbPath: ${HOME}/opt/mongodb/data
mongodconf

export PATH=${HOME}/opt/mongodb/bin:$PATH
mongod -f ${HOME}/opt/mongodb/mongod.conf
```

Install [mongo-cxx-driver (release/stable)](https://github.com/mongodb/mongo-cxx-driver):

```bash
cd ~
curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz
tar xf mongo-c-driver-1.9.3.tar.gz
cd mongo-c-driver-1.9.3
./configure --enable-static --enable-ssl=openssl --disable-automatic-init-and-cleanup --prefix=/usr/local
make -j$( nproc )
sudo make install
git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
cd mongo-cxx-driver/build
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
sudo make -j$( nproc )
```

Install [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git):

```bash
cd ~
git clone https://github.com/cryptonomex/secp256k1-zkp.git
cd secp256k1-zkp
./autogen.sh
./configure
make -j$( nproc )
sudo make install
```

By default LLVM and clang do not include the WASM build target, so you will have to build it yourself:

```bash
mkdir  ~/wasm-compiler
cd ~/wasm-compiler
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
cd llvm/tools
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
cd ..
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=.. -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly 
-DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release ../
make -j$( nproc ) 
make install
```

Your environment is set up. Now you can <a href="#runanode">build EOS and run a node</a>.

<a name="manualdepfedora"></a>
### Clean install Fedora 25 and higher

Install the development toolkit:

```bash
sudo yum update
sudo yum install git gcc.x86_64 gcc-c++.x86_64 autoconf automake libtool make cmake.x86_64 \
				 bzip2-devel.x86_64 openssl-devel.x86_64 gmp-devel.x86_64 \
				 libstdc++-devel.x86_64 python3-devel.x86_64 libedit.x86_64 \
				 mongodb.x86_64 mongodb-server.x86_64 ncurses-devel.x86_64 \
				 swig.x86_64 gettext-devel.x86_64

```

Install Boost 1.66:

```bash
cd ~
curl -L https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2 > boost_1.66.0.tar.bz2
tar xf boost_1.66.0.tar.bz2
echo "export BOOST_ROOT=$HOME/boost_1_66_0" >> ~/.bash_profile
source ~/.bash_profile
cd boost_1_66_0/
./bootstrap.sh "--prefix=$BOOST_ROOT"
./b2 install
```
Install [mongo-cxx-driver (release/stable)](https://github.com/mongodb/mongo-cxx-driver):

```bash
cd ~
curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz
tar xf mongo-c-driver-1.9.3.tar.gz
cd mongo-c-driver-1.9.3
./configure --enable-static --enable-ssl=openssl --disable-automatic-init-and-cleanup --prefix=/usr/local
make -j$( nproc )
sudo make install
git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
cd mongo-cxx-driver/build
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
sudo make -j$( nproc )
```

Install [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git):

```bash
cd ~
git clone https://github.com/cryptonomex/secp256k1-zkp.git
cd secp256k1-zkp
./autogen.sh
./configure
make -j$( nproc )
sudo make install
```

By default LLVM and clang do not include the WASM build target, so you will have to build it yourself:

```bash
mkdir  ~/wasm-compiler
cd ~/wasm-compiler
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
cd llvm/tools
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
cd ..
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=.. -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
make -j$( nproc ) install
```

Your environment is set up. Now you can <a href="#runanode">build EOS and run a node</a>.

<a name="manualdepubuntu"></a>
### Clean install Ubuntu 16.04 & Linux Mint 18

Install the development toolkit:

```bash
sudo apt-get update
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
sudo apt-get install clang-4.0 lldb-4.0 libclang-4.0-dev cmake make \
                     libbz2-dev libssl-dev libgmp3-dev \
                     autotools-dev build-essential \
                     libbz2-dev libicu-dev python-dev \
                     autoconf libtool git mongodb
```

Install Boost 1.66:

```bash
cd ~
wget -c 'https://sourceforge.net/projects/boost/files/boost/1.66.0/boost_1_66_0.tar.bz2/download' -O boost_1.66.0.tar.bz2
tar xjf boost_1.66.0.tar.bz2
cd boost_1_66_0/
echo "export BOOST_ROOT=$HOME/boost_1_66_0" >> ~/.bash_profile
source ~/.bash_profile
./bootstrap.sh "--prefix=$BOOST_ROOT"
./b2 install
source ~/.bash_profile
```

Install [mongo-cxx-driver (release/stable)](https://github.com/mongodb/mongo-cxx-driver):

```bash
cd ~
curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz
tar xf mongo-c-driver-1.9.3.tar.gz
cd mongo-c-driver-1.9.3
./configure --enable-static --enable-ssl=openssl --disable-automatic-init-and-cleanup --prefix=/usr/local
make -j$( nproc )
sudo make install
git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
cd mongo-cxx-driver/build
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
sudo make -j$( nproc )
```

Install [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git):

```bash
cd ~
git clone https://github.com/cryptonomex/secp256k1-zkp.git
cd secp256k1-zkp
./autogen.sh
./configure
make
sudo make install
```

By default LLVM and clang do not include the WASM build target, so you will have to build it yourself:

```bash
mkdir  ~/wasm-compiler
cd ~/wasm-compiler
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
cd llvm/tools
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
cd ..
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=.. -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
make -j4 install
```

Your environment is set up. Now you can <a href="#runanode">build EOS and run a node</a>.

<a name="manualdepmacos"></a>
### MacOS Sierra 10.12.6 & higher

macOS additional Dependencies:

* Brew
* Newest XCode
* MongoDB C++ driver

Upgrade your XCode to the newest version:

```bash
xcode-select --install
```

Install homebrew:

```bash
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Install the dependencies:

```bash
brew update
brew install git automake libtool cmake boost openssl@1.0 llvm@4 gmp ninja gettext mongodb
brew link gettext --force
```
Install [mongo-cxx-driver (release/stable)](https://github.com/mongodb/mongo-cxx-driver):

```bash
cd ~
brew install --force pkgconfig
brew unlink pkgconfig && brew link --force pkgconfig
curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz
tar xf mongo-c-driver-1.9.3.tar.gz
rm -f mongo-c-driver-1.9.3.tar.gz
cd mongo-c-driver-1.9.3
./configure --enable-static --enable-ssl=darwin --disable-automatic-init-and-cleanup --prefix=/usr/local
make -j$( sysctl -in machdep.cpu.core_count )
sudo make install
cd ..
rm -rf mongo-c-driver-1.9.3

git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
cd mongo-cxx-driver/build
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$( sysctl -in machdep.cpu.core_count )
sudo make install
cd ..
rm -rf mongo-cxx-driver
```

Install [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git):

```bash
cd ~
git clone https://github.com/cryptonomex/secp256k1-zkp.git
cd secp256k1-zkp
./autogen.sh
./configure
make -j$( sysctl -in machdep.cpu.core_count )
sudo make install
```

Build LLVM and clang for WASM:

```bash
mkdir  ~/wasm-compiler
cd ~/wasm-compiler
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
cd llvm/tools
git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
cd ..
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=.. -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
make -j$( sysctl -in machdep.cpu.core_count )
make install
```

Your environment is set up. Now you can <a href="#runanode">build EOS and run a node</a>.
