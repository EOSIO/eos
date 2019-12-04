FROM ubuntu:18.04
ENV EOSIO_LOCATION=/root/eosio
ENV EOSIO_INSTALL_LOCATION=/root/eosio/install
ENV VERSION 1
# Commands from the documentation are inserted right below this line
# Anything below here is exclusive to our CI/CD
RUN apt-get install -y ccache