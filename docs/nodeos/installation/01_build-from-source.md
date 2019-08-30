# Build EOSIO from Source

To build the EOSIO binaries from the source code, please follow the steps below:

1. [Download EOSIO Source](#Download-EOSIO-Source)
2. [Build EOSIO Binaries](#Build-EOSIO-Binaries)
3. [Install EOSIO Binaries](#Install-EOSIO-Binaries)

## Download EOSIO Source

To download the EOSIO source code, clone the `eos` repo and its submodules. It is adviced to create a home `eosio` folder first and download all the EOSIO related software there:

```sh
$ mkdir -p ~/eosio && cd ~/eosio
$ git clone https://github.com/EOSIO/eos --recursive
```

If a repository is cloned without the `--recursive` flag, the submodules *must* be updated before starting the build process:

```sh
$ cd ~/eosio/eos
$ git submodule update --init --recursive
```

## Build EOSIO Binaries

[[info | Building EOSIO is Recommended for Advanced Developers]]
| If you are new to EOSIO, it is suggested that you use the [Docker Quickstart Guide](docker-quickstart.md) instead, or [Install EOSIO Prebuilt Binaries](00_install-prebuilt-binaries.md) directly.

EOSIO can be built on several platforms using different building methods. The majority of users may prefer running the autobuild script or docker image, while more advanced users or block producers wishing to deploy a public node, may prefer manual methods. The build process writes content in the `eos/build` folder. The executables can be found in subfolders within the `eos/build/programs` folder.

* [Autobuild Script](#autobuild-script) - Suitable for the majority of developers, this script builds on Mac OS and many flavors of Linux.

* [Docker Compose](#docker-compose) - By far the fastest installation method, you can have a node up and running in a couple minutes. That said, it requires some additional local configuration for development to run smoothly and follow our provided tutorials.

* [Manual Build](#manual-build) - Suitable for those who have environments that may be hostile to the autobuild script or for operators who wish to exercise more control over their build.

### Autobuild Script

An automated build script is provided to install all dependencies and build EOSIO. The script supports the following operating systems:

* Amazon Linux 2
* CentOS 7
* Ubuntu 16.04
* Ubuntu 18.04
* MacOS 10.14 (Mojave)

Run the build script from the `eos` folder:

```sh
$ cd ~/eosio/eos
$ ./scripts/eosio_build.sh
```

After the script has completed, install EOSIO:

```sh
$ cd ~/eosio/eos
$ sudo ./scripts/eosio_install.sh
```

### Docker Compose

[[info]]
| If you just need to get up and running, you may find the [Docker Quickstart Guide](docker-quickstart.md) to be more suitable.

[[info]]
| This guide is intended for development environments, if you intend to run EOSIO in a production environment, consider building EOSIO from the [Autobuild Script](#autobuild-script).

### Manual Build

To build `eos` manually, use the following steps to create a `build` folder within your `eos` folder and then perform the build. The steps below assume the `eos` repository was cloned into your home eosio folder (i.e., `~/eosio`). It is also assumed that the necessary dependencies have been installed. See [EOSIO Software Dependencies](dependencies/index.md).

```sh
$ mkdir -p ~/eosio/eos/build && cd ~/eosio/eos/build
```

On Linux platforms, use this `cmake` command:
```sh
$ cmake -DBINARYEN_BIN=~/binaryen/bin -DWASM_ROOT=~/wasm-compiler/llvm -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DBUILD_MONGO_DB_PLUGIN=true ..
```

On MacOS, use this `cmake` command:
```sh
$ cmake -DBINARYEN_BIN=~/binaryen/bin -DWASM_ROOT=/usr/local/wasm -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DBUILD_MONGO_DB_PLUGIN=true ..
```

Then on all platforms except macOS:
```sh
$ make -j$(nproc)
```

For macOS run this:
```sh
$ make -j$(sysctl -n hw.ncpu)
```

Out-of-source builds are also supported. To override clang's default choice in compiler, add these flags to the CMake command:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.

## Install EOSIO Binaries

For ease of contract development, content can be installed in the `/usr/local` folder using the `make install` target. This step is run from the `eosio_install.sh` script within the `eos/scripts` folder. Adequate permission might be required to install on system folders:

```sh
$ cd ~/eosio/eos
$ sudo ./scripts/eosio_install.sh
```
