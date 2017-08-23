# Eos

[![Build Status](https://travis-ci.org/EOSIO/eos.svg?branch=master)](https://travis-ci.org/EOSIO/eos)

Welcome to the EOS.IO source code repository!

## Getting Started
The following instructions overview the process of getting the software, building it, and running a simple test network that produces blocks.

### Setting up a build/development environment
This project is written primarily in C++14 and uses CMake as its build system. An up-to-date C++ toolchain (such as Clang or GCC) and the latest version of CMake is recommended. At the time of this writing, Nathan uses clang 4.0.0 and CMake 3.8.0.

### Installing Dependencies
Eos has the following external dependencies, which must be installed on your system:
 - Boost 1.64
 - OpenSSL
 - LLVM 4.0 (Ubuntu users must install llvm-4.0 packages from https://apt.llvm.org/)
 - [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git)

```
git clone https://github.com/cryptonomex/secp256k1-zkp.git
cd secp256k1-zkp
./autogen.sh
./configure
make
sudo make install
```

### Getting the code
To download all of the code, download Eos and a recursion or two of submodules. The easiest way to get all of this is to do a recursive clone:

`git clone https://github.com/eosio/eos --recursive`

If a repo is cloned without the `--recursive` flag, the submodules can be retrieved after the fact by running this command from within the repo:

`git submodule update --init --recursive`

### Configuring and building
To do an in-source build, simply run `cmake .` from the top level directory. Out-of-source builds are also supported. To override clang's default choice in compiler, add these flags to the CMake command:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.

After successfully running cmake, simply run `make` to build everything. To run the test suite after building, run the `chain_test` executable in the `tests` folder.

### Creating and launching a single-node testnet
After successfully building the project, the `eosd` binary should be present in the `programs/eosd` directory. Go ahead and run `eosd` -- it will probably exit with an error, but if not, close it immediately with Ctrl-C. Note that `eosd` will have created a directory named `data-dir` containing the default configuration (`config.ini`) and some other internals. This default data storage path can be overridden by passing `--data-dir /path/to/data` to `eosd`.

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
# Load the block producer plugin, so we can produce blocks
plugin = eos::producer_plugin
```

When running eosd in the docker container you need to instruct the cpp socket to accept connections from all interfaces.  Adjust any address you plan to use by changing from `127.0.0.1` to `0.0.0.0`.

For example:

```
# The local IP and port to listen for incoming http connections.
http-server-endpoint = 0.0.0.0:8888
```

After starting the Docker this can be tested from container's host machine:
```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

Now it should be possible to run `eosd` and see it begin producing blocks. At present, the P2P code is not implemented, so only single-node configurations are possible. When the P2P networking is implemented, these instructions will be updated to show how to create an example multi-node testnet.

### Run in docker

So simple and fast operation EOS:
 - [Docker](https://docs.docker.com)
 - [Docker-compose](https://github.com/docker/compose)
 - [Docker-volumes](https://github.com/cpuguy83/docker-volumes)

Build eos images

```
cd eos/Docker
cp ../genesis.json .
docker build --rm -t eosio/eos .
```

Start docker

```
sudo rm -rf /data/store/eos # options 
sudo mkdir -p /data/store/eos
docker-compose -f docker-compose.yml up
```

Run example contracts

```
cd /data/store/eos/contracts/exchange
docker exec docker_eos_1 eosc setcode exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi

cd /data/store/eos/contracts/currency 
docker exec docker_eos_1 eosc setcode currency contracts/currency/currency.wast contracts/currency/currency.abi

```

Done




### How to Build LLVM and clang for WASM

By default LLVM and clang do not include the WASM build target, so you will have to build it yourself. Note that following these instructions will create a version of LLVM that can only build WASM targets.

```
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

### Using the WASM compiler to perform a full build of the project

The WASM_LLVM_CONFIG environment variable is used to find our recently built WASM compiler.
This is needed to compile the example contracts insde eos/contracts folder and their respective tests.

```
git clone https://github.com/eosio/eos --recursive
mkdir -p eos/build && cd eos/build
WASM_LLVM_CONFIG=~/wasm-compiler/llvm/bin/llvm-config cmake ..
make -j4
```
