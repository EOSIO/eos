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
        make \
        ncurses-devel \
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

# build clang10
RUN git clone --single-branch --branch llvmorg-10.0.0 https://github.com/llvm/llvm-project clang10 && \
    mkdir /clang10/build && cd /clang10/build && \
    cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX='/usr/local' \
        -DLLVM_ENABLE_PROJECTS='lld;polly;clang;clang-tools-extra;libcxx;libcxxabi;libunwind;compiler-rt' \
        -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_TARGETS_TO_BUILD=host \
        -DCMAKE_BUILD_TYPE=Release ../llvm && \
    make -j $(nproc) && \
    make install && \
    rm -rf /clang10

COPY ./.cicd/helpers/clang.make /tmp/clang.cmake

# build llvm10
RUN git clone --depth 1 --single-branch --branch llvmorg-10.0.0 https://github.com/llvm/llvm-project llvm && \
    cd llvm/llvm && \
    mkdir build && \
    cd build && \
    cmake -G 'Unix Makefiles' -DLLVM_TARGETS_TO_BUILD=host -DLLVM_BUILD_TOOLS=false \
        -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_TOOLCHAIN_FILE='/tmp/clang.cmake' -DCMAKE_EXE_LINKER_FLAGS=-pthread \
        -DCMAKE_SHARED_LINKER_FLAGS=-pthread -DLLVM_ENABLE_PIC=NO -DLLVM_ENABLE_TERMINFO=OFF \
        -DLLVM_ENABLE_Z3_SOLVER=OFF .. && \
    make -j$(nproc) && \
    make install && \
    rm -rf /llvm

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
RUN curl -LO https://boostorg.jfrog.io/artifactory/main/release/1.72.0/source/boost_1_72_0.tar.bz2 && \
    tar -xjf boost_1_72_0.tar.bz2 && \
    cd boost_1_72_0 && \
    ./bootstrap.sh --with-toolset=clang --prefix=/usr/local && \
    ./b2 toolset=clang \
        cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I/usr/local/include/c++/v1 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fpie' \
        linkflags='-stdlib=libc++ -pie' link=static threading=multi --with-iostreams --with-date_time --with-filesystem \
        --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd / && \
    rm -rf boost_1_72_0.tar.bz2 /boost_1_72_0

# requests module. used by tests
RUN python3 -m pip install requests

# install node, needed for tests
RUN dnf update -y && \
    dnf module install -y nodejs:14 && \
    dnf clean all && \
    rm -rf /var/cache/dnf /var/cache/yum

# setup Postgress
RUN localedef -c -f UTF-8 -i en_US en_US.UTF-8 && \
    su - postgres -c initdb
