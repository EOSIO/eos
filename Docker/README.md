# Run in docker

Simple and fast setup of Enumivo on Docker is also available.

## Install Dependencies

- [Docker](https://docs.docker.com) Docker 17.05 or higher is required
- [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Docker Requirement

- At least 7GB RAM (Docker -> Preferences -> Advanced -> Memory -> 7GB or above)
- If the build below fails, make sure you've adjusted Docker Memory settings and try again.

## Build eos image

```bash
git clone https://github.com/enumivo/enumivo.git --recursive
cd eos/Docker
docker build . -t enumivo/eos
```

The above will build off the most recent commit to the master branch by default. If you would like to target a specific branch/tag, you may use a build argument. For example, if you wished to generate a docker image based off of the dawn-v4.0.0 tag, you could do the following:

```bash
docker build -t enumivo/eos:dawn-v4.0.0 --build-arg branch=dawn-v4.0.0 .
```

## Start enunode docker container only

```bash
docker run --name enunode -p 8888:8888 -p 9876:9876 -t enumivo/eos nodeosd.sh arg1 arg2
```

By default, all data is persisted in a docker volume. It can be deleted if the data is outdated or corrupted:

```bash
$ docker inspect --format '{{ range .Mounts }}{{ .Name }} {{ end }}' enunode
fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
$ docker volume rm fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
```

Alternately, you can directly mount host directory into the container

```bash
docker run --name enunode -v /path-to-data-dir:/opt/enumivo/bin/data-dir -p 8888:8888 -p 9876:9876 -t enumivo/eos nodeosd.sh arg1 arg2
```

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Start both enunode and enuwallet containers

```bash
docker volume create --name=enunode-data-volume
docker volume create --name=enuwallet-data-volume
docker-compose up -d
```

After `docker-compose up -d`, two services named `nodeosd` and `enuwallet` will be started. enunode service would expose ports 8888 and 9876 to the host. enuwallet service does not expose any port to the host, it is only accessible to enucli when running enucli is running inside the enuwallet container as described in "Execute enucli commands" section.

### Execute enucli commands

You can run the `enucli` commands via a bash alias.

```bash
alias enucli='docker-compose exec enuwallet /opt/enumivo/bin/enucli -u http://nodeosd:8888'
enucli get info
enucli get account inita
```

Upload sample exchange contract

```bash
enucli set contract exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi
```

If you don't need enuwallet afterwards, you can stop the enuwallet service using

```bash
docker-compose stop enuwallet
```

### Develop/Build custom contracts

Due to the fact that the enumivo/eos image does not contain the required dependencies for contract development (this is by design, to keep the image size small), you will need to utilize enumivo/builder. However, enumivo/builder does not contain enumivocpp. As such, you will need to run enumivo/builder interactively, and clone, build and install Enumivo. Once this is complete, you can then utilize enumivocpp to compile your contracts.

You can also create a Dockerfile that will do this for you.

```
FROM enumivo/builder

RUN git clone -b master --depth 1 https://github.com/enumivo/enumivo.git --recursive \
    && cd eos \
    && cmake -H. -B"/tmp/build" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DSecp256k1_ROOT_DIR=/usr/local -DBUILD_MONGO_DB_PLUGIN=true \
    && cmake --build /tmp/build --target install && rm -rf /tmp/build /eos
```

Then, from the same directory as the Dockerfile, simply run:

```bash
docker build -t enumivo/contracts .
docker run -it -v /path/to/custom/contracts:/contracts enumivo/contracts /bin/bash
```

At this time you should be at a bash shell. You can navigate into the /contracts directory and use enumivocpp to compile your custom contracts.

### Change default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  enunode:
    volumes:
      - enunode-data-volume:/opt/enumivo/bin/data-dir
      - ./config2.ini:/opt/enumivo/bin/data-dir/config.ini
```

Then restart your docker containers as follows:

```bash
docker-compose down
docker-compose up
```

### Clear data-dir

The data volume created by docker-compose can be deleted as follows:

```bash
docker volume rm enunode-data-volume
docker volume rm enuwallet-data-volume
```

### Docker Hub

Docker Hub image available from [docker hub](https://hub.docker.com/r/enumivo/eos/).
Create a new `docker-compose.yaml` file with the content below

```bash
version: "3"

services:
  nodeosd:
    image: enumivo/eos:latest
    command: /opt/enumivo/bin/nodeosd.sh
    hostname: nodeosd
    ports:
      - 8888:8888
      - 9876:9876
    expose:
      - "8888"
    volumes:
      - enunode-data-volume:/opt/enumivo/bin/data-dir

  enuwallet:
    image: enumivo/eos:latest
    command: /opt/enumivo/bin/enuwallet
    hostname: enuwallet
    links:
      - nodeosd
    volumes:
      - enuwallet-data-volume:/opt/enumivo/bin/data-dir

volumes:
  enunode-data-volume:
  enuwallet-data-volume:

```

*NOTE:* the default version is the latest, you can change it to what you want

run `docker pull enumivo/eos:latest`

run `docker-compose up`

### Dawn 4.0 Testnet

We can easily set up a Dawn 4.0 local testnet using docker images. Just run the following commands:

Note: if you want to use the mongo db plugin, you have to enable it in your `data-dir/config.ini` first.

```
# pull images
docker pull enumivo/eos:latest
docker pull mongo:latest
# create volume
docker volume create --name=enunode-data-volume
docker volume create --name=enuwallet-data-volume
docker volume create --name=mongo-data-volume
# start containers
docker-compose -f docker-compose-dawn4.0.yaml up -d
# get chain info
curl http://127.0.0.1:8888/v1/chain/get_info
# get logs
docker-compose logs nodeosd
# stop containers
docker-compose -f docker-compose-dawn4.0.yaml down
```

The `blocks` data are stored under `--data-dir` by default, and the wallet files are stored under `--wallet-dir` by default, of course you can change these as you want.

### About MongoDB Plugin

Currently, the mongodb plugin is disabled in `config.ini` by default, you have to change it manually in `config.ini` or you can mount a `config.ini` file to `/opt/enumivo/bin/data-dir/config.ini` in the docker-compose file.
