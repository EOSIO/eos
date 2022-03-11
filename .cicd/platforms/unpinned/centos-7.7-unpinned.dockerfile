FROM centos:7.7.1908
ENV VERSION 1
# install dependencies.
# iproute configures traffic control for p2p_high_latency_test.py test
RUN yum update -y && \
    yum install -y epel-release && \
    yum --enablerepo=extras install -y centos-release-scl && \
    yum --enablerepo=extras install -y devtoolset-8 && \
    yum --enablerepo=extras install -y which git autoconf automake libtool make bzip2 doxygen \
    graphviz bzip2-devel openssl-devel gmp-devel ocaml \
    python python-devel rh-python36 file libusbx-devel \
    libcurl-devel patch vim-common jq llvm-toolset-7.0-llvm-devel llvm-toolset-7.0-llvm-static iproute && \
    yum clean all && rm -rf /var/cache/yum
# install erlang and rabbitmq
RUN curl -fsSLO https://packagecloud.io/install/repositories/rabbitmq/erlang/script.rpm.sh && \
    bash script.rpm.sh && \
    rm script.rpm.sh && \
    yum install -y erlang
RUN curl -fsSLO https://packagecloud.io/install/repositories/rabbitmq/rabbitmq-server/script.rpm.sh && \
    bash script.rpm.sh && \
    rm script.rpm.sh && \
    yum install -y rabbitmq-server
RUN source /opt/rh/rh-python36/enable && \
    pip install --upgrade pip && pip install requests requests_unixsocket
# build cmake
RUN curl -fsSLO https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2.tar.gz && \
    tar -xzf cmake-3.16.2.tar.gz && \
    cd cmake-3.16.2 && \
    source /opt/rh/devtoolset-8/enable && \
    ./bootstrap --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf cmake-3.16.2.tar.gz cmake-3.16.2

# build boost
ENV BOOST_VERSION 1_78_0
ENV BOOST_VERSION_DOT 1.78.0
RUN curl -fsSLO "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION_DOT}/source/boost_${BOOST_VERSION}.tar.bz2" && \
    source /opt/rh/devtoolset-8/enable && \
    source /opt/rh/rh-python36/enable && \
    tar -xjf "boost_${BOOST_VERSION}.tar.bz2" && \
    cd "boost_${BOOST_VERSION}" && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd / && \
    rm -rf "boost_${BOOST_VERSION}.tar.bz2" "/boost_${BOOST_VERSION}"
# install nvm
RUN curl -fsSLO https://raw.githubusercontent.com/nvm-sh/nvm/v0.35.0/install.sh && \
    bash install.sh && \
    rm install.sh
# load nvm in non-interactive shells
RUN cp ~/.bashrc ~/.bashrc.bak && \
    cat ~/.bashrc.bak | tail -3 > ~/.bashrc && \
    cat ~/.bashrc.bak | head -n '-3' >> ~/.bashrc && \
    rm ~/.bashrc.bak
# install node 10
RUN bash -c '. ~/.bashrc; nvm install --lts=dubnium' && \
    ln -s "/root/.nvm/versions/node/$(ls -p /root/.nvm/versions/node | sort -Vr | head -1)bin/node" /usr/local/bin/node
RUN yum install -y nodejs npm && \
    yum clean all && rm -rf /var/cache/yum
