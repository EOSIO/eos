FROM ubuntu:20.04
ENV VERSION 1
# install dependencies.
RUN apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y git make \
    bzip2 automake libbz2-dev libssl-dev doxygen graphviz libgmp3-dev \
    autotools-dev python2.7 python2.7-dev python3 python3-dev python3-pip python3-requests \
    autoconf libtool curl zlib1g-dev sudo ruby libusb-1.0-0-dev \
    libcurl4-gnutls-dev pkg-config patch llvm-7-dev clang-7 vim-common jq g++ gnupg bison libreadline-dev flex && \
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

ENV TZ=America/Chicago

# build libpq and postgres
RUN curl -L https://github.com/postgres/postgres/archive/refs/tags/REL_13_3.tar.gz | tar zxvf - && \
    cd postgres-REL_13_3 && \
    ./configure && make && make install && \
    cd .. && rm -rf postgres-REL_13_3
ENV PostgreSQL_ROOT=/usr/local/pgsql
ENV PKG_CONFIG_PATH=/usr/local/pgsql/lib/pkgconfig:/usr/local/lib64/pkgconfig
ENV PATH="/usr/local/pgsql/bin:${PATH}"
# install node 12
RUN curl -fsSL https://deb.nodesource.com/gpgkey/nodesource.gpg.key | apt-key add - && \
    . /etc/lsb-release && \
    echo "deb https://deb.nodesource.com/node_12.x $DISTRIB_CODENAME main" | tee /etc/apt/sources.list.d/nodesource.list && \
    echo "deb-src https://deb.nodesource.com/node_12.x $DISTRIB_CODENAME main" | tee -a /etc/apt/sources.list.d/nodesource.list && \
    apt-get update && \
    apt-get install -y nodejs && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
RUN useradd -m postgres &&  mkdir /usr/local/pgsql/data && chown postgres:postgres /usr/local/pgsql/data &&  su - postgres -c "/usr/local/pgsql/bin/initdb -D /usr/local/pgsql/data/"
ENV PGDATA=/usr/local/pgsql/data
