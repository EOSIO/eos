FROM amazonlinux:2.0.20190508
ENV VERSION 1
# install dependencies.
RUN yum update -y && \
    yum install -y which git sudo procps-ng util-linux autoconf automake \
    libtool make bzip2 bzip2-devel openssl openssl-devel gmp-devel libstdc++ libcurl-devel \
    libusbx-devel python3 python3-devel python-devel python-requests python3-requests libedit-devel doxygen \
    graphviz clang patch llvm-devel llvm-static vim-common jq postgresql-server postgresql-devel && \
    yum clean all && rm -rf /var/cache/yum
# install erlang and rabbitmq
RUN curl -s https://packagecloud.io/install/repositories/rabbitmq/erlang/script.rpm.sh | bash && \
    yum install -y erlang
RUN curl -s https://packagecloud.io/install/repositories/rabbitmq/rabbitmq-server/script.rpm.sh | bash && \
    yum install -y rabbitmq-server
# build cmake
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2.tar.gz && \
    tar -xzf cmake-3.16.2.tar.gz && \
    cd cmake-3.16.2 && \
    ./bootstrap --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf cmake-3.16.2.tar.gz cmake-3.16.2
# build boost
ENV BOOST_VERSION 1_78_0
ENV BOOST_VERSION_DOT 1.78.0
RUN curl -fsSLO "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION_DOT}/source/boost_${BOOST_VERSION}.tar.bz2" && \
    tar -xjf "boost_${BOOST_VERSION}.tar.bz2" && \
    cd "boost_${BOOST_VERSION}" && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd / && \
    rm -rf "boost_${BOOST_VERSION}.tar.bz2" "/boost_${BOOST_VERSION}"

# install nvm
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.35.0/install.sh | bash
# load nvm in non-interactive shells
RUN echo 'export NVM_DIR="$HOME/.nvm"' > ~/.bashrc && \
    echo '[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"' >> ~/.bashrc
# install node 10
RUN bash -c '. ~/.bashrc; nvm install --lts=dubnium' && \
    ln -s "/root/.nvm/versions/node/$(ls -p /root/.nvm/versions/node | sort -Vr | head -1)bin/node" /usr/local/bin/node && \
    ln -s "/root/.nvm/versions/node/$(ls -p /root/.nvm/versions/node | sort -Vr | head -1)bin/npm" /usr/local/bin/npm
# setup Postgress
RUN su - postgres -c initdb
