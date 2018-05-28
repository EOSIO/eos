FROM enumivo/builder
ARG branch=master
ARG symbol=ENU

RUN git clone -b $branch https://github.com/enumivo/enumivo.git --recursive \
    && cd enumivo && echo "$branch:$(git rev-parse HEAD)" > /etc/enumivo-version \
    && cmake -H. -B"/opt/enumivo" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/opt/enumivo  -DSecp256k1_ROOT_DIR=/usr/local -DBUILD_MONGO_DB_PLUGIN=true -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /opt/enumivo --target install \
    && mv /enumivo/Docker/config.ini / && mv /opt/enumivo/contracts /contracts && mv /enumivo/Docker/enunoded.sh /opt/enumivo/bin/nodeosd.sh && mv tutorials /tutorials \
    && rm -rf /enumivo

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl vim psmisc python3-pip && rm -rf /var/lib/apt/lists/*
RUN pip3 install numpy
ENV ENUMIVO_ROOT=/opt/enumivo
RUN chmod +x /opt/enumivo/bin/enunoded.sh
ENV LD_LIBRARY_PATH /usr/local/lib
VOLUME /opt/enumivo/bin/data-dir
ENV PATH /opt/enumivo/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
