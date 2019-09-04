FROM fedora:27
ENV VERSION 1
RUN yum update -y && \
    yum install -y which git sudo jq nano
ENV VERBOSE true
RUN git clone https://github.com/EOSIO/eos.git -b release/1.7.x && \
    cd eos && \
    git submodule update --init --recursive && \
    ./scripts/eosio_build.sh -y && \
    cd .. && \
    rm -rf eos
RUN mkdir -p ~/.ssh && \
    chmod 700 ~/.ssh && \
    ssh-keyscan -H github.com >> ~/.ssh/known_hosts
ENV PATH=/usr/lib/llvm-4.0/bin:/root/bin:${PATH}:/root/opt/mongodb/bin
ENV LLVM_DIR=/root/opt/llvm/lib/cmake/llvm
ENV BOOST_ROOT=/root/opt/boost