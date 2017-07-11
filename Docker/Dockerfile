FROM ubuntu:xenial-20170619
MAINTAINER xiaobo (peterwillcn@gmail.com)

RUN echo 'APT::Install-Recommends 0;' >> /etc/apt/apt.conf.d/01norecommends \
 && echo 'APT::Install-Suggests 0;' >> /etc/apt/apt.conf.d/01norecommends \
 && apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y vim sudo wget htop iftop net-tools ca-certificates unzip

RUN echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-4.0 main" >> /etc/apt/sources.list.d/llvm.list \
 && echo "deb-src http://apt.llvm.org/xenial/ llvm-toolchain-xenial-4.0 main" >> /etc/apt/sources.list.d/llvm.list \
 && wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add - \
 && apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y git-core automake autoconf libtool make cmake build-essential automake pkg-config libtool \
    mpi-default-dev libicu-dev python-dev python3-dev libbz2-dev zlib1g-dev clang libssl-dev \
    clang-4.0 lldb-4.0 lld-4.0 \
 && rm -rf /var/lib/apt/lists/*

RUN cd /tmp && wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz \
  && tar zxf boost_1_64_0.tar.gz \
  && cd boost_1_64_0 \
  && ./bootstrap.sh --with-toolset=clang \
  && ./b2 -a -j$(nproc) stage release -sHAVE_ICU=1 --sICU_PATH=/usr \
  && ./b2 install --prefix=/usr \
  && rm -rf /tmp/boost_1_64_0*

RUN update-alternatives --install /usr/bin/clang clang /usr/lib/llvm-4.0/bin/clang 400 \
  && update-alternatives --install /usr/bin/clang++ clang++ /usr/lib/llvm-4.0/bin/clang++ 400

RUN cd /tmp && git clone https://github.com/cryptonomex/secp256k1-zkp.git \
  && tar zcf secp256k1-zkp.tgz secp256k1-zkp && cd secp256k1-zkp \
  && ./autogen.sh && ./configure && make && make install \
  && ldconfig && rm -rf /tmp/secp256k1-zkp*

RUN cd /tmp && git clone https://github.com/EOSIO/eos.git --recursive \
  && touch /tmp/eos/contracts/CMakeLists.txt && cd eos && mkdir build && cd build \
  && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_STANDARD=14 -DCMAKE_INSTALL_PREFIX=/opt/eos ../ \
  && make -j$(nproc) && make install \
  && rm -rf /tmp/eos*

RUN mkdir -p /opt/eos/bin/data-dir
COPY config.ini genesis.json /
COPY entrypoint.sh /sbin
RUN cd /opt/eos/bin && chmod +x /sbin/entrypoint.sh
VOLUME /opt/eos/bin/data-dir
EXPOSE 9876 8888
ENTRYPOINT ["/sbin/entrypoint.sh"]
