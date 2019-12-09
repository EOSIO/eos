FROM ubuntu:18.04
ENV EOSIO_LOCATION=/root/eosio
ENV EOSIO_INSTALL_LOCATION=/root/install
ENV VERSION 1
# Commands from the documentation are inserted right below this line
# Anything below here is exclusive to our CI/CD
## Set WORKDIR to location we mount into the container
WORKDIR /root
## install ccache
RUN apt-get install -y ccache
## Cleanup eosio directory (~ 600MB)
RUN rm -rf ${EOSIO_LOCATION}