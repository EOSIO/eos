FROM centos:7.6.1810
ENV VERSION 1
ENV PATH="/usr/local/cmake/bin:${PATH}"

RUN yum update -y
