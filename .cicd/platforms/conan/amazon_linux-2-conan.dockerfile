FROM amazonlinux:2.0.20190508
ENV VERSION 1
ENV PATH="/usr/local/cmake/bin:${PATH}"

RUN yum update -y
