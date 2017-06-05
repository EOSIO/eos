# Eos

Welcome to the Eos source code repository!

## Getting Started
The following instructions overview the process of getting the software, building it, and running a simple test network that produces blocks.

### Setting up a build/development environment
This project is written primarily in C++14 and uses CMake as its build system. An up-to-date C++ toolchain (such as Clang or GCC) and the latest version of CMake is recommended. At the time of this writing, Nathan uses clang 4.0.0 and CMake 3.8.0.

### Installing Dependencies
Eos has the following external dependencies, which must be installed on your system:
 - Boost
 - OpenSSL
 - [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git)

```
git clone https://github.com/cryptonomex/secp256k1-zkp.git
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
producer-id = {"_id":0}
producer-id = {"_id":1}
producer-id = {"_id":2}
producer-id = {"_id":3}
producer-id = {"_id":4}
producer-id = {"_id":5}
producer-id = {"_id":6}
producer-id = {"_id":7}
producer-id = {"_id":8}
producer-id = {"_id":9}
producer-id = {"_id":10}
# Load the block producer plugin, so we can produce blocks
plugin = eos::producer_plugin
```

Now it should be possible to run `eosd` and see it begin producing blocks. At present, the P2P code is not implemented, so only single-node configurations are possible. When the P2P networking is implemented, this instructions will be updated to show how to create an example multi-node testnet.
