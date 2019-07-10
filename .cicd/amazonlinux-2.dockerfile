FROM amazonlinux:2

# YUM dependencies.
RUN yum update -y \
  && yum install -y which git sudo procps-ng util-linux autoconf automake \
  libtool make bzip2 bzip2-devel openssl-devel gmp-devel libstdc++ libcurl-devel \
  libusbx-devel python3 python3-devel python-devel libedit-devel doxygen \
  graphviz clang patch

# Build appropriate version of CMake.
RUN curl -LO https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz \
  && tar -xzf cmake-3.13.2.tar.gz \
  && cd cmake-3.13.2 \
  && ./bootstrap --prefix=/usr/local \
  && make -j$(nproc) \
  && make install \
  && cd .. \
  && rm -f cmake-3.13.2.tar.gz

# Build appropriate version of LLVM.
RUN git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git llvm \
  && cd llvm \
  && mkdir build \
  && cd build \
  && cmake -G 'Unix Makefiles' -DLLVM_TARGETS_TO_BUILD=host -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. \
  && make -j$(nproc) \
  && make install \
  && cd /

# Build appropriate version of Boost.
RUN curl -LO https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2 \
  && tar -xjf boost_1_70_0.tar.bz2 \
  && cd boost_1_70_0 \
  && ./bootstrap.sh --prefix=/usr/local \
  && ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j$(nproc) install \
  && cd .. \
  && rm -f boost_1_70_0.tar.bz2    

# Build appropriate version of MongoDB.
RUN curl -LO https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-amazon-3.6.3.tgz \
  && tar -xzf mongodb-linux-x86_64-amazon-3.6.3.tgz \
  && rm -f mongodb-linux-x86_64-amazon-3.6.3.tgz

# Build appropriate version of MongoDB C Driver. 
RUN curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz \
  && tar -xzf mongo-c-driver-1.13.0.tar.gz \
  && cd mongo-c-driver-1.13.0 \
  && mkdir -p build \
  && cd build \
  && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SNAPPY=OFF .. \
  && make -j$(nproc) \
  && make install \
  && cd / \
  && rm -rf mongo-c-driver-1.13.0.tar.gz

# Build appropriate version of MongoDB C++ driver.
RUN curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz -o mongo-cxx-driver-r3.4.0.tar.gz \
  && tar -xzf mongo-cxx-driver-r3.4.0.tar.gz \
  && cd mongo-cxx-driver-r3.4.0 \
  && sed -i 's/\"maxAwaitTimeMS\", count/\"maxAwaitTimeMS\", static_cast<int64_t>(count)/' src/mongocxx/options/change_stream.cpp \
  && sed -i 's/add_subdirectory(test)//' src/mongocxx/CMakeLists.txt src/bsoncxx/CMakeLists.txt \
  && cd build \
  && cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. \
  && make -j$(nproc) \
  && make install \
  && cd / \
  && rm -f mongo-cxx-driver-r3.4.0.tar.gz

ENV PATH=${PATH}:/mongodb-linux-x86_64-amazon-3.6.3/bin
