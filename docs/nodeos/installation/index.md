# Installation

[[info | Building EOSIO is Recommended for Advanced Developers. If you are new to EOSIO it's suggested you use the [Docker Quickstart Guide](docker-quickstart.md)]]

EOSIO can be built on several platforms with various paths to building. The majority of users will prefer to use the autobuild script or docker, while more advanced users, or users wishing to deploy a public node may desire manual methods. The build places content in the `eos/build` folder. The executables can be found in subfolders within the `eos/build/programs` folder.

* [Autobuild Script](#autobuild-script)  - Suitable for the majority of developers, this script builds on Mac OS and many flavors of Linux.

* [Docker Compose](#docker-compose) - By far the fastest installation method, you can have a node up and running in a couple minutes. That said, it requires some additional local configuration for development to run smoothly and follow our provided tutorials.

* [Manual Build](#manual-build)  - Suitable for those who have environments that may be hostile to the autobuild script or for operators who wish to exercise more control over their build.

* [Install Executables](install-executables)  - An optional `make install` step that makes local development a bit more developer friendly.

## Autobuild Script

There is an automated build script that can install all dependencies and build EOSIO.

The script supports the following operating systems.  

Amazon Linux 2
CentOS 7
Ubuntu 16.04
Ubuntu 18.04
MacOS 10.14 (Mojave)

Run the build script from the `eos` folder.

```sh
cd eos
./scripts/eosio_build.sh
```

After the script has completed, install EOSIO 

```sh
sudo ./scripts/eosio_install.sh
```

## Docker Compose

[[info | If you're just trying to get up and running, you may find the [Docker Quickstart Guide](docker-quickstart.md) to be more suitable.]]

[[info | This guide is intended for development environments, if you intend to run EOSIO in a production environment, consider building EOSIO from the [Autobuild Script](#autobuild-script).]]

## Manual Build

To build `eos` manually, use the following steps to create a `build` folder within your `eos` folder and then perform the build. The steps below assume the `eos` repository was cloned into your home (i.e., `~`) folder.  It is also assumes that the necessary dependencies have been installed.  See [Manual Installation of the Dependencies](https://developers.eos.io/eosio-nodeos/docs/overview).

```sh
cd ~
mkdir -p ~/eos/build && cd ~/eos/build
```

On Linux platforms, use this `cmake` command:
``` sh
cmake -DBINARYEN_BIN=~/binaryen/bin -DWASM_ROOT=~/wasm-compiler/llvm -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DBUILD_MONGO_DB_PLUGIN=true ..
```

On MacOS, use this `cmake` command:
``` sh
cmake -DBINARYEN_BIN=~/binaryen/bin -DWASM_ROOT=/usr/local/wasm -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DBUILD_MONGO_DB_PLUGIN=true ..
```

Then on all platforms except macOS:
``` sh
make -j$( nproc )
```

For macOS run this:
``` sh
make -j$(sysctl -n hw.ncpu)
```

Out-of-source builds are also supported. To override clang's default choice in compiler, add these flags to the CMake command:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.

## Install Executables

For ease of contract development, content can be installed in the `/usr/local` folder using the `make install` target. This step is run from the `build` folder. Adequate permission is required to install.

```sh
./eosio_install.sh
```

Please choose your environment:

* [Docker Quickstart](docker-quickstart.md)
* [Installing on MacOS](installing-on-macos.md)
* [Installing on Ubuntu](installing-on-ubuntu.md)
