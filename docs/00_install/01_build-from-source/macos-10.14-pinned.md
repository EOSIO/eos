# MacOS 10.14 (Pinned Compiler)
The following commands will install all of the necessary dependencies for source installing EOSIO.
<!-- The code within the following block is used in our CI/CD. It will be converted line by line into RUN statements inside of a temporary Dockerfile and used to build our docker tag for this OS. 
Therefore, COPY and other Dockerfile-isms are not permitted. -->
## Clone EOSIO Repository
<!-- CLONE -->
```
brew update && brew install git
export EOSIO_LOCATION=$HOME/eosio
git clone https://github.com/EOSIO/eos.git $EOSIO_LOCATION
cd $EOSIO_LOCATION && git submodule update --init --recursive
export EOSIO_INSTALL_LOCATION=$EOSIO_LOCATION/install
mkdir -p $EOSIO_INSTALL_LOCATION
```
<!-- CLONE END -->
## Install Dependencies
<!-- DEPS -->
```
brew install cmake python@2 python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl@1.1 jq || :
PATH=$EOSIO_INSTALL_LOCATION/bin:$PATH
# Boost Fix: eosio/install/bin/../include/c++/v1/stdlib.h:94:15: fatal error: 'stdlib.h' file not found
SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
cd $EOSIO_INSTALL_LOCATION && git clone --single-branch --branch release_80 https://git.llvm.org/git/llvm.git clang8 && cd clang8 && git checkout 18e41dc && \
    cd tools && git clone --single-branch --branch release_80 https://git.llvm.org/git/lld.git && cd lld && git checkout d60a035 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/polly.git && cd polly && git checkout 1bc06e5 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/clang.git clang && cd clang && git checkout a03da8b && \
    cd tools && mkdir extra && cd extra && git clone --single-branch --branch release_80 https://git.llvm.org/git/clang-tools-extra.git && cd clang-tools-extra && git checkout 6b34834 && \
    mkdir -p $EOSIO_INSTALL_LOCATION/clang8/projects && cd $EOSIO_INSTALL_LOCATION/clang8/projects && git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxx.git && cd libcxx && git checkout 1853712 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxxabi.git && cd libcxxabi && git checkout d7338a4 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/libunwind.git && cd libunwind && git checkout 57f6739 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/compiler-rt.git && cd compiler-rt && git checkout 5bc7979 && \
    mkdir $EOSIO_INSTALL_LOCATION/clang8/build && cd $EOSIO_INSTALL_LOCATION/clang8/build && \
    cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_LIBCXX=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release .. && \
    make -j $(getconf _NPROCESSORS_ONLN) && \
    make install && \
    rm -rf $EOSIO_INSTALL_LOCATION/clang8
cd $EOSIO_INSTALL_LOCATION && curl -LO https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2 && \
    tar -xjf boost_1_71_0.tar.bz2 && cd boost_1_71_0 && \
    SDKROOT="$SDKROOT" ./bootstrap.sh --prefix=$EOSIO_INSTALL_LOCATION && \
    SDKROOT="$SDKROOT" ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(getconf _NPROCESSORS_ONLN) install && \
    rm -rf $EOSIO_INSTALL_LOCATION/boost_1_71_0.tar.bz2 $EOSIO_INSTALL_LOCATION/boost_1_71_0
# install mongoDB
cd $EOSIO_INSTALL_LOCATION && curl -OL https://fastdl.mongodb.org/osx/mongodb-osx-ssl-x86_64-3.6.3.tgz
    tar -xzf mongodb-osx-ssl-x86_64-3.6.3.tgz && rm -f mongodb-osx-ssl-x86_64-3.6.3.tgz && \
    ln -s $EOSIO_INSTALL_LOCATION/mongodb-osx-x86_64-3.6.3 $EOSIO_INSTALL_LOCATION/mongodb
# install mongo-c-driver from source
cd $EOSIO_INSTALL_LOCATION && curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz && \
    tar -xzf mongo-c-driver-1.13.0.tar.gz && cd mongo-c-driver-1.13.0 && \
    mkdir -p cmake-build && cd cmake-build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION -DENABLE_BSON=ON -DENABLE_SSL=DARWIN -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SASL=OFF -DENABLE_SNAPPY=OFF .. && \
    make -j $(getconf _NPROCESSORS_ONLN) && \
    make install && \
    rm -f mongo-c-driver-1.13.0.tar.gz
# install mongo-cxx-driver from source
cd $EOSIO_INSTALL_LOCATION && curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz -o mongo-cxx-driver-r3.4.0.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.4.0.tar.gz && cd mongo-cxx-driver-r3.4.0/build && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION .. && \
    make -j $(getconf _NPROCESSORS_ONLN) VERBOSE=1 && \
    make install && \
    rm -f mongo-cxx-driver-r3.4.0.tar.gz
```
<!-- DEPS END -->
## Build EOSIO
<!-- BUILD -->
```
mkdir -p $EOSIO_LOCATION/build
cd $EOSIO_LOCATION/build
cmake -DCMAKE_BUILD_TYPE='Release' -DCMAKE_TOOLCHAIN_FILE=$EOSIO_LOCATION/scripts/pinned_toolchain.cmake -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION ..
make -j$(getconf _NPROCESSORS_ONLN)
```
<!-- BUILD END -->
## Install EOSIO
<!-- INSTALL -->
```
make install
```
<!-- INSTALL END -->
## Test EOSIO
<!-- TEST -->
```
make test
```
<!-- TEST END -->
## Uninstall EOSIO
<!-- UNINSTALL -->
```
xargs rm < $EOSIO_LOCATION/build/install_manifest.txt
rm -rf $EOSIO_LOCATION/build
```
<!-- UNINSTALL END -->