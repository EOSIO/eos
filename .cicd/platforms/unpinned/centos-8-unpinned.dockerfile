FROM centos:8
ENV VERSION 1
#install dependencies
RUN yum update -y && \
    yum install -y epel-release  && \
    yum --enablerepo=extras install -y which git autoconf automake libtool make bzip2 && \
    yum --enablerepo=extras install -y  graphviz bzip2-devel openssl-devel gmp-devel  && \
    yum --enablerepo=extras install -y  file libusbx-devel && \
    yum --enablerepo=extras install -y libcurl-devel patch vim-common jq && \
    yum install -y python3 llvm-toolset cmake boost
RUN dnf install -y  https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm && \
    dnf group install -y  "Development Tools" && \
    yum config-manager --set-enabled powertools && \
    yum install -y doxygen ocaml
# install nvm
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.35.0/install.sh | bash
# load nvm in non-interactive shells
RUN cp ~/.bashrc ~/.bashrc.bak && \
    cat ~/.bashrc.bak | tail -3 > ~/.bashrc && \
    cat ~/.bashrc.bak | head -n '-3' >> ~/.bashrc && \
    rm ~/.bashrc.bak
# install node 10
RUN bash -c '. ~/.bashrc; nvm install --lts=dubnium' && \
    ln -s "/root/.nvm/versions/node/$(ls -p /root/.nvm/versions/node | sort -Vr | head -1)bin/node" /usr/local/bin/node
RUN yum install -y nodejs

