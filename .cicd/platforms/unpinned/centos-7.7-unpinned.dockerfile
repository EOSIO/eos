FROM centos:7.7.1908
ENV VERSION 1
# install dependencies.
RUN yum update -y && \
    yum install -y epel-release && \
    yum --enablerepo=extras install -y centos-release-scl && \
    yum --enablerepo=extras install -y devtoolset-8 && \
    yum --enablerepo=extras install -y which git autoconf automake libtool make bzip2 doxygen \
    graphviz bzip2-devel openssl-devel gmp-devel ocaml \
    python python-devel rh-python36 file libusbx-devel \
    libcurl-devel patch vim-common jq llvm-toolset-7.0-llvm-devel llvm-toolset-7.0-llvm-static \
    glibc-locale-source glibc-langpack-en && \
    yum clean all && rm -rf /var/cache/yum
# build cmake
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2.tar.gz && \
    tar -xzf cmake-3.16.2.tar.gz && \
    cd cmake-3.16.2 && \
    source /opt/rh/devtoolset-8/enable && \
    ./bootstrap --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf cmake-3.16.2.tar.gz cmake-3.16.2
# build boost
RUN curl -LO https://boostorg.jfrog.io/artifactory/main/release/1.71.0/source/boost_1_71_0.tar.bz2 && \
    source /opt/rh/devtoolset-8/enable && \
    source /opt/rh/rh-python36/enable && \
    tar -xjf boost_1_71_0.tar.bz2 && \
    cd boost_1_71_0 && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd / && \
    rm -rf boost_1_71_0.tar.bz2 /boost_1_71_0
#install libpq postgresql-server
RUN yum install -y https://download.postgresql.org/pub/repos/yum/reporpms/EL-7-x86_64/pgdg-redhat-repo-latest.noarch.rpm && \
    yum install -y postgresql13-devel postgresql13-server && \
    yum clean all && rm -rf /var/cache/yum
ENV PostgreSQL_ROOT=/usr/pgsql-13   
ENV PKG_CONFIG_PATH=/usr/pgsql-13/lib/pkgconfig:/usr/local/lib64/pkgconfig
#build libpqxx
RUN curl -L https://github.com/jtv/libpqxx/archive/7.2.1.tar.gz | tar zxvf - && \
    cd  libpqxx-7.2.1  && \
    source /opt/rh/devtoolset-8/enable && \
    cmake -DSKIP_BUILD_TEST=ON  -DCMAKE_BUILD_TYPE=Release -S . -B build && \
    cmake --build build && cmake --install build && \
    cd .. && rm -rf libpqxx-7.2.1
# install nvm
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.35.0/install.sh | bash
# load nvm in non-interactive shells
RUN cp ~/.bashrc ~/.bashrc.bak && \
    cat ~/.bashrc.bak | tail -3 > ~/.bashrc && \
    cat ~/.bashrc.bak | head -n '-3' >> ~/.bashrc && \
    rm ~/.bashrc.bak
# install node 10
RUN bash -c '. ~/.bashrc; nvm install --lts=dubnium' && \
    ln -s "/root/.nvm/versions/node/$(ls -p /root/.nvm/versions/node | sort -Vr | head -1)bin/node" /usr/local/bin/node
RUN yum install -y nodejs && \
    yum clean all && rm -rf /var/cache/yum
# setup Postgress
RUN localedef -c -f UTF-8 -i en_US en_US.UTF-8 && \
    su - postgres -c "/usr/pgsql-13/bin/initdb" 