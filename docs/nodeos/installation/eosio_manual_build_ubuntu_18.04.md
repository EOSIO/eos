# Instructions to build EOSIO manually on Ubuntu 18.04

This guide contains manual instructions to build EOSIO from source on a Ubuntu 18.04 system. These instructions are intended as a backup in case the automated build script process fails.

## Contents

1. [EOSIO Software](#EOSIO-Software)
   1. [EOSIO Dependencies](#EOSIO-Dependencies)
   2. [EOSIO Binaries](#EOSIO-Binaries)
2. [EOSIO Build Instructions](#EOSIO-Build-Instructions)
   1. [System Requirements](#System-Requirements)
   2. [Install EOSIO Dependencies](#Install-EOSIO-Dependencies)
      1. [Install Deps for Regular Users](#Install-Deps-for-Regular-Users)
      2. [Install Deps for Block Producers](#Install-Deps-for-Block-Producers)
   3. [Build EOSIO Binaries](#Build-EOSIO-Binaries)
      1. [Build EOSIO for Regular Users](#Build-EOSIO-for-Regular-Users)
      2. [Build EOSIO for Block Producers](#Build-EOSIO-for-Block-Producers)
3. [EOSIO Installation Instructions](#EOSIO-Installation-Instructions)
3. [EOSIO Test Instructions](#EOSIO-Test-Instructions)

## EOSIO Software

The EOSIO software suite enables developers and users to build and deploy dApps and smart contracts on an EOISO blockchain. Block Producers (BPs) can also run and maintain EOSIO nodes to sustain the distributed operation of the blockchain.

### EOSIO Dependencies

The EOSIO software requires certain software dependencies to build the EOSIO binaries. These dependencies can be built from source or installed from binaries directly. On either case, to guarantee interoperability across different EOSIO software releases, it might be necessary to reproduce the exact dependency binaries used in-house. These are also called "pinned" dependencies.

[Technical Note: For instance, a Boost library update might generate binary differences in a multi-indexed object that causes incompatibilities within a memory mapped file created by a previous version of Boost on the local system where the node runs. Consequently, even an EOSIO patch release might be incompatible with the previous release if a dependency is updated in between.]

### EOSIO Binaries

For the reasons outlined above, unless company security policy dictates otherwise, it is recommended to install and use EOSIO binaries distributed in-house, especially if interoperability with the local copy of the EOSIO blockchain, stored on the local system where the node runs, is required. The instructions presented below assume the first option: building the EOSIO software from source.

## EOSIO Build Instructions

There are two sets of manual instructions to build EOSIO manually . These are intended for:

- Regular Users, who don't mind using the latest version of the EOSIO dependencies.
- Block producers, who might need to reproduce the exact dependencies used in-house.

To build the EOSIO software manually:

1. Make sure your system meets the [System Requirements](#System-Requirements) below; then
2. Follow the instructions under [Install EOSIO Dependencies](#Install-EOSIO-Dependencies); then
3. Follow the instructions under [Build EOSIO Binaries](#Build-EOSIO-Binaries).

Note: For Regular Users or Block Producers alike, unless company security policy dictates otherwise, it is recommended to install the EOSIO binary distributions instead.

### System Requirements

To install the EOSIO software, the following system requirements must be met:

- Physical Memory: 7 Gigabytes of RAM or more.
- Physical Storage: 5 Gigabytes of hard disk space or more.

However, much larger memory/storage requirements are needed to run one or more EOSIO nodes.

### Install EOSIO Dependencies

From base image (root on `/bin/bash` shell):

1. Update packages  
   `# apt update && apt upgrade`
2. Install sudo  
   `# apt install sudo`
3. Create a user account (follow prompts)  
   `# adduser <username>`
4. Add user to sudo group  
   `# usermod -aG sudo <username>`
5. Switch to user account  
   `# su - <username>`
6. Verify the new user profile was loaded  
   `$ pwd` (should show `/home/<username>`)
7. Continue to user account instructions below.

From user account (user at `/home/<username>` directory):

1. Update packages  
   `$ sudo apt update && sudo apt upgrade`
2. Install git  
   `$ sudo apt install -y git`
3. Create and change to eosio directory  
   `$ mkdir -p eosio && cd eosio`
4. Clone release/1.8.x branch from eosio/eos.git repository  
   `$ git clone --single-branch -b release/1.8.x https://github.com/EOSIO/eos.git && cd eos && git submodule update --init --recursive`
5. [Re]create build and output directories  
   `$ rm -rf build && mkdir build && mkdir -p ../1.8 && cd ../1.8 && mkdir -p tmp src opt bin var/log etc lib`
7. Set options directory in `pinned_toolchain.cmake`  
   ```$ bash -c "sed -e 's~@~`pwd`/opt~g' `pwd`/../eos/scripts/pinned_toolchain.cmake &> `pwd`/../eos/build/pinned_toolchain.cmake"```

Install basic dependencies:

1. Update packages and install basic dependencies  
   `$ sudo apt update && sudo apt upgrade && sudo apt install -y build-essential make bzip2 automake libbz2-dev libssl-dev doxygen graphviz libgmp3-dev autotools-dev libicu-dev python2.7 python2.7-dev python3 python3-dev autoconf libtool curl zlib1g-dev ruby libusb-1.0-0-dev libcurl4-gnutls-dev pkg-config patch`
2. Choose instructions for either [Regular Users](#Install-Deps-for-Regular-Users) or [Block Producers](#Block-Producers), accordingly.

#### Install Deps for Regular Users

Install dependencies (Note: make sure current directory is ...`/eosio/1.8`):

1. Install clang and llvm-4 support  
   ```$ sudo apt install -y clang llvm-4.0 && ln -s /usr/lib/llvm-4.0 `pwd`/opt/llvm```
2. Build cmake 3.13.2  
   ```$ bash -c "cd `pwd`/src && curl -LO https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz && tar -xzf cmake-3.13.2.tar.gz && cd cmake-3.13.2 && ./bootstrap --prefix=`pwd` && make -j4 && make install && cd .. && rm -f cmake-3.13.2.tar.gz"```
3. Build Boost 1.70.0 libraries  
   ```$ bash -c "cd `pwd`/src && curl -LO https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2 && tar -xjf boost_1_70_0.tar.bz2 && cd `pwd`/src/boost_1_70_0 && ./bootstrap.sh --prefix=`pwd`/src/boost_1_70_0 && ./b2 -q -j4 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test install && cd .. && rm -f boost_1_70_0.tar.bz2 && rm -rf `pwd`/opt/boost"```
4. Proceed to [Build EOSIO for Regular Users](#Build-EOSIO-for-Regular-Users)

#### Install Deps for Block Producers

Install pinned dependencies (Note: make sure current directory is ...`/eosio/1.8`):

1. Install cmake 3.13.2  
   ```$ bash -c "cd src && curl -LO https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz && tar -xzf cmake-3.13.2.tar.gz && cd cmake-3.13.2 && ./bootstrap --prefix=`pwd` && make -j4 && make install && cd .. && rm -f cmake-3.13.2.tar.gz && export CMAKE=`pwd`/bin/cmake"```
2. Install clang 8 support  
   ```$ bash -c "cd `pwd`/tmp && rm -rf clang8 && git clone --single-branch --branch release_80 https://git.llvm.org/git/llvm.git clang8 && cd clang8 && git checkout 18e41dc && cd tools && git clone --single-branch --branch release_80 https://git.llvm.org/git/lld.git && cd lld && git checkout d60a035 && cd .. && git clone --single-branch --branch release_80 https://git.llvm.org/git/polly.git && cd polly && git checkout 1bc06e5 && cd .. && git clone --single-branch --branch release_80 https://git.llvm.org/git/clang.git clang && cd clang && git checkout a03da8b && patch -p2 < \"`pwd`/../eos/scripts/clang-devtoolset8-support.patch\" && cd tools && mkdir extra && cd extra && git clone --single-branch --branch release_80 https://git.llvm.org/git/clang-tools-extra.git && cd clang-tools-extra && git checkout 6b34834 && cd .. && cd ../../../../projects && git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxx.git && cd libcxx && git checkout 1853712 && cd .. && git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxxabi.git && cd libcxxabi && git checkout d7338a4 && cd .. && git clone --single-branch --branch release_80 https://git.llvm.org/git/libunwind.git && cd libunwind && git checkout 57f6739 && cd .. && git clone --single-branch --branch release_80 https://git.llvm.org/git/compiler-rt.git && cd compiler-rt && git checkout 5bc7979 && cd .. && cd `pwd`/tmp/clang8 && mkdir build && cd build && `pwd`/bin/cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX='`pwd`/opt/clang8' -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_LIBCXX=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD=all -DCMAKE_BUILD_TYPE=Release .. && make -j4 && make install && rm -rf `pwd`/tmp/clang8"```
3. Install llvm 4 support  
   ```$ bash -c "cd `pwd`/opt && git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git llvm && cd llvm && mkdir build && cd build && `pwd`/bin/cmake -DCMAKE_INSTALL_PREFIX='`pwd`/opt/llvm' -DLLVM_TARGETS_TO_BUILD=host -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE='`pwd`/../eos/build/pinned_toolchain.cmake' .. && make -j4 && make install"```
4. Install Boost 1.70.0 libraries  
   ```$ bash -c "export PATH=`pwd`/opt/clang8/bin:$PATH && cd `pwd`/src && curl -LO https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2 && tar -xjf boost_1_70_0.tar.bz2 && cd `pwd`/src/boost_1_70_0 && ./bootstrap.sh --with-toolset=clang --prefix=`pwd`/src/boost_1_70_0 && ./b2 toolset=clang cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I`pwd`/opt/clang8/include/c++/v1' linkflags='-stdlib=libc++' link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j4 install && cd .. && rm -f boost_1_70_0.tar.bz2 && rm -rf `pwd`/opt/boost"```
5. Proceed to [Build EOSIO for Block Producers](#Build-EOSIO-for-Block-Producers)

### Build EOSIO Binaries

To build EOSIO, it is assumed that all EOSIO dependencies have been built and/or installed. If not, please proceed to [Install EOSIO Dependencies](#Install-EOSIO-Dependencies)

#### Build EOSIO for Regular Users

Build EOSIO binaries (Note: make sure current directory is ...`/eosio/1.8`):

1. Change to build directory  
   `$ cd ../eos/build`  
2. Create EOSIO build Makefile  
   ```$ bash -c "`pwd`/../../1.8/bin/cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DCMAKE_INSTALL_PREFIX='`pwd`/../../1.8' -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang' -DCMAKE_PREFIX_PATH='`pwd`/../../1.8' .."```
3. Build EOSIO  
   `$ make`

#### Build EOSIO for Block Producers

Build EOSIO binaries (Note: make sure current directory is ...`/eosio/1.8`):

1. Change to build directory  
   `$ cd ../eos/build`
2. Create EOSIO build Makefile  
   ```$ bash -c "../../1.8/bin/cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DCMAKE_INSTALL_PREFIX='`pwd`/../../1.8' -DCMAKE_TOOLCHAIN_FILE='pinned_toolchain.cmake' -DCMAKE_PREFIX_PATH='`pwd`../../1.8;`pwd`../../1.8/opt/llvm;`pwd`../../1.8/src/boost_1_70_0' .."```
3. Build EOSIO  
   `$ make`

## EOSIO Installation Instructions

To install EOSIO, it is assumed that all EOSIO dependencies have been installed and EOSIO has been built. If not, please proceed to [Install EOSIO Dependencies](#Install-EOSIO-Dependencies) or [Build EOSIO Binaries](#Build-EOSIO-Binaries), accordingly. (Note: make sure current directory is ...`/eos/build`):

1. Install EOSIO  
   `$ make install`
2. Check EOSIO binaries are now installed at ...`/eosio/1.8/bin`  
   `$ ls -al ../../1.8/bin`

## EOSIO Test instructions

To test EOSIO and make sure everything works as expected, run the command below. If all tests pass, the EOSIO build was successful. (Note: make sure current directory is ...`/eos/build`):

1. Test EOSIO build  
   `$ make test`
