FROM amazonlinux:2.0.20190508
ENV EOSIO_LOCATION=/root/eosio
ARG EOSIO_INSTALL_LOCATION
ENV EOSIO_INSTALL_LOCATION=/root/install
ENV VERSION 1
# Commands from the documentation are inserted right below this line
# Anything below here is exclusive to our CI/CD
## Set WORKDIR to location we mount into the container
WORKDIR /root
## install ccache
RUN curl -LO http://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/c/ccache-3.3.4-1.el7.x86_64.rpm && \
    yum install -y ccache-3.3.4-1.el7.x86_64.rpm && rm ccache-3.3.4-1.el7.x86_64.rpm
## Cleanup eosio directory (~ 600MB)
RUN rm -rf ${EOSIO_LOCATION}