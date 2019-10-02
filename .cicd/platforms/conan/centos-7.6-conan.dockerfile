FROM centos:7.6.1810
ENV VERSION 1
ENV PATH="/usr/local/cmake:${PATH}"

RUN yum update -y
