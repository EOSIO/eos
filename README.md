# EOS.IO - The Most Powerful Infrastructure for Decentralized Applications

[![Build Status](https://travis-ci.org/EOSIO/eos.svg?branch=master)](https://travis-ci.org/EOSIO/eos)

Welcome to the EOS.IO source code repository!  EOS.IO software enables developers to create and deploy
high-performance, horizontally scalable, blockchain infrastructure upon which decentralized applications
can be built. 

This code is currently alpha-quality and under rapid development. That said,
there is plenty early experimenters can do including, running a private multi-node test network and
develop applications (smart contracts).  

# Resources
1. [EOS.IO Website](https://eos.io)
2. [Blog](https://steemit.com/@eosio)
2. [Roadmap](https://github.com/EOSIO/Documentation/blob/master/Roadmap.md)
3. [Telegram](https://eos.io/chat)
4. [Documentation](https://eosio.github.io/eos/)
5. [White Paper](https://github.com/EOSIO/Documentation/blob/master/TechnicalWhitePaper.md)

# Table of contents

1. [Getting Started](#gettingstarted)
2. [Setting up a build/development environment](#setup)
	1. [Automated build script](#autobuild)
	2. [Clean install Ubuntu 16.10](#ubuntu)
	3. [macOS Sierra 10.12.6](#macos)
3. [Building EOS and running a node](#runanode)
	1. [Getting the code](#getcode)
	2. [Building from source code](#build)
	3. [Creating and launching a single-node testnet](#singlenode)
4. [Accounts and smart contracts](#accountssmartcontracts)
	1. [Example smart contracts](#smartcontractexample)
	2. [Setting up a wallet and importing account key](#walletimport)
	3. [Creating accounts for your smart contracts](#createaccounts)
	4. [Upload sample contract to blockchain](#uploadsmartcontract)
	5. [Pushing a message to a sample contract](#pushamessage)
	6. [Reading Currency Contract Balance](#readingcontract)
5. [Running local testnet](#localtestnet)
6. [Doxygen documentation](#doxygen)
7. [Running EOS in Docker](#docker)
	1. [Running contract in docker example](#dockercontract)

<a name="gettingstarted"></a>
## Getting Started
The following instructions overview the process of getting the software, building it, running a simple test network that produces blocks, account creation and uploading a sample contract to the blockchain.

<a name="setup"></a>
## Setting up a build/development environment
This project is written primarily in C++14 and uses CMake as its build system. An up-to-date Clang and the latest version of CMake is recommended.

Dependencies:

* Clang 4.0.0
* CMake 3.5.1
* Boost 1.64
* OpenSSL
* LLVM 4.0
* [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git)
* [binaryen](https://github.com/WebAssembly/binaryen.git)

<a name="autobuild"></a>
### Automated build script

For Ubuntu 16.10 and macOS Sierra, there is an automated build script that can install all dependencies and builds EOS.

Clone EOS repository recursively as below and run build.sh located in root `eos` folder:

```bash
git clone https://github.com/eosio/eos --recursive

cd eos
./build.sh ubuntu # For ubuntu 
./build.sh darwin # For macOS
```


<a name="ubuntu"></a>
### Clean install Ubuntu 16.10 

Install the development toolkit:

```bash
sudo apt-get update
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
sudo apt-get install clang-4.0 lldb-4.0 cmake make \
                     libbz2-dev libssl-dev libgmp3-dev \
                     autotools-dev build-essential \
                     libbz2-dev libicu-dev python-dev \
                     autoconf libtool git
```

Install Boost 1.64:

```bash
cd ~
wget -c 'https://sourceforge.net/projects/boost/files/boost/1.64.0/boost_1_64_0.tar.bz2/download' -O boost_1.64.0.tar.bz2
tar xjf boost_1.64.0.tar.bz2
cd boost_1_64_0/
echo "export BOOST_ROOT=$HOME/opt/boost_1_64_0" >> ~/.bash_profile
source ~/.bash_profile
./bootstrap.sh "--prefix=$BOOST_ROOT"
./b2 install
source ~/.bash_profile
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

To use the WASM compiler, EOS has an external dependency on [binaryen](https://github.com/WebAssembly/binaryen.git):

```bash
cd ~
git clone https://github.com/WebAssembly/binaryen.git
cd ~/binaryen
git checkout tags/1.37.14
cmake . && make

```

Add `BINARYEN_ROOT` to your .bash_profile:

```bash
echo "export BINARYEN_ROOT=~/binaryen" >> ~/.bash_profile
source ~/.bash_profile
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

<a name="macos"></a>
### macOS Sierra 10.12.6 

macOS additional Dependencies:

* Brew
* Newest XCode

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
brew install git automake libtool boost openssl llvm@4 gmp cmake
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

Install [binaryen v1.37.14](https://github.com/WebAssembly/binaryen.git):

```bash
cd ~
git clone https://github.com/WebAssembly/binaryen.git
cd ~/binaryen
git checkout tags/1.37.14
cmake -DCMAKE_C_COMPILER="$(brew --prefix llvm@4)/bin/clang" -DCMAKE_CXX_COMPILER="$(brew --prefix llvm@4)/bin/clang++" . && make
```

Add `BINARYEN_ROOT` to your .bash_profile:

```bash
echo "export BINARYEN_ROOT=~/binaryen" >> ~/.bash_profile
source ~/.bash_profile
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
make -j4 install
```

Add `WASM_LLVM_CONFIG` and `LLVM_DIR` to your `.bash_profile`:

```bash
echo "export WASM_LLVM_CONFIG=~/wasm-compiler/llvm/bin/llvm-config" >> ~/.bash_profile
echo "export LLVM_DIR=$(brew --prefix llvm@4)/lib/cmake" >> ~/.bash_profile
source ~/.bash_profile
```

<a name="runanode"></a>
## Building EOS and running a node 

<a name="getcode"></a>
### Getting the code 

To download all of the code, download EOS source code and a recursion or two of submodules. The easiest way to get all of this is to do a recursive clone:

```bash
git clone https://github.com/eosio/eos --recursive
```

If a repo is cloned without the `--recursive` flag, the submodules can be retrieved after the fact by running this command from within the repo:

```bash
git submodule update --init --recursive
```

<a name="build"></a>
### Building from source code 

The *WASM_LLVM_CONFIG* environment variable is used to find our recently built WASM compiler.
This is needed to compile the example contracts inside `eos/contracts` folder and their respective tests.

```bash
cd ~
git clone https://github.com/eosio/eos --recursive
mkdir -p ~/eos/build && cd ~/eos/build
cmake -DBINARYEN_BIN=~/binaryen/bin -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib ..
make -j4
```

Out-of-source builds are also supported. To override clang's default choice in compiler, add these flags to the CMake command:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.

To run the test suite after building, run the `chain_test` executable in the `tests` folder.

EOS comes with a number of programs you can find in `~/eos/build/programs`. They are listed below:

* eosd - server-side blockchain node component
* eosc - command line interface to interact with the blockchain
* eos-walletd - EOS wallet
* launcher - application for nodes network composing and deployment; [more on launcher](https://github.com/EOSIO/eos/blob/master/testnet.md)

<a name="singlenode"></a>
### Creating and launching a single-node testnet 

After successfully building the project, the `eosd` binary should be present in the `programs/eosd` directory. Go ahead and run `eosd` -- it will probably exit with an error, but if not, close it immediately with <kbd>Ctrl-C</kbd>. Note that `eosd` created a directory named `data-dir` containing the default configuration (`config.ini`) and some other internals. This default data storage path can be overridden by passing `--data-dir /path/to/data` to `eosd`.

Edit the `config.ini` file, adding the following settings to the defaults already in place:

```
# Load the testnet genesis state, which creates some initial block producers with the default key
genesis-json = /path/to/eos/source/genesis.json
# Enable production on a stale chain, since a single-node test chain is pretty much always stale
enable-stale-production = true
# Enable block production with the testnet producers
producer-name = inita
producer-name = initb
producer-name = initc
producer-name = initd
producer-name = inite
producer-name = initf
producer-name = initg
producer-name = inith
producer-name = initi
producer-name = initj
producer-name = initk
producer-name = initl
producer-name = initm
producer-name = initn
producer-name = inito
producer-name = initp
producer-name = initq
producer-name = initr
producer-name = inits
producer-name = initt
producer-name = initu
# Load the block producer plugin, so you can produce blocks
plugin = eos::producer_plugin
# Wallet plugin
plugin = eos::wallet_api_plugin
# As well as API and HTTP plugins
plugin = eos::chain_api_plugin
plugin = eos::http_plugin
```

Now it should be possible to run `eosd` and see it begin producing blocks.

When running `eosd` you should get log messages similar to below. It means the blocks are successfully produced.

```
1575001ms thread-0   chain_controller.cpp:235      _push_block          ] initm #1 @2017-09-04T04:26:15  | 0 trx, 0 pending, exectime_ms=0
1575001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initm generated block #1 @ 2017-09-04T04:26:15 with 0 trxs  0 pending
1578001ms thread-0   chain_controller.cpp:235      _push_block          ] initc #2 @2017-09-04T04:26:18  | 0 trx, 0 pending, exectime_ms=0
1578001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initc generated block #2 @ 2017-09-04T04:26:18 with 0 trxs  0 pending
1581001ms thread-0   chain_controller.cpp:235      _push_block          ] initd #3 @2017-09-04T04:26:21  | 0 trx, 0 pending, exectime_ms=0
1581001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initd generated block #3 @ 2017-09-04T04:26:21 with 0 trxs  0 pending
1584000ms thread-0   chain_controller.cpp:235      _push_block          ] inite #4 @2017-09-04T04:26:24  | 0 trx, 0 pending, exectime_ms=0
1584000ms thread-0   producer_plugin.cpp:207       block_production_loo ] inite generated block #4 @ 2017-09-04T04:26:24 with 0 trxs  0 pending
1587000ms thread-0   chain_controller.cpp:235      _push_block          ] initf #5 @2017-09-04T04:26:27  | 0 trx, 0 pending, exectime_ms=0
```

<a name="accountssmartcontracts"></a>
## Accounts and smart contracts 

EOS comes with example contracts that can be uploaded and run for testing purposes. To upload and test them, follow the steps below.

<a name="smartcontractexample"></a>
### Example smart contracts 

To publish sample smart contracts you need to create accounts for them.

Run the node:

```bash
cd ~/eos/build/programs/eosd/
./eosd
```

<a name="walletimport"></a>
### Setting up a wallet and importing account key 

Before running API commands you need to import the private key of an account you will be authorizing the transactions under into the EOS wallet.

As you've previously added `plugin = eos::wallet_api_plugin` into `config.ini`, EOS wallet will be running as a part of `eosd` process.

For testing purposes you can use a pre-created account `inita` from the `genesis.json` file.

To login you need to run a command importing an active (not owner!) private key from `inita` account (you can find it in `~/eos/build/programs/eosd/data-dir/config.ini`) to the wallet.

```bash
cd ~/eos/build/programs/eosc/
./eosc wallet create # Outputs a password that you need to save to be able to lock/unlock the wallet
./eosc wallet import 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
```

Now you can issue API commands under `inita` authority.

<a name="createaccounts"></a>
### Creating accounts for your smart contracts 

First, generate public/private key pairs for the `owner_key` and `active_key`. You will need them to create an account:

```bash
cd ~/eos/build/programs/eosc/
./eosc create key
./eosc create key
```

You will get two pairs of a public and private key:

```
Private key: XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Public key:  XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

**Warning:**
Save the values for future reference.

Run `create` command where `PUBLIC_KEY_1` and `PUBLIC_KEY_2` are the values generated by the `create key` command:

```bash
./eosc create account inita currency PUBLIC_KEY_1 PUBLIC_KEY_2 
```

You should get a json response back with a transaction ID confirming it was executed successfully.

Check that account was successfully created: 

```bash
./eosc get account currency
```

You should get a response similar to this:

```json
{
  "name": "currency",
  "eos_balance": 0,
  "staked_balance": 1,
  "unstaking_balance": 0,
  "last_unstaking_time": "2106-02-07T06:28:15"
}
```

<a name="uploadsmartcontract"></a>
### Upload sample contract to blockchain 

Before uploading a contract, you can verify that there is no current contract:

```bash
./eosc get code currency 
code hash: 0000000000000000000000000000000000000000000000000000000000000000
```

With an account for a contract created, you can upload a sample contract:

```bash
./eosc set contract currency ../../contracts/currency/currency.wast ../../contracts/currency/currency.abi
```

As a response you should get a json with a `transaction_id` field. Your contract was successfully uploaded!

You can also verify that the code has been set:

```bash
./eosc get code currency
code hash: 9b9db1a7940503a88535517049e64467a6e8f4e9e03af15e9968ec89dd794975
```

Next you can verify that the currency contract has the proper initial balance:

```bash
./eosc get table currency currency account
{
  "rows": [{
     "account": "account",
     "balance": 1000000000
     }
  ],
  "more": false
}
```

<a name="pushamessage"></a>
### Pushing a message to a sample contract 

To send a message to a contract you need to create a new user account who will be sending the message.

Firstly, generate the keys for the account:

```bash
cd ~/eos/build/programs/eosc/
./eosc create key
./eosc create key
```

And then create the `tester` account:

```bash
./eosc create account inita tester PUBLIC_USER_KEY_1 PUBLIC_USER_KEY_2 
```

After this you can send a message to the contract:

```bash
./eosc push message currency transfer '{"from":"currency","to":"inita","amount":50}' --scope currency,inita --permission currency@active
```

As a confirmation of a successfully submitted transaction you will get a json with a `transaction_id` field.

<a name="readingcontract"></a>
### Reading currency contract balance 


```bash
./eosc get table inita currency account
{
  "rows": [{
      "account": "account",
      "balance": 50 
       }
    ],
  "more": false
}
./eosc get table currency currency account
{
  "rows": [{
      "account": "account",
      "balance": 999999950
    }
  ],
  "more": false
}
```

<a name="localtestnet"></a>
## Running local testnet 

To run a local testnet you can use a `launcher` application provided in `~/eos/build/programs/launcher` folder.

For testing purposes you will run 2 local production nodes talking to each other.

```bash
cd ~/eos/build
cp ../genesis.json ./
./programs/launcher/launcher -p2 --skip-signature
```

This command will generate 2 data folders for each instance of the node: `tn_data_0` and `tn_data_1`.

You should see a following response:

```bash
adding hostname ip-XXX-XXX-XXX
found interface 127.0.0.1
found interface XXX.XX.XX.XX
spawning child, programs/eosd/eosd --skip-transaction-signatures --data-dir tn_data_0
spawning child, programs/eosd/eosd --skip-transaction-signatures --data-dir tn_data_1
```

To confirm the nodes are running, run following `eosc` commands:
```bash
~/eos/build/programs/eosc
./eosc -p 8888 get info
./eosc -p 8889 get info
```

For each you should get a json with a blockchain information.

You can read more on launcher and its settings [here](https://github.com/EOSIO/eos/blob/master/testnet.md)

<a name="doxygen"></a>
## Doxygen documentation 

You can find more detailed API documentation in Doxygen reference: https://eosio.github.io/eos/

<a name="docker"></a>
## Running EOS in Docker 

Simple and fast setup of EOS in Docker is also available. Firstly, install dependencies:

 - [Docker](https://docs.docker.com)
 - [Docker-compose](https://github.com/docker/compose)
 - [Docker-volumes](https://github.com/cpuguy83/docker-volumes)

Build eos image:


```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos
cp genesis.json Docker 
docker build -t eosio/eos -f Docker/Dockerfile .
```

We recommend 6GB+ of memory allocated to Docker to successfully build the image.

Now you can start the Docker container:

```bash
sudo rm -rf /data/store/eos # options 
sudo mkdir -p /data/store/eos
docker-compose -f Docker/docker-compose.yml up
```

Get chain info:

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

<a name="dockercontract"></a>
### Running contract in docker example 

You can run the `eosc` commands via `docker exec` command. For example:

```bash
docker exec docker_eos_1 eosc contract exchange build/contracts/exchange/exchange.wast build/contracts/exchange/exchange.abi
```
