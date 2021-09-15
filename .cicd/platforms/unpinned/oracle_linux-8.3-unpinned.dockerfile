FROM oraclelinux:8.3

ENV VERSION 1
# install dependencies.
RUN yum update -y && \
    yum install -y which git autoconf automake libtool make bzip2  \
    graphviz bzip2-devel openssl openssl-devel gmp-devel ncurses-devel \
    python38 file libusbx-devel python3-pip libcurl-devel patch vim-common jq \
    glibc-locale-source glibc-langpack-en postgresql-server postgresql-devel && \
    yum clean all && rm -rf /var/cache/yum

RUN dnf update && dnf install -y clang cmake llvm llvm-devel llvm-toolset && \
    dnf clean all 

# doxygen prereqs
RUN yum install -y flex bison && yum clean all && rm -rf /var/cache/yum

# build erlang from source (required for rabbitmq)
RUN curl -LO https://github.com/erlang/otp/releases/download/OTP-23.3.4.7/otp_src_23.3.4.7.tar.gz && \
    tar -xzvf otp_src_23.3.4.7.tar.gz && \
    cd otp_src_23.3.4.7 && export ERL_TOP=`pwd` && \
    ./configure && make -j$(nproc) && make install && \
    cd .. && rm -rf otp_src_23.3.4.7 otp_src_23.3.4.7.tar.gz

# manually install rabbitmq from binary 
RUN dnf update && dnf install -y xz && \
    curl -LO https://github.com/rabbitmq/rabbitmq-server/releases/download/v3.9.5/rabbitmq-server-generic-unix-3.9.5.tar.xz && \
    tar -xf rabbitmq-server-generic-unix-3.9.5.tar.xz && \
    mv rabbitmq_server-3.9.5 /usr/share && \
    rm rabbitmq-server-generic-unix-3.9.5.tar.xz && \
    echo 'export PATH=$PATH:/usr/share/rabbitmq_server-3.9.5' >> ~/.bashrc 
    
# build doxygen
RUN curl -LO https://github.com/doxygen/doxygen/archive/refs/tags/Release_1_9_2.tar.gz && \
    tar -xzvf Release_1_9_2.tar.gz && \
    cd doxygen-Release_1_9_2 && \
    mkdir build && cd build && cmake -G "Unix Makefiles" .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf doxygen-Release_1_9_2 Release_1_9_2.tar.gz

# build boost
RUN curl -LO https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.bz2 && \
    tar -xjf boost_1_71_0.tar.bz2 && \
    cd boost_1_71_0 && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd / && \
    rm -rf boost_1_71_0.tar.bz2 /boost_1_71_0

# requests module. used by tests
RUN python3 -m pip install requests

# install nvm
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.35.0/install.sh | bash

# install node 10
RUN bash -c '. ~/.bashrc; nvm install --lts=dubnium' && \
    ln -s "/root/.nvm/versions/node/$(ls -p /root/.nvm/versions/node | sort -Vr | head -1)bin/node" /usr/local/bin/node
RUN yum install -y nodejs && \
    yum clean all && rm -rf /var/cache/yum
# setup Postgress
RUN localedef -c -f UTF-8 -i en_US en_US.UTF-8 && \
    su - postgres -c initdb
