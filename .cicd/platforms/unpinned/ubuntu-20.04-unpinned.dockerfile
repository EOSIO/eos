FROM ubuntu:20.04

# install dependencies.
RUN export DEBIAN_FRONTEND=noninteractive \
 && apt-get update \
 && apt-get install -yq \
        binaryen                    \
        build-essential             \
        ccache                      \
        cmake                       \
        curl                        \
        dumb-init                   \
        git                         \
        jq                          \
        libboost-all-dev            \
        libcurl4-openssl-dev        \
        libgmp-dev                  \
        libssl-dev                  \
        libtinfo5                   \
        libusb-1.0-0-dev            \
        libzstd-dev                 \
        llvm-11-dev                 \
        ninja-build                 \
        npm                         \
        pkg-config                  \
        time                        \
 && apt-get clean -yq \
 && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/usr/bin/dumb-init", "--"]
