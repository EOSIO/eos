FROM ubuntu:18.04
ENV VERSION 1
# install dependencies.
RUN apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y git python2.7 python2.7-dev python3 python3-dev python3-pip clang llvm-7-dev ccache curl doxygen graphviz vim-common jq
# install conan
RUN pip3 install conan
# build cmake.
RUN curl -LO https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz && \
    tar -xzf cmake-3.13.2.tar.gz && \
    cd cmake-3.13.2 && \
    ./bootstrap --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -rf cmake-3.13.2.tar.gz /cmake-3.13.2