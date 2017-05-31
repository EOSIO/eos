# Eos

Welcome to the Eos source code repository!

## Getting Started
The following instructions overview the process of getting the software, building it, and running a simple test network that produces blocks.

### Setting up a build/development environment
This project is written primarily in C++14 and uses CMake as its build system. An up-to-date C++ toolchain (such as Clang or GCC) and the latest version of CMake is recommended. At the time of this writing, Nathan uses clang 4.0.0 and CMake 3.8.0.

This project also depends on Boost version 1.60 and OpenSSL 1.0.2. Ensure that these libraries, and their development headers, are available with the correct versions.

### Installing Dependencies
EOS depends upon a version of secp256k1-skp that it expects to be installed.

`git clone https://github.com/cryptonomex/secp256k1-zkp.git`
`./autogen.sh`
`./configure`
`make`
`sudo make install`

### Getting the code
To download all of the code, download Eos and a recursion or two of submodules. The easiest way to get all of this is to do a recursive clone:

`git clone https://github.com/eosio/eos --recursive`

If a repo is cloned without the `--recursive` flag, the submodules can be retrieved after the fact by running this command from within the repo:

`git submodule update --init --recursive`

### Configuring and building
To do an in-source build, simply run `cmake .` from the top level directory. Out-of-source builds are also supported. When running CMake, it may be necessary to point to the correct versions of these libraries using environment variables, i.e:

`PKG_CONFIG_PATH=/opt/openssl-1.0/pkgconfig`

To override clang's default choice in compiler, add these flags to the CMake command:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.

After successfully running cmake, simply run `make` to build everything. To run the test suite after building, run the `chain_test` executable in the `tests` folder.

### Creating and launching a testnet
After successfully building the project, the `eosd` binary should be present in the `programs/eosd` directory. Go ahead and run `eosd` -- it will probably exit with an error, but if not, close it immediately with Ctrl-C. Note that `eosd` will have created a directory named `data-dir` containing the default configuration (`config.ini`) and some other internals. This default data storage path can be overridden by passing `--data-dir /path/to/data` to `eosd`.

Edit the `config.ini` file and set the path to the testnet genesis.json file, which is in the top level of the repo:

```genesis-json = /path/to/eos/source/genesis.json```

In our example testnet, we'll have three nodes: two that produce blocks and one that acts as the seed node. So make three copies of the data directory, and name them `seed`, `prod1`, and `prod2`. We'll configure each of these separately, but all three need the `genesis-json` so make sure that setting is set before copying the configs.

Next, edit `seed/config.ini` to set the seed node port:

```listen-endpoint = 127.0.0.1:3000```

At this point, the seednode configuration is done and it can be launched with `eosd --data-dir seed`. It should emit an output something like this, then sit and wait:

```
1285039ms th_a       chain_plugin.cpp:51           plugin_initialize    ] initializing chain plugin
1285039ms th_a       p2p_plugin.cpp:93             plugin_initialize    ] Initialize P2P Plugin
1285039ms th_a       block_log.cpp:88              open                 ] Opening block log at /some/path/to/seed/blocks/blocks.log
1285040ms th_a       chain_plugin.cpp:99           plugin_startup       ] starting chain in read/write mode
1285040ms th_a       chain_plugin.cpp:103          plugin_startup       ] Blockchain started; head block is #0
1285041ms th_a       p2p_plugin.cpp:113            plugin_startup       ] Configuring P2P to listen at 127.0.0.1:3000
1285042ms th_a       p2p_plugin.cpp:126            plugin_startup       ] P2P node listening at 127.0.0.1:3000
```

Now edit the `prod1` and `prod2` configurations to point them to the seednode and to enable the block production plugin:

```
remote-endpoint = 127.0.0.1:3000
plugin = eos::producer_plugin
```

Also, set `prod1` to produce even when the blockchain is stale (otherwise, no node will ever be the first one to produce a block):

```enable-stale-production = true```

Finally, configure `prod1` and `prod2` with some init block producers that our testnet genesis.json defined:

##### prod1:
```
producer-id = {"_id":0}
producer-id = {"_id":1}
producer-id = {"_id":2}
producer-id = {"_id":3}
producer-id = {"_id":4}
producer-id = {"_id":5}
```

##### prod2:
```
producer-id = {"_id":6}
producer-id = {"_id":7}
producer-id = {"_id":8}
producer-id = {"_id":9}
producer-id = {"_id":10}
```

Now start the block producer nodes, and the testnet should start up and begin producing blocks.
