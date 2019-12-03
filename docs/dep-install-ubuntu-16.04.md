# Ubuntu 16.04
The following commands will install all of the necessary dependencies for source installing EOSIO.
<!-- The code within the following block is used in our CI/CD. It will be converted line by line into RUN statements inside of a temporary Dockerfile and used to build our docker tag for this OS. 
Therefore, COPY and other Dockerfile-isms are not permitted. -->
```
export EOSIO_LOCATION=$HOME/eosio
git clone git@github.com:EOSIO/eos.git -b develop $EOSIO_INSTALL_LOCATION
export EOSIO_INSTALL_LOCATION=$EOSIO_LOCATION/install
mkdir $EOSIO_INSTALL_LOCATION
apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential git automake \
    libbz2-dev libssl-dev doxygen graphviz libgmp3-dev autotools-dev libicu-dev \
    python2.7 python2.7-dev python3 python3-dev autoconf libtool curl zlib1g-dev \
    sudo ruby libusb-1.0-0-dev libcurl4-gnutls-dev pkg-config apt-transport-https vim-common jq
cd $EOSIO_INSTALL_LOCATION
curl -LO https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz && \
    tar -xzf cmake-3.13.2.tar.gz && \
    cd cmake-3.13.2 && \
    ./bootstrap --prefix=$EOSIO_INSTALL_LOCATION && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf cmake-3.13.2.tar.gz cmake-3.13.2
git clone --single-branch --branch release_80 https://git.llvm.org/git/llvm.git clang8 && cd clang8 && git checkout 18e41dc && \
    cd tools && git clone --single-branch --branch release_80 https://git.llvm.org/git/lld.git && cd lld && git checkout d60a035 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/polly.git && cd polly && git checkout 1bc06e5 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/clang.git clang && cd clang && git checkout a03da8b && \
    cd tools && mkdir extra && cd extra && git clone --single-branch --branch release_80 https://git.llvm.org/git/clang-tools-extra.git && cd clang-tools-extra && git checkout 6b34834 && \
    cd /clang8/projects && git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxx.git && cd libcxx && git checkout 1853712 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/libcxxabi.git && cd libcxxabi && git checkout d7338a4 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/libunwind.git && cd libunwind && git checkout 57f6739 && \
    cd ../ && git clone --single-branch --branch release_80 https://git.llvm.org/git/compiler-rt.git && cd compiler-rt && git checkout 5bc7979 && \
    mkdir $EOSIO_INSTALL_LOCATION/clang8/build && cd $EOSIO_INSTALL_LOCATION/clang8/build && \
    cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_LIBCXX=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release .. && \
    make -j $(nproc) && \
    make install && \
    cd $EOSIO_INSTALL_LOCATION && \
    rm -rf clang8
cp -rfp $EOSIO_LOCATION/scripts/pinned_toolchain.cmake $EOSIO_INSTALL_LOCATION/pinned_toolchain.cmake
git clone --depth 1 --single-branch --branch release_80 https://github.com/llvm-mirror/llvm.git llvm && \
    cd llvm && \
    mkdir build && cd build && \
    cmake -DLLVM_TARGETS_TO_BUILD=host -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION -DCMAKE_TOOLCHAIN_FILE=$EOSIO_INSTALL_LOCATION/pinned_toolchain.cmake -DCMAKE_EXE_LINKER_FLAGS=-pthread -DCMAKE_SHARED_LINKER_FLAGS=-pthread -DLLVM_ENABLE_PIC=NO .. && \
    make -j$(nproc) && \
    make install && \
    cd $EOSIO_INSTALL_LOCATION && \
    rm -rf llvm
curl -LO https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2 && \
    tar -xjf boost_1_71_0.tar.bz2 && \
    cd boost_1_71_0 && \
    ./bootstrap.sh --with-toolset=clang --prefix=$EOSIO_INSTALL_LOCATION && \
    ./b2 toolset=clang cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I$EOSIO_INSTALL_LOCATION/include/c++/v1 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fpie' linkflags='-stdlib=libc++ -pie' link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd $EOSIO_INSTALL_LOCATION && \
    rm -rf boost_1_71_0.tar.bz2 boost_1_71_0
curl -LO http://downloads.mongodb.org/linux/mongodb-linux-x86_64-ubuntu1604-3.6.3.tgz && \
    tar -xzf mongodb-linux-x86_64-ubuntu1604-3.6.3.tgz && \
    rm -f mongodb-linux-x86_64-ubuntu1604-3.6.3.tgz
curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz && \
    tar -xzf mongo-c-driver-1.13.0.tar.gz && \
    cd mongo-c-driver-1.13.0 && \
    mkdir -p build && \
    cd build && \
    cmake --DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DCMAKE_TOOLCHAIN_FILE=$EOSIO_INSTALL_LOCATION/pinned_toolchain.cmake .. && \
    make -j$(nproc) && \
    make install && \
    cd $EOSIO_INSTALL_LOCATION && \
    rm -rf mongo-c-driver-1.13.0.tar.gz mongo-c-driver-1.13.0 
curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz -o mongo-cxx-driver-r3.4.0.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.4.0.tar.gz && \
    cd mongo-cxx-driver-r3.4.0 && \
    sed -i 's/\"maxAwaitTimeMS\", count/\"maxAwaitTimeMS\", static_cast<int64_t>(count)/' src/mongocxx/options/change_stream.cpp && \
    sed -i 's/add_subdirectory(test)//' src/mongocxx/CMakeLists.txt src/bsoncxx/CMakeLists.txt && \
    cd build && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION -DCMAKE_TOOLCHAIN_FILE=$EOSIO_INSTALL_LOCATION/pinned_toolchain.cmake .. && \
    make -j$(nproc) && \
    make install && \
    cd $EOSIO_INSTALL_LOCATION && \
    rm -rf mongo-cxx-driver-r3.4.0.tar.gz mongo-cxx-driver-r3.4.0
PATH=$PATH:$EOSIO_INSTALL_LOCATION/mongodb-linux-x86_64-ubuntu1604-3.6.3/bin
```