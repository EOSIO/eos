FROM arisen/builder
ARG branch=master
ARG symbol=RSN

RUN git clone -b $branch https://github.com/arisenio/arisen.git --recursive \
    && cd arisen && echo "$branch:$(git rev-parse HEAD)" > /etc/arisen-version \
    && cmake -H. -B"/opt/arisen" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/opt/arisen -DBUILD_MONGO_DB_PLUGIN=true -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /opt/arisen --target install \
    && cp /arisen/Docker/config.ini / && ln -s /opt/arisen/contracts /contracts && cp /arisen/Docker/aosd.sh /opt/arisen/bin/aosd.sh && ln -s /arisen/tutorials /tutorials

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates vim psmisc python3-pip && rm -rf /var/lib/apt/lists/*
RUN pip3 install numpy
ENV ARISEN_ROOT=/opt/arisen
RUN chmod +x /opt/arisen/bin/aosd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /opt/arisen/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
