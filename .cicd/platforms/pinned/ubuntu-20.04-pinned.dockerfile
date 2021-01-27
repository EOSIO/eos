FROM ubuntu:20.04
ENV VERSION 1
# install dependencies.
RUN apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y git make \
    bzip2 automake libbz2-dev libssl-dev doxygen graphviz libgmp3-dev \
    autotools-dev python2.7 python2.7-dev python3 \
    python3-dev python-configparser \
    autoconf libtool g++ gcc curl zlib1g-dev sudo ruby libusb-1.0-0-dev \
    libcurl4-gnutls-dev pkg-config patch vim-common jq gnupg rabbitmq-server && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
# build cmake
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2.tar.gz && \
    tar -xzf cmake-3.16.2.tar.gz && \
    cd cmake-3.16.2 && \
    ./bootstrap --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf cmake-3.16.2.tar.gz cmake-3.16.2
# build clang10
RUN git clone --single-branch --branch llvmorg-10.0.0 https://github.com/llvm/llvm-project clang10 && \
    mkdir /clang10/build && cd /clang10/build && \
    cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX='/usr/local' -DLLVM_ENABLE_PROJECTS='lld;polly;clang;clang-tools-extra;libcxx;libcxxabi;libunwind;compiler-rt' -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_TARGETS_TO_BUILD=host -DCMAKE_BUILD_TYPE=Release ../llvm && \
    make -j $(nproc) && \
    make install && \
    cd / && \
    rm -rf /clang10
COPY ./.cicd/helpers/clang.make /tmp/clang.cmake
# build llvm10
RUN git clone --depth 1 --single-branch --branch llvmorg-10.0.0 https://github.com/llvm/llvm-project llvm && \
    cd llvm/llvm && \
    mkdir build && \
    cd build && \
    cmake -G 'Unix Makefiles' -DLLVM_TARGETS_TO_BUILD=host -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_TOOLCHAIN_FILE='/tmp/clang.cmake' -DCMAKE_EXE_LINKER_FLAGS=-pthread -DCMAKE_SHARED_LINKER_FLAGS=-pthread -DLLVM_ENABLE_PIC=NO -DLLVM_ENABLE_TERMINFO=OFF .. && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -rf /llvm
# install libpq postgresql
ENV TZ=America/Chicago
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
    echo "deb http://apt.postgresql.org/pub/repos/apt focal-pgdg main" > /etc/apt/sources.list.d/pgdg.list && \
    curl -sL https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - && \
    apt-get update && apt-get -y install libpq-dev postgresql-13 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
#build libpqxx
RUN curl -L https://github.com/jtv/libpqxx/archive/7.2.1.tar.gz | tar zxvf - && \
    cd  libpqxx-7.2.1  && \
    cmake -DCMAKE_TOOLCHAIN_FILE=/tmp/clang.cmake -DSKIP_BUILD_TEST=ON -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql -DCMAKE_BUILD_TYPE=Release -S . -B build && \
    cmake --build build && cmake --install build && \
    cd .. && rm -rf libpqxx-7.2.1
# build boost
RUN curl -LO https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.bz2 && \
    tar -xjf boost_1_72_0.tar.bz2 && \
    cd boost_1_72_0 && \
    ./bootstrap.sh --with-toolset=clang --prefix=/usr/local && \
    ./b2 toolset=clang cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I/usr/local/include/c++/v1 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fpie' linkflags='-stdlib=libc++ -pie' link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd / && \
    rm -rf boost_1_72_0.tar.bz2 /boost_1_72_0
# install node 12
RUN curl -fsSL https://deb.nodesource.com/gpgkey/nodesource.gpg.key | apt-key add - && \
    . /etc/lsb-release && \
    echo "deb https://deb.nodesource.com/node_12.x $DISTRIB_CODENAME main" | tee /etc/apt/sources.list.d/nodesource.list && \
    echo "deb-src https://deb.nodesource.com/node_12.x $DISTRIB_CODENAME main" | tee -a /etc/apt/sources.list.d/nodesource.list && \
    apt-get update && \
    apt-get install -y nodejs && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
