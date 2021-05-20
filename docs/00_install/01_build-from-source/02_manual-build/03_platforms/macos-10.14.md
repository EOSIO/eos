---
content_title: MacOS 10.14
---

This section contains shell commands to manually download, build, install, test, and uninstall EOSIO and dependencies on MacOS 10.14.

[[info | Building EOSIO is for Advanced Developers]]
| If you are new to EOSIO, it is recommended that you install the [EOSIO Prebuilt Binaries](../../../00_install-prebuilt-binaries.md) instead of building from source.

Select a task below, then copy/paste the shell commands to a Unix terminal to execute:

* [Download EOSIO Repository](#download-eosio-repository)
* [Install EOSIO Dependencies](#install-eosio-dependencies)
* [Build EOSIO](#build-eosio)
* [Install EOSIO](#install-eosio)
* [Test EOSIO](#test-eosio)
* [Uninstall EOSIO](#uninstall-eosio)

[[info | Building EOSIO on another OS?]]
| Visit the [Build EOSIO from Source](../../index.md) section.

## Download EOSIO Repository
These commands set the EOSIO directories, install git, and clone the EOSIO repository.
```sh
# set EOSIO directories
export EOSIO_LOCATION=~/eosio/eos
export EOSIO_INSTALL_LOCATION=$EOSIO_LOCATION/../install
mkdir -p $EOSIO_INSTALL_LOCATION
# install git
brew update && brew install git
# clone EOSIO repository
git clone https://github.com/EOSIO/eos.git $EOSIO_LOCATION
cd $EOSIO_LOCATION && git submodule update --init --recursive
```

## Install EOSIO Dependencies
These commands install the EOSIO software dependencies. Make sure to [Download the EOSIO Repository](#download-eosio-repository) first and set the EOSIO directories.
```sh
# install dependencies
brew install cmake python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl@1.1 jq boost || :
export PATH=$EOSIO_INSTALL_LOCATION/bin:$PATH
```

## Build EOSIO
These commands build the EOSIO software on the specified OS. Make sure to [Install EOSIO Dependencies](#install-eosio-dependencies) first.

[[caution | `EOSIO_BUILD_LOCATION` environment variable]]
| Do NOT change this variable. It is set for convenience only. It should always be set to the `build` folder within the cloned repository.

```sh
export EOSIO_BUILD_LOCATION=$EOSIO_LOCATION/build
mkdir -p $EOSIO_BUILD_LOCATION
cd $EOSIO_BUILD_LOCATION && cmake -DCMAKE_BUILD_TYPE='Release' -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION $EOSIO_LOCATION
cd $EOSIO_BUILD_LOCATION && make -j$(getconf _NPROCESSORS_ONLN)
```

## Install EOSIO
This command installs the EOSIO software on the specified OS. Make sure to [Build EOSIO](#build-eosio) first.
```sh
cd $EOSIO_BUILD_LOCATION && make install
```

## Test EOSIO
These commands validate the EOSIO software installation on the specified OS. This task is optional but recommended. Make sure to [Install EOSIO](#install-eosio) first.
```sh
cd $EOSIO_BUILD_LOCATION && make test
```

## Uninstall EOSIO
These commands uninstall the EOSIO software from the specified OS.
```sh
xargs rm < $EOSIO_BUILD_LOCATION/install_manifest.txt
rm -rf $EOSIO_BUILD_LOCATION
```
