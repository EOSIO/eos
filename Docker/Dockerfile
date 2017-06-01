FROM base/archlinux:latest
MAINTAINER Nathan Hourt <nat.hourt@gmail.com>

RUN pacman --noconfirm --needed -Syu clang ninja cmake git autoconf automake make m4 binutils pkg-config libtool fakeroot boost
RUN mkdir /work
WORKDIR /work
COPY * /work/
RUN bash ./bootstrap.sh
