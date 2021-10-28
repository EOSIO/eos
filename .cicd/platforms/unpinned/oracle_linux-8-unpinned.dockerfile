FROM oraclelinux:8

ENV VERSION 1
# install dependencies.
RUN dnf update -y && \
    dnf install -y \
        autoconf \
        automake \
        bison \
        bzip2 \
        bzip2-devel \
        clang \
        cmake \
        file \
        flex \
        gcc-c++ \
        git \
        glibc-langpack-en \
        glibc-locale-source \
        gmp-devel \
        graphviz \
        jq \
        libcurl-devel \
        libtool \
        libusbx-devel \
        llvm \
        llvm-devel \
        llvm-toolset \
        make \
        ncurses-devel \
        nodejs \
        openssl \
        openssl-devel \
        patch \
        postgresql \
        postgresql-devel \
        postgresql-server \
        python3-pip \
        python38 \
        vim-common \
        which \
        xz && \
    dnf clean all && \
    rm -rf /var/cache/dnf && rm -rf /var/cache/yum

# build erlang from source (required for rabbitmq)
RUN curl -LO https://github.com/erlang/otp/releases/download/OTP-23.3.4.7/otp_src_23.3.4.7.tar.gz && \
    tar -xzvf otp_src_23.3.4.7.tar.gz && \
    cd otp_src_23.3.4.7 && \
    export ERL_TOP=`pwd` && \
    ./configure && \
    make -j$(nproc) && \
    make install && \
    rm -rf /otp_src_23.3.4.7 /otp_src_23.3.4.7.tar.gz

# manually install rabbitmq from binary 
RUN curl -LO https://github.com/rabbitmq/rabbitmq-server/releases/download/v3.9.5/rabbitmq-server-generic-unix-3.9.5.tar.xz && \
    tar -xf rabbitmq-server-generic-unix-3.9.5.tar.xz && \
    mv rabbitmq_server-3.9.5 /usr/share && \
    rm rabbitmq-server-generic-unix-3.9.5.tar.xz && \
    ln -s /usr/share/rabbitmq_server-3.9.5/sbin/rabbitmq-server  /usr/sbin/rabbitmq-server && \
    ln -s /usr/share/rabbitmq_server-3.9.5/sbin/rabbitmq-plugins  /usr/sbin/rabbitmq-plugins && \
    ln -s /usr/share/rabbitmq_server-3.9.5/sbin/rabbitmq-env  /usr/sbin/rabbitmq-env     

# install doxygen package
RUN curl -LO https://yum.oracle.com/repo/OracleLinux/OL8/codeready/builder/x86_64/getPackage/doxygen-1.8.14-12.el8.x86_64.rpm && \
    rpm -i doxygen-1.8.14-12.el8.x86_64.rpm && \
    rm doxygen-1.8.14-12.el8.x86_64.rpm

# build boost
RUN curl -LO https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.bz2 && \
    tar -xjf boost_1_71_0.tar.bz2 && \
    cd boost_1_71_0 && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options \
        --with-chrono --with-test -q -j$(nproc) install && \
    cd / && \
    rm -rf boost_1_71_0.tar.bz2 /boost_1_71_0

# requests module. used by tests
RUN python3 -m pip install requests

# install nvm
RUN curl -LO https://raw.githubusercontent.com/nvm-sh/nvm/v0.35.0/install.sh && \
    bash install.sh

# setup Postgress
RUN localedef -c -f UTF-8 -i en_US en_US.UTF-8 && \
    su - postgres -c initdb
