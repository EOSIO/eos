---
content_title: MacOS 10.14 (native compiler)
---

<!-- This document is aggregated by our internal documentation tool to generate EOSIO documentation. The code within the codeblocks below is used in our CI/CD. It will be converted line by line into statements inside of a temporary Dockerfile and used to build our docker tag for this OS. Therefore, COPY and other Dockerfile-isms are not permitted. Code changes will update hashes and regenerate new docker images, so use with caution and do not modify unless necessary. -->

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
<!-- DAC CLONE -->
```sh
# set EOSIO directories
export EOSIO_LOCATION=$HOME/eosio/eos
export EOSIO_INSTALL_LOCATION=$EOSIO_LOCATION/../install
mkdir -p $EOSIO_INSTALL_LOCATION
# install git
brew update && brew install git
# clone EOSIO repository
git clone https://github.com/EOSIO/eos.git $EOSIO_LOCATION
cd $EOSIO_LOCATION && git submodule update --init --recursive
```
<!-- DAC CLONE END -->

## Install Dependencies
These commands install the EOSIO software dependencies. Make sure to [Download the EOSIO Repository](#download-eosio-repository) first and set the EOSIO directories.
<!-- DAC DEPS -->
```sh
# install dependencies
brew install cmake python@2 python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl@1.1 jq boost || :
PATH=$EOSIO_INSTALL_LOCATION/bin:$PATH
# install mongodb
mkdir -p $EOSIO_INSTALL_LOCATION/bin
cd $EOSIO_INSTALL_LOCATION && curl -OL https://fastdl.mongodb.org/osx/mongodb-osx-ssl-x86_64-3.6.3.tgz
    tar -xzf mongodb-osx-ssl-x86_64-3.6.3.tgz && rm -f mongodb-osx-ssl-x86_64-3.6.3.tgz && \
    mv $EOSIO_INSTALL_LOCATION/mongodb-osx-x86_64-3.6.3/bin/* $EOSIO_INSTALL_LOCATION/bin/ && \
    rm -rf $EOSIO_INSTALL_LOCATION/mongodb-osx-x86_64-3.6.3 && rm -rf $EOSIO_INSTALL_LOCATION/mongodb-osx-ssl-x86_64-3.6.3.tgz
# install mongo-c-driver from source
cd $EOSIO_INSTALL_LOCATION && curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz && \
    tar -xzf mongo-c-driver-1.13.0.tar.gz && cd mongo-c-driver-1.13.0 && \
    mkdir -p cmake-build && cd cmake-build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION -DENABLE_BSON=ON -DENABLE_SSL=DARWIN -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SASL=OFF -DENABLE_SNAPPY=OFF .. && \
    make -j $(getconf _NPROCESSORS_ONLN) && \
    make install && \
    rm -rf $EOSIO_INSTALL_LOCATION/mongo-c-driver-1.13.0.tar.gz $EOSIO_INSTALL_LOCATION/mongo-c-driver-1.13.0
# install mongo-cxx-driver from source
cd $EOSIO_INSTALL_LOCATION && curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz -o mongo-cxx-driver-r3.4.0.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.4.0.tar.gz && cd mongo-cxx-driver-r3.4.0/build && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION .. && \
    make -j $(getconf _NPROCESSORS_ONLN) VERBOSE=1 && \
    make install && \
    rm -rf $EOSIO_INSTALL_LOCATION/mongo-cxx-driver-r3.4.0.tar.gz $EOSIO_INSTALL_LOCATION/mongo-cxx-driver-r3.4.0
```
<!-- DAC DEPS END -->

## Build EOSIO
These commands build the EOSIO software on the specified OS. Make sure to [Install EOSIO Dependencies](#install-eosio-dependencies) first.
<!-- DAC BUILD -->
```sh
mkdir -p $EOSIO_LOCATION/build
cd $EOSIO_LOCATION/build
cmake -DCMAKE_BUILD_TYPE='Release' -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION -DBUILD_MONGO_DB_PLUGIN=true ..
make -j$(getconf _NPROCESSORS_ONLN)
```
<!-- DAC BUILD END -->

## Install EOSIO
This command installs the EOSIO software on the specified OS. Make sure to [Build EOSIO](#build-eosio) first.
<!-- DAC INSTALL -->
```sh
make install
```
<!-- DAC INSTALL END -->

## Test EOSIO
These commands validate the EOSIO software installation on the specified OS. This task is optional but recommended. Make sure to [Install EOSIO](#install-eosio) first.
<!-- DAC TEST -->
```sh
$EOSIO_INSTALL_LOCATION/bin/mongod --fork --logpath $(pwd)/mongod.log --dbpath $(pwd)/mongodata
make test
```
<!-- DAC TEST END -->

## Uninstall EOSIO
These commands uninstall the EOSIO software from the specified OS.
<!-- DAC UNINSTALL -->
```sh
xargs rm < $EOSIO_LOCATION/build/install_manifest.txt
rm -rf $EOSIO_LOCATION/build
```
<!-- DAC UNINSTALL END -->