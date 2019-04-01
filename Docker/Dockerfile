FROM uosproject/builder as builder
ARG branch=master
ARG symbol=UOS

RUN git clone -b $branch https://github.com/UOSnetwork/uos.git --recursive \
    && cd uos && echo "$branch:$(git rev-parse HEAD)" > /etc/eosio-version \
    && cmake -H. -B"/tmp/build" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/tmp/build -DBUILD_MONGO_DB_PLUGIN=true -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /tmp/build --target install


FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /tmp/build/bin /opt/uosproject/bin
COPY --from=builder /tmp/build/contracts /contracts
COPY --from=builder /uos/Docker/config.ini /
COPY --from=builder /etc/eosio-version /etc
COPY --from=builder /uos/Docker/nodeosd.sh /opt/uosproject/bin/nodeosd.sh
ENV EOSIO_ROOT=/opt/uosproject
RUN chmod +x /opt/uosproject/bin/nodeosd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /opt/uosproject/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
