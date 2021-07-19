FROM debian:10
ENV VERSION 1

# minimum packages to complete a build and run standard unit test suite
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y build-essential cmake curl git jq libboost-dev \
                       libboost-chrono-dev libboost-date-time-dev libboost-filesystem-dev libboost-iostreams-dev \
                       libboost-program-options-dev libboost-regex-dev libboost-test-dev \
                       libcurl4-gnutls-dev libgmp-dev libssl-dev \
                       libusb-1.0-0-dev libz-dev llvm-dev pkg-config python3 python3-requests

# libpq: blockvault support
RUN apt-get install -y libpq-dev postgresql-server-dev-all
# postgresql: blockvault unit tests
RUN apt-get install -y postgresql
# nodejs & npm: ship unit test
RUN apt-get install -y nodejs npm

# tpm2-tss: TPM support (3.0+ needed for eosio unit test support)
RUN apt-get install -y acl && \
    curl -L https://github.com/tpm2-software/tpm2-tss/releases/download/3.1.0/tpm2-tss-3.1.0.tar.gz | tar zx && \
    cd tpm2-tss-* && \
    ./configure --disable-static --disable-fapi && \
    make -j$(nproc) install && \
    cd .. && \
    rm -r tpm2-tss-*

# tpm2-tools, libtpms & swtpm: TPM unit tests
RUN apt-get install -y uuid-dev && \
    curl -L https://github.com/tpm2-software/tpm2-tools/releases/download/5.1.1/tpm2-tools-5.1.1.tar.gz | tar zx && \
    cd tpm2-tools-* && \
    ./configure && \
    make -j$(nproc) install && \
    cd .. && \
    rm -r tpm2-tools-*
RUN apt-get install -y autoconf libtool && \
    curl -L https://github.com/stefanberger/libtpms/archive/refs/tags/v0.8.4.tar.gz | tar zx && \
    cd libtpms* && \
    autoreconf --install && \
    ./configure --with-tpm2 --with-openssl && \
    make -j$(nproc) install && \
    cd .. && \
    rm -r libtpms*
RUN apt-get install -y libtasn1-dev libjson-glib-dev expect gawk socat libseccomp-dev && \
    curl -L https://github.com/stefanberger/swtpm/archive/refs/tags/v0.6.0.tar.gz | tar zx && \
    cd swtpm* && \
    autoreconf --install && \
    ./configure && \
    make -j$(nproc) install && \
    cd .. && \
    rm -r swtpm*
