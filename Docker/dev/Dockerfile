FROM uosproject/builder
ARG branch=master
ARG symbol=UOS

RUN git clone -b $branch https://github.com/UOSnetwork/uos.git --recursive \
    && cd uos && echo "$branch:$(git rev-parse HEAD)" > /etc/eosio-version \
    && cmake -H. -B"/opt/uosproject" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/opt/uosproject -DBUILD_MONGO_DB_PLUGIN=true -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /opt/uosproject --target install \
    && cp /uos/Docker/config.ini / && ln -s /opt/uosproject/contracts /contracts && cp /uos/Docker/nodeosd.sh /opt/uosproject/bin/nodeosd.sh && ln -s /uos/tutorials /tutorials

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates vim psmisc python3-pip && rm -rf /var/lib/apt/lists/*
RUN pip3 install numpy
ENV EOSIO_ROOT=/opt/uosproject
RUN chmod +x /opt/uosproject/bin/nodeosd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /opt/uosproject/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
