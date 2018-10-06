FROM arisen/builder as builder
ARG branch=master
ARG symbol=RSN

RUN git clone -b $branch https://github.com/ARISENIO/arisen.git --recursive \
    && cd arisen && echo "$branch:$(git rev-parse HEAD)" > /etc/arisen-version \
    && cmake -H. -B"/tmp/build" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/tmp/build -DBUILD_MONGO_DB_PLUGIN=true -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /tmp/build --target install && rm /tmp/build/bin/arisencpp


FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /tmp/build/bin /opt/arisen/bin
COPY --from=builder /tmp/build/contracts /contracts
COPY --from=builder /arisen/Docker/config.ini /
COPY --from=builder /etc/arisen-version /etc
COPY --from=builder /arisen/Docker/aosd.sh /opt/arisen/bin/aosd.sh
ENV ARISEN_ROOT=/opt/arisen
RUN chmod +x /opt/arisen/bin/aosd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /opt/arisen/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
