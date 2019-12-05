FROM ubuntu:18.04
ENV EOSIO_LOCATION=/root/eosio
ENV EOSIO_INSTALL_LOCATION=/root/eosio/install
ENV VERSION 1
# Commands from the documentation are inserted right below this line
# Anything below here is exclusive to our CI/CD
## Remove the INSTALL LOCATION bin (set in the docs) and build dir so we don't mess with CI
ENV PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
RUN rm -rf ${EOSIO_LOCATION}/build
# install ccache
RUN apt-get install -y ccache