## EOS.IO - The Most Powerful Infrastructure for Decentralized Applications

[![Build Status](https://travis-ci.org/EOSIO/eos.svg?branch=master)](https://travis-ci.org/EOSIO/eos)

Welcome to the EOS.IO source code repository!  EOS.IO software enables developers to create and deploy
high-performance, horizontally scalable, blockchain infrastructure upon which decentralized applications
can be built.

This code is currently alpha-quality and under rapid development. That said,
there is plenty early experimenters can do including running a private multi-node test network and
developing applications (smart contracts).

The public testnet described in the [wiki](https://github.com/EOSIO/eos/wiki/Testnet%3A%20Public) is running the `dawn-2.x` branch.  The `master` branch is no longer compatible with the public testnet.  Instructions are provided below for building either option.

# Resources
1. [EOS.IO Website](https://eos.io)
2. [Documentation](https://eosio.github.io/eos/)
3. [Blog](https://steemit.com/@eosio)
4. [Community Telegram Group](https://t.me/EOSProject)
5. [Developer Telegram Group](https://t.me/joinchat/EaEnSUPktgfoI-XPfMYtcQ)
6. [White Paper](https://github.com/EOSIO/Documentation/blob/master/TechnicalWhitePaper.md)
7. [Roadmap](https://github.com/EOSIO/Documentation/blob/master/Roadmap.md)

# Table of contents

1. [Getting Started](#gettingstarted)
2. [Setting up a build/development environment](#setup)
	1. [Automated build script](#autobuild)
      1. [Clean install Ubuntu 16.10 for a local testnet](#autoubuntulocal)
      2. [Clean install Ubuntu 16.10 for the public testnet](#autoubuntupublic)
      3. [MacOS Sierra 10.12.6 for a local testnet](#automaclocal)
      4. [MacOS Sierra 10.12.6 for the public testnet](#automacpublic)
3. [Building EOS and running a node](#runanode)
	1. [Getting the code](#getcode)
	2. [Building from source code](#build)
	3. [Creating and launching a single-node testnet](#singlenode)
4. [Example Currency Contract Walkthrough](#smartcontracts)
	1. [Example Contracts](#smartcontractexample)
	2. [Setting up a wallet and importing account key](#walletimport)
	3. [Creating accounts for your smart contracts](#createaccounts)
	4. [Upload sample contract to blockchain](#uploadsmartcontract)
	5. [Pushing a message to a sample contract](#pushamessage)
	6. [Reading Currency Contract Balance](#readingcontract)
5. [Running local testnet](#localtestnet)
6. [Running a node on the public testnet](#publictestnet)
7. [Doxygen documentation](#doxygen)
8. [Running EOS in Docker](#docker)
9. [Manual installation of the dependencies](#manualdep)
   1. [Clean install Ubuntu 16.10](#ubuntu)
   2. [MacOS Sierra 10.12.6](#macos)

<a name="gettingstarted"></a>
## Getting Started
The following instructions detail the process of getting the software, building it, running a simple test network that produces blocks, account creation and uploading a sample contract to the blockchain.

<a name="setup"></a>
## Setting up a build/development environment

<a name="autobuild"></a>
### Automated build script

For Ubuntu 16.10 and MacOS Sierra, there is an automated build script that can install all dependencies and builds EOS.

It is called eosio-build.sh with the following inputs.
- architecture [ubuntu|darwin]
- optional mode [full|build]

The second optional input can be `full` or `build` where `full` implies that it installs dependencies and builds eos. If you omit this input then the build script installs dependencies and then builds eos.

```bash
./eosio-build.sh <architecture> <optional mode>
```
Choose whether you will be building for a local testnet or for the public testnet and jump to the appropriate section below.  Clone the EOS repository recursively as described and run eosio-build.sh located in the root `eos` folder.

:warning: **As of February 2018, `master` is under heavy development and is not suitable for experimentation.** :warning:

We strongly recommend following the instructions for building the public testnet version for [Ubuntu](#autoubuntupublic) or [Mac OS X](#automacpublic). `master` is in pieces on the garage floor while we rebuild this hotrod. This notice will be removed when `master` is usable again. Your patience is appreciated.

<a name="autoubuntulocal"></a>
#### :no_entry: Clean install Ubuntu 16.10 for a local testnet :no_entry:

```bash
git clone https://github.com/eosio/eos --recursive

cd eos
./eosio-build.sh ubuntu
```

Now you can proceed to the next step - [Creating and launching a single-node testnet](#singlenode)

<a name="autoubuntupublic"></a>
#### Clean install Ubuntu 16.10 for the public testnet

```bash
git clone https://github.com/eosio/eos --recursive

cd eos
git checkout dawn-2.x
./build.sh ubuntu
```

Now you can proceed to the next step - [Running a node on the public testnet](#publictestnet)

<a name="automaclocal"></a>
#### :no_entry: MacOS Sierra for a local testnet :no_entry:

Before running the script make sure you have updated XCode and brew:

```bash
xcode-select --install
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Then clone the EOS repository recursively and run eosio-build.sh in the root `eos` folder.

```bash
git clone https://github.com/eosio/eos --recursive

cd eos
./eosio-build.sh darwin
```

Now you can proceed to the next step - [Creating and launching a single-node testnet](#singlenode)

<a name="automacpublic"></a>
#### MacOS Sierra for the public testnet

Before running the script make sure you have updated XCode and brew:

```bash
xcode-select --install
ruby -e "$(curl -fsSl https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Then clone the EOS repository recursively, checkout the branch that is compatible with the public testnet, and run eosio-build.sh in the root `eos` folder.

```bash
git clone https://github.com/eosio/eos --recursive

cd eos
git checkout dawn-2.x
./build.sh darwin
```

Now you can proceed to the next step - [Running a node on the public testnet](#publictestnet)

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

* eosiod - server-side blockchain node component
* eosioc - command line interface to interact with the blockchain
* eosiowd - EOS wallet
* eosio-launcher - application for nodes network composing and deployment; [more on eosio-launcher](https://github.com/EOSIO/eos/blob/master/testnet.md)

<a name="singlenode"></a>
### Creating and launching a single-node testnet

After successfully building the project, the `eosiod` binary should be present in the `build/programs/eosiod` directory. Run `eosiod` -- it will probably exit with an error, but if not, close it immediately with <kbd>Ctrl-C</kbd>. If it exited with an error, note that `eosiod` created a directory named `data-dir` containing the default configuration (`config.ini`) and some other internals. This default data storage path can be overridden by passing `--data-dir /path/to/data` to `eosiod`.  These instructions will continue to use the default directory.

Edit the `config.ini` file, adding/updating the following settings to the defaults already in place:

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
plugin = eosio::producer_plugin
# Wallet plugin
plugin = eosio::wallet_api_plugin
# As well as API and HTTP plugins
plugin = eosio::chain_api_plugin
plugin = eosio::http_plugin
```

Now it should be possible to run `eosiod` and see it begin producing blocks.

When running `eosiod` you should get log messages similar to below. It means the blocks are successfully produced.

```
1575001ms thread-0   chain_controller.cpp:235      _push_block          ] initm #1 @2017-09-04T04:26:15  | 0 trx, 0 pending, exectime_ms=0
1575001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initm generated block #1 @ 2017-09-04T04:26:15 with 0 trxs  0 pending
1578001ms thread-0   chain_controller.cpp:235      _push_block          ] initc #2 @2017-09-04T04:26:18  | 0 trx, 0 pending, exectime_ms=0
1578001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initc generated block #2 @ 2017-09-04T04:26:18 with 0 trxs  0 pending
...
```

<a name="smartcontracts"></a>
## Example "Currency" Contract Walkthrough

EOS comes with example contracts that can be uploaded and run for testing purposes. Next we demonstrate how to upload and interact with the sample contract "currency".

<a name="smartcontractexample"></a>
### Example smart contracts

First, run the node

```bash
cd ~/eos/build/programs/eosiod/
./eosiod
```

<a name="walletimport"></a>
### Setting up a wallet and importing account key

As you've previously added `plugin = eosio::wallet_api_plugin` into `config.ini`, EOS wallet will be running as a part of `eosiod` process. Every contract requires an associated account, so first, create a wallet.

```bash
cd ~/eos/build/programs/eosioc/
./eosioc wallet create # Outputs a password that you need to save to be able to lock/unlock the wallet
```

For the purpose of this walkthrough, import the private key of the `inita` account, a test account included within genesis.json, so that you're able to issue API commands under authority of an existing account. The private key referenced below is found within your `config.ini` and is provided to you for testing purposes.

```bash
./eosioc wallet import 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
```

<a name="createaccounts"></a>
### Creating accounts for sample "currency" contract

First, generate some public/private key pairs that will be later assigned as `owner_key` and `active_key`.

```bash
cd ~/eos/build/programs/eosioc/
./eosioc create key # owner_key
./eosioc create key # active_key
```

This will output two pairs of public and private keys

```
Private key: XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Public key: EOSXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

**Important:**
Save the values for future reference.

Run the `create` command where `inita` is the account authorizing the creation of the `currency` account and `PUBLIC_KEY_1` and `PUBLIC_KEY_2` are the values generated by the `create key` command

```bash
./eosioc create account inita currency PUBLIC_KEY_1 PUBLIC_KEY_2
```

You should then get a JSON response back with a transaction ID confirming it was executed successfully.

Go ahead and check that the account was successfully created

```bash
./eosioc get account currency
```

If all went well, you will receive output similar to the following:

```json
{
  "account_name": "currency",
  "eos_balance": "0.0000 EOS",
  "staked_balance": "0.0001 EOS",
  "unstaking_balance": "0.0000 EOS",
  "last_unstaking_time": "2035-10-29T06:32:22",
...
```

Now import the active private key generated previously in the wallet:

```bash
./eosioc wallet import XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

<a name="uploadsmartcontract"></a>
### Upload sample "currency" contract to blockchain

Before uploading a contract, verify that there is no current contract:

```bash
./eosioc get code currency
code hash: 0000000000000000000000000000000000000000000000000000000000000000
```

With an account for a contract created, upload a sample contract:

```bash
./eosioc set contract currency ../../contracts/currency/currency.wast ../../contracts/currency/currency.abi
```

As a response you should get a JSON with a `transaction_id` field. Your contract was successfully uploaded!

You can also verify that the code has been set with the following command:

```bash
./eosioc get code currency
```

It will return something like:
```bash
code hash: 9b9db1a7940503a88535517049e64467a6e8f4e9e03af15e9968ec89dd794975
```

Before using the currency contract, you must issue the currency.

```bash
./eosioc push action currency issue '{"to":"currency","quantity":"1000.0000 CUR"}' --permission currency@active
```

Next verify the currency contract has the proper initial balance:

```bash
./eosioc get table currency currency account
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

The content of the message is `'{"from":"currency","to":"inita","quantity":"20.0000 CUR","memo":"any string"}'`. In this case we are asking the currency contract to transfer funds from itself to someone else. This requires the permission of the currency contract.

```bash
./eosioc push action currency transfer '{"from":"currency","to":"inita","quantity":"20.0000 CUR","memo":"my first transfer"}' --permission currency@active
```

Below is a generalization that shows the `currency` account is only referenced once, to specify which contract to deliver the `transfer` message to.

```bash
./eosioc push action currency transfer '{"from":"${usera}","to":"${userb}","quantity":"20.0000 CUR","memo":""}' --permission ${usera}@active
```

As confirmation of a successfully submitted transaction, you will receive JSON output that includes a `transaction_id` field.

<a name="readingcontract"></a>
### Reading sample "currency" contract balance

So now check the state of both of the accounts involved in the previous transaction.

```bash
./eosioc get table inita currency account
{
  "rows": [{
      "currency": 1381319428,
      "balance": 200000
       }
    ],
  "more": false
}
./eosioc get table currency currency account
{
  "rows": [{
      "currency": 1381319428,
      "balance": 9800000
    }
  ],
  "more": false
}
```

As expected, the receiving account **inita** now has a balance of **20** tokens, and the sending account now has **20** less tokens than its initial supply.

<a name="localtestnet"></a>
## Running multi-node local testnet

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
spawning child, programs/eosiod/eosiod --skip-transaction-signatures --data-dir tn_data_0
spawning child, programs/eosiod/eosiod --skip-transaction-signatures --data-dir tn_data_1
```

To confirm the nodes are running, run the following `eosioc` commands:
```bash
~/eos/build/programs/eosioc
./eosioc -p 8888 get info
./eosioc -p 8889 get info
```

For each command, you should get a JSON response with blockchain information.

You can read more on eosio-launcher and its settings [here](https://github.com/EOSIO/eos/blob/master/testnet.md)

<a name="publictestnet"></a>
## Running a local node connected to the public testnet

To run a local node connected to the public testnet operated by block.one, a script is provided.

```bash
cd ~/eos/build/scripts
./start_npnode.sh
```

This command will use the data folder provided for the instance called `testnet_np`.

You should see the following response:

```bash
Launched eosd.
See testnet_np/stderr.txt for eosd output.
Synching requires at least 8 minutes, depending on network conditions.
```

To confirm eosd operation and synchronization:

```bash
tail -F testnet_np/stderr.txt
```

To exit tail, use Ctrl-C.  During synchronization, you will see log messages similar to:

```bash
3439731ms            chain_plugin.cpp:272          accept_block         ] Syncing Blockchain --- Got block: #200000 time: 2017-12-09T07:56:32 producer: initu
3454532ms            chain_plugin.cpp:272          accept_block         ] Syncing Blockchain --- Got block: #210000 time: 2017-12-09T13:29:52 producer: initc
```

Synchronization is complete when you see log messages similar to:

```bash
42467ms            net_plugin.cpp:1245           start_sync           ] Catching up with chain, our last req is 351734, theirs is 351962 peer ip-10-160-11-116:9876
42792ms            chain_controller.cpp:208      _push_block          ] initt #351947 @2017-12-12T22:59:44  | 0 trx, 0 pending, exectime_ms=0
42793ms            chain_controller.cpp:208      _push_block          ] inito #351948 @2017-12-12T22:59:46  | 0 trx, 0 pending, exectime_ms=0
42793ms            chain_controller.cpp:208      _push_block          ] initd #351949 @2017-12-12T22:59:48  | 0 trx, 0 pending, exectime_ms=0
```

This eosd instance listens on 127.0.0.1:8888 for http requests, on all interfaces at port 9877
for p2p requests, and includes the wallet plugins.

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
* Boost 1.64
* OpenSSL
* LLVM 4.0
* [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git)
* [binaryen](https://github.com/WebAssembly/binaryen.git)

<a name="ubuntu"></a>
### Clean install Ubuntu 16.10

Install the development toolkit:

```bash
sudo apt-get update
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
sudo apt-get install clang-4.0 lldb-4.0 libclang-4.0-dev cmake make \
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
### MacOS Sierra 10.12.6

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
brew install git automake libtool boost openssl llvm@4 gmp ninja gettext
brew link gettext --force
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
cmake . && make
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
echo "export LLVM_DIR=/usr/local/Cellar/llvm/4.0.1/lib/cmake/llvm" >> ~/.bash_profile
source ~/.bash_profile
```
