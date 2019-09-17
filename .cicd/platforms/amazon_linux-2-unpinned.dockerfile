FROM amazonlinux:2.0.20190508
ENV VERSION 1
# install dependencies.
RUN yum update -y && \
    yum install -y which git procps-ng python3 python3-devel python python-devel clang tar make llvm-devel llvm-static doxygen graphviz vim-common jq
# install conan
RUN pip3 install conan
# install ccache
RUN curl -LO http://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/c/ccache-3.3.4-1.el7.x86_64.rpm && \
    yum install -y ccache-3.3.4-1.el7.x86_64.rpm && \
    rm -rf ccache-3.3.4-1.el7.x86_64.rpm
# build cmake.
RUN curl -LO https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz && \
    tar -xzf cmake-3.13.2.tar.gz && \
    cd cmake-3.13.2 && \
    ./bootstrap --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -rf cmake-3.13.2.tar.gz /cmake-3.13.2