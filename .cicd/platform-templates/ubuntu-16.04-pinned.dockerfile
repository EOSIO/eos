FROM ubuntu:16.04
ENV EOSIO_LOCATION=/root/eosio
ENV EOSIO_INSTALL_LOCATION=/root/eosio/install
ENV VERSION 1
# Commands from the documentation are inserted right below this line
# Anything below here is exclusive to our CI/CD
WORKDIR ${EOSIO_LOCATION}
## Remove the INSTALL LOCATION bin (set in the docs) from PATH so builds don't try and use the cleos installed
ENV PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
## install ccache
RUN curl -LO https://github.com/ccache/ccache/releases/download/v3.4.1/ccache-3.4.1.tar.gz && \
    tar -xzf ccache-3.4.1.tar.gz && \
    cd ccache-3.4.1 && \
    ./configure && \
    make && \
    make install && \
    cd / && \
    rm -rf ccache-3.4.1.tar.gz /ccache-3.4.1
