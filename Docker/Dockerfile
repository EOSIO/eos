FROM eosio/builder as builder

RUN git clone https://github.com/EOSIO/eos.git --recursive 
WORKDIR /eos
RUN cmake -DBINARYEN_BIN=/eos/externals/binaryen/bin -DWASM_ROOT=/opt/wasm -DOPENSSL_ROOT_DIR=/usr/bin/openssl -DOPENSSL_LIBRARIES=/usr/lib/ssl -DBUILD_MONGO_DB_PLUGIN=true .
RUN make 
RUN make install

ENV PATH=/usr/local/eosio/bin/:$PATH

FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates
COPY --from=builder /usr/local/eosio/bin/ /opt/eosio/bin
COPY --from=builder /eos/contracts /  
ENV EOSIO_ROOT=/opt/eosio
ENV PATH /opt/eosio/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

EXPOSE 8889 8899

CMD ["/bin/bash"]
