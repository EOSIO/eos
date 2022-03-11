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
    libcurl-devel patch vim-common jq \
    libuuid-devel libtasn1-devel expect socat libseccomp-devel iproute && \
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
# upgrade pip installation
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
# build clang10
RUN git clone --single-branch --branch llvmorg-10.0.0 https://github.com/llvm/llvm-project clang10 && \
    mkdir /clang10/build && cd /clang10/build && \
    source /opt/rh/devtoolset-8/enable && \
    source /opt/rh/rh-python36/enable && \
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
# download Boost, apply fix for CVE-2016-9840 and build
ENV BEAST_FIX_URL https://raw.githubusercontent.com/boostorg/beast/3fd090af3b7e69ed7871c64a4b4b86fae45e98da/include/boost/beast/zlib/detail/inflate_stream.ipp
RUN curl -fsSLO https://boostorg.jfrog.io/artifactory/main/release/1.72.0/source/boost_1_72_0.tar.bz2 && \
    tar -xjf boost_1_72_0.tar.bz2 && \
    cd boost_1_72_0 && \
    curl -fsSLo boost/beast/zlib/detail/inflate_stream.ipp "${BEAST_FIX_URL}" && \
    ./bootstrap.sh --with-toolset=clang --prefix=/usr/local && \
    ./b2 toolset=clang cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I/usr/local/include/c++/v1 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fpie' linkflags='-stdlib=libc++ -pie' link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install && \
    cd / && \
    rm -rf boost_1_72_0.tar.bz2 /boost_1_72_0
# TPM support; this is a little tricky because we'd like nodeos static linked with it, but the tpm2-tools needed
# for unit testing will need to be dynamic linked
RUN curl -fsSLO https://github.com/tpm2-software/tpm2-tss/releases/download/3.0.1/tpm2-tss-3.0.1.tar.gz
# build static tpm2-tss; this needs some "patching" by way of removing some duplicate symbols at end of tcti impls
RUN tar xf tpm2-tss-3.0.1.tar.gz && \
    cd tpm2-tss-3.0.1 && \
    head -n -14 src/tss2-tcti/tcti-swtpm.c > tcti-swtpm.c.new && \
    mv tcti-swtpm.c.new src/tss2-tcti/tcti-swtpm.c && \
    head -n -14 src/tss2-tcti/tcti-device.c > tcti-device.c.new && \
    mv tcti-device.c.new src/tss2-tcti/tcti-device.c && \
    head -n -14 src/tss2-tcti/tcti-mssim.c > tcti-mssim.c.new && \
    mv tcti-mssim.c.new src/tss2-tcti/tcti-mssim.c && \
    . /opt/rh/devtoolset-8/enable && \
    ./configure --disable-tcti-cmd --disable-fapi --disable-shared --enable-nodl --disable-doxygen-doc && \
    make -j$(nproc) install && \
    cd .. && \
    rm -rf tpm2-tss-3.0.1
# build dynamic tpm2-tss, do this one last so that the installed pkg-config files reference it
RUN tar xf tpm2-tss-3.0.1.tar.gz && \
    cd tpm2-tss-3.0.1 && \
    . /opt/rh/devtoolset-8/enable && \
    ./configure --disable-static --disable-fapi --disable-doxygen-doc && \
    make -j$(nproc) install && \
    cd .. && \
    rm -rf tpm2-tss-3.0.1*
# build TPM components used in unitests; tpm2-tools first
RUN curl -fsSLO https://github.com/tpm2-software/tpm2-tools/releases/download/4.3.0/tpm2-tools-4.3.0.tar.gz && \
    tar zxf tpm2-tools-4.3.0.tar.gz && \
    cd tpm2-tools-4.3.0 && \
    . /opt/rh/devtoolset-8/enable && \
    PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure && \
    make -j$(nproc) install && \
    cd .. && \
    rm -rf tpm2-tools-4.3.0*
# build libtpms
RUN git clone -b v0.7.3 https://github.com/stefanberger/libtpms && \
    cd libtpms && \
    . /opt/rh/devtoolset-8/enable && \
    autoreconf --install && \
    ./configure --with-tpm2 --with-openssl && \
    make -j$(nproc) install && \
    cd .. && \
    rm -rf libtpms
# build swtpm
RUN git clone -b v0.5.0 https://github.com/stefanberger/swtpm && \
    cd swtpm && \
    . /opt/rh/devtoolset-8/enable && \
    . /opt/rh/rh-python36/enable && \
    pip install cryptography && \
    autoreconf --install && \
    PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure && \
    make -j$(nproc) install && \
    cd .. && \
    rm -rf swtpm
RUN ldconfig
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
