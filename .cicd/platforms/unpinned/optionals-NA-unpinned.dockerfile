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
