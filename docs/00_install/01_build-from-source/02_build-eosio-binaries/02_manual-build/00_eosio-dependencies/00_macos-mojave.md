## Dependencies - Manual Install - MacOS Mojave 10.14 or Higher

[[info | Reminder]]
| This section assumes you already have the EOSIO source code. If not, just [Download the EOSIO source](../../../../01_build-from-source/01_download-eosio-source.md).

## Steps

Please follow the steps below to build EOSIO on your selected OS:

1. [Change to EOSIO folder](#1-change-to-eosio-folder)
2. [Install Homebrew](#2-install-homebrew)
3. [Install dependencies](#3-install-dependencies)
4. [Build clang8](#4-build-clang8)
5. [Build boost](#5-build-boost)
6. [Build mongodb](#6-build-mongodb)
7. [Build mongodb C driver](#7-build-mongodb-c-driver)
8. [Build mongodb CXX driver](#8-build-mongodb-cxx-driver)

### 1. Change to EOSIO folder
```sh
$ cd ~/eosio
```

### 2. Install Homebrew

```sh
$ which brew && echo 'brew already installed' || /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

### 3. Install dependencies

```sh
$ brew update && brew install git cmake python@2 python libtool libusb \
    graphviz automake wget gmp pkgconfig doxygen openssl@1.1 jq
```

<!--
### 4. download eosio
```sh
mkdir -p ~/eosio && cd ~/eosio
git clone --recursive --single-branch -b release/2.0.x https://github.com/EOSIO/eos.git
```
-->

### 4. Build clang8
```sh
$ git clone --single-branch --branch release_80 https://git.llvm.org/git/llvm.git clang8 && cd clang8 && git checkout 18e41dc && \
    cd tools && git clone --single-branch --branch release_80 https://git.llvm.org/git/lld.git && cd lld && git checkout d60a035 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/polly.git && cd polly && git checkout 1bc06e5 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/clang.git clang && cd clang && git checkout a03da8b && \
    cd tools && mkdir extra && cd extra && git clone --single-branch --branch release_80 https://git.llvm.org/git/clang-tools-extra.git && cd clang-tools-extra && git checkout 6b34834 && \
    cd ~/eosio/clang8/projects && git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxx.git && cd libcxx && git checkout 1853712 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxxabi.git && cd libcxxabi && git checkout d7338a4 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/libunwind.git && cd libunwind && git checkout 57f6739 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/compiler-rt.git && cd compiler-rt && git checkout 5bc7979 && \
    mkdir ~/eosio/clang8/build && cd ~/eosio/clang8/build && \
    cmake -DCMAKE_INSTALL_PREFIX='/usr/local' -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_LIBCXX=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release .. && \
    make -j $(getconf _NPROCESSORS_ONLN) && \
    sudo make install && \
    cd ~/eosio && \
    rm -rf clang8
```

### 5. Build boost
```sh
$ curl -LO https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2 && \
    tar -xjf boost_1_71_0.tar.bz2 && \
    cd boost_1_71_0 && \
    ./bootstrap.sh --prefix=/usr/local && \
    sudo ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install && \
    cd ~/eosio && \
    sudo rm -rf boost_1_71_0.tar.bz2 boost_1_71_0
```

### 6. Build mongodb

```sh
$ curl -LO https://fastdl.mongodb.org/osx/mongodb-osx-ssl-x86_64-3.6.3.tgz && \
    tar -xzf mongodb-osx-ssl-x86_64-3.6.3.tgz && \
    rm -f mongodb-osx-ssl-x86_64-3.6.3.tgz && \
    ln -s ~/mongodb-osx-x86_64-3.6.3 ~/mongodb
```

### 7. Build mongodb C driver
```sh
$ curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz && \
    tar -xzf mongo-c-driver-1.13.0.tar.gz && \
    cd mongo-c-driver-1.13.0 && \
    mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_BSON=ON -DENABLE_SSL=DARWIN -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SASL=OFF -DENABLE_SNAPPY=OFF .. && \
    make -j $(getconf _NPROCESSORS_ONLN) && \
    sudo make install && \
    cd ~/eosio && \
    rm -rf mongo-c-driver-1.13.0.tar.gz mongo-c-driver-1.13.0
```

### 8. Build mongodb CXX driver
```sh
$ curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz -o mongo-cxx-driver-r3.4.0.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.4.0.tar.gz && \
    cd mongo-cxx-driver-r3.4.0/build && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j $(getconf _NPROCESSORS_ONLN) && \
    sudo make install && \
    cd ~/eosio && \
    rm -rf mongo-cxx-driver-r3.4.0.tar.gz mongo-cxx-driver-r3.4.0
```

<!--
### 9. build eosio
```sh
$ bash -c "mkdir -p ~/eosio/eos/build && cd ~/eosio/eos/build && cmake -DCMAKE_BUILD_TYPE='Release' -DBUILD_MONGO_DB_PLUGIN=true -DCMAKE_TOOLCHAIN_FILE=~/eosio/eos/.cicd/helpers/clang.make .. && make -j $(getconf _NPROCESSORS_ONLN)"
```
-->

[[info | What's Next?]]
| The EOSIO dependencies are now installed. Next, you can [Manually Build the EOSIO Binaries](../01_eosio-manual-build.md).
