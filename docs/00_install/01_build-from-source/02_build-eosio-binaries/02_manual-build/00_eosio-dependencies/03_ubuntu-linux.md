## Dependencies - Manual Install - Ubuntu 16.04 or higher and Linux Mint 18

[[info | Reminder]]
| This section assumes you already have the EOSIO source code. If not, just [Download the EOSIO source](../../../../01_build-from-source/01_download-eosio-source.md).

## Steps

Please follow the steps below to build EOSIO on your selected OS:

1. [Change to EOSIO folder](#1-change-to-eosio-folder)
2. [Install dependencies](#2-install-dependencies)
3. [Build cmake](#3-build-cmake)
4. [Build clang8](#4-build-clang8) <!-- 5. [Copy clang8 cmake file](#5-copy-clang8-cmake-file) -->
5. [Build llvm8](#5-build-llvm8)
6. [Build boost](#6-build-boost)
7. [Build mongodb](#7-build-mongodb)
8. [Build mongodb C driver](#8-build-mongodb-c-driver)
9. [Build mongodb CXX driver](#9-build-mongodb-cxx-driver)
10. [Add mongodb to PATH](#10-add-mongodb-to-path)
11. [Install ccache (Ubuntu 16.04 only)](#11-install-ccache-(ubuntu-16.04-only))

<!--
### 0. download and run base docker image
```sh
docker pull ubuntu:16.04
docker run -it ubuntu:16.04
  (OR)
docker pull ubuntu:18.04
docker run -it ubuntu:18.04
```
-->

### 1. Change to EOSIO folder
```sh
$ cd ~/eosio
```

### 2. Install dependencies

#### Ubuntu 16.04:
```sh
$ apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential git automake \
    libbz2-dev libssl-dev doxygen graphviz libgmp3-dev autotools-dev libicu-dev \
    python2.7 python2.7-dev python3 python3-dev autoconf libtool curl zlib1g-dev \
    sudo ruby libusb-1.0-0-dev libcurl4-gnutls-dev pkg-config apt-transport-https vim-common jq
```

#### Ubuntu 18.04:
```sh
apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y git make \
    bzip2 automake libbz2-dev libssl-dev doxygen graphviz libgmp3-dev \
    autotools-dev libicu-dev python2.7 python2.7-dev python3 python3-dev \
    autoconf libtool g++ gcc curl zlib1g-dev sudo ruby libusb-1.0-0-dev \
    libcurl4-gnutls-dev pkg-config patch ccache vim-common jq
```

<!--
### 3. download eosio
```sh
mkdir -p ~/eosio && cd ~/eosio
git clone --recursive --single-branch -b release/2.0.x https://github.com/EOSIO/eos.git
```
-->

### 3. Build cmake
```sh
$ curl -LO https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz && \
    tar -xzf cmake-3.13.2.tar.gz && \
    cd cmake-3.13.2 && \
    ./bootstrap --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    cd ~/eosio && \
    rm -rf cmake-3.13.2.tar.gz cmake-3.13.2
```

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
    make -j $(nproc) && \
    make install && \
    cd ~/eosio && \
    rm -rf clang8
```

<!--
### 5. Copy clang8 cmake file
```sh
$ cp eos/.cicd/helpers/clang.make /tmp/clang.cmake
```
-->

### 5. Build llvm8
```sh
$ git clone --depth 1 --single-branch --branch release_80 https://github.com/llvm-mirror/llvm.git llvm && \
    cd llvm && \
    mkdir build && \
    cd build && \
    cmake -DLLVM_TARGETS_TO_BUILD=host -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_TOOLCHAIN_FILE='~/eosio/eos/.cicd/helpers/clang.make' -DCMAKE_EXE_LINKER_FLAGS=-pthread -DCMAKE_SHARED_LINKER_FLAGS=-pthread -DLLVM_ENABLE_PIC=NO .. && \
    make -j$(nproc) && \
    make install && \
    cd ~/eosio && \
    rm -rf llvm
```

### 6. Build boost
```sh
$ curl -LO https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2 && \
    tar -xjf boost_1_71_0.tar.bz2 && \
    cd boost_1_71_0 && \
    ./bootstrap.sh --with-toolset=clang --prefix=/usr/local && \
    ./b2 toolset=clang cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I/usr/local/include/c++/v1 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fpie' linkflags='-stdlib=libc++ -pie' link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd ~/eosio && \
    rm -rf boost_1_71_0.tar.bz2 boost_1_71_0
```

### 7. Build mongodb

#### Ubuntu 16.04:
```sh
$ curl -LO http://downloads.mongodb.org/linux/mongodb-linux-x86_64-ubuntu1604-3.6.3.tgz && \
    tar -xzf mongodb-linux-x86_64-ubuntu1604-3.6.3.tgz && \
    rm -f mongodb-linux-x86_64-ubuntu1604-3.6.3.tgz
```

#### Ubuntu 18.04:
```sh
$ curl -LO http://downloads.mongodb.org/linux/mongodb-linux-x86_64-ubuntu1804-4.1.1.tgz && \
    tar -xzf mongodb-linux-x86_64-ubuntu1804-4.1.1.tgz && \
    rm -f mongodb-linux-x86_64-ubuntu1804-4.1.1.tgz
```

### 8. Build mongodb C driver
```sh
$ curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz && \
    tar -xzf mongo-c-driver-1.13.0.tar.gz && \
    cd mongo-c-driver-1.13.0 && \
    mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DCMAKE_TOOLCHAIN_FILE='~/eosio/eos/.cicd/helpers/clang.make' .. && \
    make -j$(nproc) && \
    make install && \
    cd ~/eosio && \
    rm -rf mongo-c-driver-1.13.0.tar.gz mongo-c-driver-1.13.0
```

### 9. Build mongodb CXX driver
```sh
$ curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz -o mongo-cxx-driver-r3.4.0.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.4.0.tar.gz && \
    cd mongo-cxx-driver-r3.4.0 && \
    sed -i 's/\"maxAwaitTimeMS\", count/\"maxAwaitTimeMS\", static_cast<int64_t>(count)/' src/mongocxx/options/change_stream.cpp && \
    sed -i 's/add_subdirectory(test)//' src/mongocxx/CMakeLists.txt src/bsoncxx/CMakeLists.txt && \
    cd build && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_TOOLCHAIN_FILE='~/eosio/eos/.cicd/helpers/clang.make' .. && \
    make -j$(nproc) && \
    make install && \
    cd ~/eosio && \
    rm -rf mongo-cxx-driver-r3.4.0.tar.gz mongo-cxx-driver-r3.4.0
```

### 10. Add mongodb to PATH

#### Ubuntu 16.04:
```sh
$ export PATH=${PATH}:/mongodb-linux-x86_64-ubuntu1604-3.6.3/bin
```

#### Ubuntu 18.04:
```sh
$ export PATH=${PATH}:/mongodb-linux-x86_64-ubuntu1804-4.1.1/bin
```

### 11. Install ccache (Ubuntu 16.04 only)
```sh
$ curl -LO https://github.com/ccache/ccache/releases/download/v3.4.1/ccache-3.4.1.tar.gz && \
    tar -xzf ccache-3.4.1.tar.gz && \
    cd ccache-3.4.1 && \
    ./configure && \
    make && \
    make install && \
    cd ~/eosio && \
    rm -rf ccache-3.4.1.tar.gz ccache-3.4.1
```

<!--
### 12. build eosio
```sh
$ bash -c "mkdir -p ~/eosio/eos/build && cd ~/eosio/eos/build && export PATH=/usr/lib/ccache:\${PATH} && cmake -DCMAKE_BUILD_TYPE='Release' -DBUILD_MONGO_DB_PLUGIN=true -DCMAKE_TOOLCHAIN_FILE=~/eosio/eos/.cicd/helpers/clang.make -DCMAKE_CXX_COMPILER_LAUNCHER=ccache .. && make -j$(nproc)"
```
-->

[[info | What's Next?]]
| The EOSIO dependencies are now installed. Next, you can [Manually Build the EOSIO Binaries](../01_eosio-manual-build.md).
