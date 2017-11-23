FROM ubuntu:16.04 as builder

LABEL author="Xiaobo Tian <peterwillcn@gmail.com>" \
  maintainer="Huang-Ming Huang <huangh@objectcomputing.com>" \
  version="0.2.1" \
  description="This is a base image for building eosio/eos"

RUN echo 'APT::Install-Recommends 0;' >> /etc/apt/apt.conf.d/01norecommends \
  && echo 'APT::Install-Suggests 0;' >> /etc/apt/apt.conf.d/01norecommends \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y sudo wget curl net-tools ca-certificates unzip

RUN echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-4.0 main" >> /etc/apt/sources.list.d/llvm.list \
  && wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add - \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y git-core automake autoconf libtool build-essential pkg-config libtool \
     mpi-default-dev libicu-dev python-dev python3-dev libbz2-dev zlib1g-dev libssl-dev libgmp-dev \
     clang-4.0 lldb-4.0 lld-4.0 llvm-4.0-dev libclang-4.0-dev ninja-build \
  && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/clang clang /usr/lib/llvm-4.0/bin/clang 400 \
  && update-alternatives --install /usr/bin/clang++ clang++ /usr/lib/llvm-4.0/bin/clang++ 400

RUN wget --no-check-certificate https://cmake.org/files/v3.9/cmake-3.9.6-Linux-x86_64.sh \
    && bash cmake-3.9.6-Linux-x86_64.sh --prefix=/usr/local --exclude-subdir --skip-license \
    && rm cmake-3.9.6-Linux-x86_64.sh

ENV CC clang
ENV CXX clang++

RUN wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.bz2 -O - | tar -xj \
    && cd boost_1_64_0 \
    && ./bootstrap.sh --prefix=/usr/local \
    && echo 'using clang : 4.0 : clang++-4.0 ;' >> project-config.jam \
    && ./b2 -d0 -j4 --with-thread --with-date_time --with-system --with-filesystem --with-program_options \
       --with-signals --with-serialization --with-chrono --with-test --with-context --with-locale --with-coroutine toolset=clang link=static install \
    && cd .. && rm -rf boost_1_64_0

RUN wget https://github.com/mongodb/mongo-c-driver/releases/download/1.8.0/mongo-c-driver-1.8.0.tar.gz -O - | tar -xz \
    && cd mongo-c-driver-1.8.0 \
    && ./configure --disable-automatic-init-and-cleanup --prefix=/usr/local \
    && make install \
    && cd .. && rm -rf mongo-c-driver-1.8.0

RUN git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git \
    && git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git llvm/tools/clang \
    && cd llvm \
    && cmake -H. -Bbuild -GNinja -DCMAKE_INSTALL_PREFIX=/opt/wasm -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release  \
    && cmake --build build --target install \
    && cd .. && rm -rf llvm

RUN wget https://github.com/WebAssembly/binaryen/archive/1.37.21.tar.gz -O - | tar -xz \
  && cd binaryen-1.37.21 \
  && cmake -H. -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build --target install \
  && cd .. && rm -rf binaryen-1.37.21


RUN git clone --depth 1 git://github.com/cryptonomex/secp256k1-zkp \
    && cd secp256k1-zkp \
    && ./autogen.sh \
    && ./configure --prefix=/usr/local \
    && make install \
    && cd .. && rm -rf secp256k1-zkp

RUN git clone --depth 1 -b releases/stable git://github.com/mongodb/mongo-cxx-driver \
    && cd mongo-cxx-driver \
    && cmake -H. -Bbuild -G Ninja -DCMAKE_BUILD_TYPE=Release  -DCMAKE_INSTALL_PREFIX=/usr/local\
    && cmake --build build --target install


### If you don't want to change the depedencies, you can commet out above lines and uncoment the following line to get faster build time.
# FROM huangminghuang/eos_builder as builder

RUN git clone -b master --depth 1 https://github.com/EOSIO/eos.git --recursive \
    && cd eos \
    && cmake -H. -B"/tmp/build" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_LLVM_CONFIG=/opt/wasm/bin/llvm-config -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/opt/eos  -DSecp256k1_ROOT_DIR=/usr/local \
    && cmake --build /tmp/build --target install



FROM ubuntu:16.04
LABEL author="Xiaobo Tian <peterwillcn@gmail.com>" \
  maintainer="Huang-Ming Huang <huangh@objectcomputing.com>" \
  version="0.2.1" \
  description="This is an eosio/eos image"
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /opt/eos/bin /opt/eos/bin
COPY --from=builder /tmp/build/contracts /contracts
COPY --from=builder /eos/Docker/config.ini /eos/genesis.json /
COPY start_eosd.sh /opt/eos/bin/start_eosd.sh
RUN chmod +x /opt/eos/bin/start_eosd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
VOLUME /opt/eos/bin/data-dir
ENV PATH /opt/eos/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
