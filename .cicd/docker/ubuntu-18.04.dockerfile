FROM ubuntu:18.04
# install dependencies
RUN apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y git make bzip2 automake libbz2-dev libssl-dev doxygen graphviz libgmp3-dev autotools-dev libicu-dev python2.7 python2.7-dev python3 python3-dev autoconf libtool g++ gcc curl zlib1g-dev sudo ruby libusb-1.0-0-dev libcurl4-gnutls-dev pkg-config patch llvm-4.0 clang ccache
# build cmake
RUN curl -LO https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz && \
    tar -xzf cmake-3.13.2.tar.gz && \
    cd cmake-3.13.2 && \
    ./bootstrap --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -f cmake-3.13.2.tar.gz
# build boost
RUN curl -LO https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2 && \
    tar -xjf boost_1_70_0.tar.bz2 && \
    cd boost_1_70_0 && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -j$(nproc) install && \
    cd .. && \
    rm -f boost_1_70_0.tar.bz2     
# build mongodb
RUN curl -LO http://downloads.mongodb.org/linux/mongodb-linux-x86_64-ubuntu1804-4.1.1.tgz && \
    tar -xzf mongodb-linux-x86_64-ubuntu1804-4.1.1.tgz && \
    rm -f mongodb-linux-x86_64-ubuntu1804-4.1.1.tgz
# build mongo c driver
RUN curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz && \
    tar -xzf mongo-c-driver-1.13.0.tar.gz && \
    cd mongo-c-driver-1.13.0 && \
    mkdir -p build && \
    cd build && \
    cmake --DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON .. && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -rf mongo-c-driver-1.13.0.tar.gz
# build mongo c++ driver
RUN curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz -o mongo-cxx-driver-r3.4.0.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.4.0.tar.gz && \
    cd mongo-cxx-driver-r3.4.0 && \
    sed -i 's/\"maxAwaitTimeMS\", count/\"maxAwaitTimeMS\", static_cast<int64_t>(count)/' src/mongocxx/options/change_stream.cpp && \
    sed -i 's/add_subdirectory(test)//' src/mongocxx/CMakeLists.txt src/bsoncxx/CMakeLists.txt && \
    cd build && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -f mongo-cxx-driver-r3.4.0.tar.gz
# add mongodb to path
ENV PATH=${PATH}:/mongodb-linux-x86_64-ubuntu1804-4.1.1/bin
# install buildkite-agent
RUN echo "deb https://apt.buildkite.com/buildkite-agent stable main" > /etc/apt/sources.list.d/buildkite-agent.list && \
    apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 32A37959C2FA5C3C99EFBC32A79206696452D198 && \
    apt-get update && \
	  apt-get install -y apt-transport-https && \
	  apt-get install -y buildkite-agent
# PRE_COMMANDS is executed before cmake
ENV PRE_COMMANDS="export PATH=/usr/lib/ccache:\$PATH"
# CMAKE_EXTRAS is inserted at the end of the cmake command right before the ..
ENV CMAKE_EXTRAS="-DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang'"
# import logging libraries
COPY ./docker/.logging-helpers /tmp/.helpers
# runtime instructions
CMD /workdir/.cicd/docker/entrypoint.sh