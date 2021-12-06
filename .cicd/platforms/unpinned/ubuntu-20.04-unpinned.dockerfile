FROM ubuntu:20.04
ENV VERSION 1
# install dependencies.
RUN apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    autoconf \
    automake \
    autotools-dev \
    bzip2 \
    curl \
    clang-7 \
    doxygen \
    g++ \
    git \
    gnupg \
    graphviz \
    jq \
    libbz2-dev \
    libcurl4-gnutls-dev \
    libgmp3-dev \
    libpq-dev \
    libssl-dev \
    libusb-1.0-0-dev \
    libtinfo5 \
    libtool \
    llvm-7-dev \
    make \
    patch \
    pkg-config \
    postgresql \
    postgresql-server-dev-all \
    python2.7 \
    python2.7-dev \
    python3 \
    python3-dev \
    python3-pip \
    python3-requests \
    rabbitmq-server \
    ruby \
    sudo \ 
    vim-common \
    zlib1g-dev && \
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
# build boost
RUN curl -LO https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.bz2 && \
    tar -xjf boost_1_71_0.tar.bz2 && \
    cd boost_1_71_0 && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -j$(nproc) install && \
    cd / && \
    rm -rf boost_1_71_0.tar.bz2 /boost_1_71_0
# install node 12
RUN curl -fsSL https://deb.nodesource.com/gpgkey/nodesource.gpg.key | apt-key add - && \
    . /etc/lsb-release && \
    echo "deb https://deb.nodesource.com/node_12.x $DISTRIB_CODENAME main" | tee /etc/apt/sources.list.d/nodesource.list && \
    echo "deb-src https://deb.nodesource.com/node_12.x $DISTRIB_CODENAME main" | tee -a /etc/apt/sources.list.d/nodesource.list && \
    apt-get update && \
    apt-get install -y nodejs && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
