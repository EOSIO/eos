# Building EOSIO

The instructions below can be used to build and test EOSIO.

# AmazonLinux-2

**NOTE**: You may utilize Conan to manage dependencies for EOSIO on AmazonLinux-2. Instructions for this are located [here](../.conan/AMAZONLINUX-2.md).

## Build Steps

```
yum install -y which git sudo procps-ng util-linux autoconf automake libtool make bzip2 bzip2-devel openssl-devel gmp-devel libstdc++ libcurl-devel libusbx-devel python3 python3-devel python-devel libedit-devel doxygen graphviz clang patch llvm-devel llvm-static vim-common jq

curl -LO https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh

chmod +x cmake-3.15.3-Linux-x86_64.sh

./cmake-3.15.3-Linux-x86_64.sh --prefix=/usr/local --include-subdir

curl -LO https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2

tar -xjf boost_1_71_0.tar.bz2

cd boost_1_71_0/

./bootstrap.sh --prefix=/usr/local/boost-1.71.0

./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install

cd ..

git clone https://github.com/EOSIO/eos.git

cd eos/

git submodule update --init --recursive

/usr/local/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang' -DCMAKE_INSTALL_PREFIX='/usr/local/eos' -Bbuild

cd build/

make -j$(getconf _NPROCESSORS_ONLN)
```

## Test Steps

```
/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test

/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -L nonparallelizable_tests --output-on-failure -T Test
```

# CentOS-7.6

**NOTE**: You may utilize Conan to manage dependencies for EOSIO on CentOS-7.6. Instructions for this are located [here](../.conan/CENTOS-7.6.md).

## Build Steps

```
yum install -y epel-release

yum --enablerepo=extras install -y centos-release-scl && yum --enablerepo=extras install -y devtoolset-8

yum --enablerepo=extras install -y which git autoconf automake libtool make bzip2 doxygen graphviz bzip2-devel openssl-devel gmp-devel ocaml libicu-devel python python-devel rh-python36 llvm7.0-devel llvm7.0-static gettext-devel file libusbx-devel libcurl-devel patch vim-common jq

source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable

curl -LO https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh

chmod +x cmake-3.15.3-Linux-x86_64.sh

./cmake-3.15.3-Linux-x86_64.sh --prefix=/usr/local --include-subdir

curl -LO https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2

tar -xjf boost_1_71_0.tar.bz2

cd boost_1_71_0/

./bootstrap.sh --prefix=/usr/local/boost-1.71.0

./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install

cd ..

git clone https://github.com/EOSIO/eos.git

cd eos/

git submodule update --init --recursive

/usr/local/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DLLVM_DIR='/usr/lib64/llvm7.0/lib/cmake/llvm' -DCMAKE_INSTALL_PREFIX='/usr/local/eos' -Bbuild

cd build/

make -j$(getconf _NPROCESSORS_ONLN)
```

## Test Steps

```
/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test

/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -L nonparallelizable_tests --output-on-failure -T Test
```
# macOS-10.14

**NOTE**: You may utilize Conan to manage dependencies for EOSIO on macOS-10.14. Instructions for this are located [here](../.conan/MACOS-10.14.md).

**NOTE**: This requires Homebrew to be installed on your system.

## Build Steps

```
brew install git cmake python@2 python libtool libusb graphviz automake wget gmp llvm@7 pkgconfig doxygen openssl jq boost

git clone https://github.com/EOSIO/eos.git

cd eos/

git submodule update --init --recursive

cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DCMAKE_INSTALL_PREFIX='/usr/local/eos' -Bbuild

cd build/

make -j$(getconf _NPROCESSORS_ONLN)
```

## Test Steps

```
ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test

ctest -L nonparallelizable_tests --output-on-failure -T Test
```
# Ubuntu-18.04

**NOTE**: You may utilize Conan to manage dependencies for EOSIO on Ubuntu-18.04. Instructions for this are located [here](../.conan/UBUNTU-18.04.md).

## Build Steps

```
apt-get install -y git make bzip2 automake libbz2-dev libssl-dev doxygen graphviz libgmp3-dev autotools-dev libicu-dev python2.7 python2.7-dev python3 python3-dev autoconf libtool g++ gcc curl zlib1g-dev sudo ruby libusb-1.0-0-dev libcurl4-gnutls-dev pkg-config patch llvm-7-dev clang ccache vim-common jq

curl -LO https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh

chmod +x cmake-3.15.3-Linux-x86_64.sh

./cmake-3.15.3-Linux-x86_64.sh --prefix=/usr/local --include-subdir

curl -LO https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2

tar -xjf boost_1_71_0.tar.bz2

cd boost_1_71_0/

./bootstrap.sh --prefix=/usr/local/boost-1.71.0

./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install

cd ..

git clone https://github.com/EOSIO/eos.git

cd eos/

git submodule update --init --recursive

/usr/local/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang' -DLLVM_DIR='/usr/lib/llvm-7/lib/cmake/llvm' -DCMAKE_INSTALL_PREFIX='/usr/local/eos' -Bbuild

cd build/

make -j$(getconf _NPROCESSORS_ONLN)
```

## Test Steps

```
/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test

/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -L nonparallelizable_tests --output-on-failure -T Test
```
## License

[MIT](../LICENSE)

## Important

See [LICENSE](../LICENSE) for copyright and license terms.

All repositories and other materials are provided subject to the terms of this [IMPORTANT](../IMPORTANT.md) notice and you must familiarize yourself with its terms.  The notice contains important information, limitations and restrictions relating to our software, publications, trademarks, third-party resources, and forward-looking statements.  By accessing any of our repositories and other materials, you accept and agree to the terms of the notice.
