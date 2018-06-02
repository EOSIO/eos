# Run in docker

Simple and fast setup of Enumivo on Docker is also available.

## Install Dependencies

- [Docker](https://docs.docker.com) Docker 17.05 or higher is required
- [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Docker Requirement

- At least 7GB RAM (Docker -> Preferences -> Advanced -> Memory -> 7GB or above)
- If the build below fails, make sure you've adjusted Docker Memory settings and try again.

## Build Enumivo image

```bash
git clone https://github.com/enumivo/enumivo.git --recursive  --depth 1
cd enumivo/Docker
docker build . -t enumivo/enumivo
```

The above will build off the most recent commit to the master branch by default. If you would like to target a specific branch/tag, you may use a build argument. For example, if you wished to generate a docker image based off of the v1.0.0 tag, you could do the following:

```bash
docker build -t enumivo/enumivo:v1.0.0 --build-arg branch=v1.0.0 .
```

By default, the symbol in enu.system is set to ENU. You can override this using the symbol argument while building the docker image.

```bash
docker build -t enumivo/enumivo --build-arg symbol=<symbol> .
```

## Start enunode docker container only

```bash
docker run --name enunode -p 8888:8888 -p 9876:9876 -t enumivo/enumivo enunoded.sh -e arg1 arg2
```

By default, all data is persisted in a docker volume. It can be deleted if the data is outdated or corrupted:

```bash
$ docker inspect --format '{{ range .Mounts }}{{ .Name }} {{ end }}' enunode
fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
$ docker volume rm fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
```

Alternately, you can directly mount host directory into the container

```bash
docker run --name enumivo -v /path-to-data-dir:/opt/enumivo/bin/data-dir -p 8888:8888 -p 9876:9876 -t enumivo/enumivo enunoded.sh -e arg1 arg2
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

After `docker-compose up -d`, two services named `enunoded` and `enuwalletd` will be started. enunode service would expose ports 8888 and 9876 to the host. enuwalletd service does not expose any port to the host, it is only accessible to enucli when running enucli is running inside the enuwalletd container as described in "Execute enucli commands" section.

### Execute enulic commands

You can run the `enulic` commands via a bash alias.

```bash
alias enulic='docker-compose exec enuwallet /opt/enumivo/bin/enulic -u http://enunoded:8888 --wallet-url http://localhost:8888'
enulic get info
enulic get account inita
```

Upload sample exchange contract

```bash
enulic set contract exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi
```

If you don't need enuwallet afterwards, you can stop the enuwallet service using

```bash
docker-compose stop enuwallet
```

### Develop/Build custom contracts

Due to the fact that the enumivo/enumivo image does not contain the required dependencies for contract development (this is by design, to keep the image size small), you will need to utilize the enumivo/enumivo image. This image contains both the required binaries and dependencies to build contracts using enumivocpp.

You can either use the image available on [Docker Hub](https://hub.docker.com/r/enumivo/enumivo/) or navigate into the dev folder and build the image manually.

```bash
cd dev
docker build -t enumivo/enumivo .
```

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

Docker Hub image available from [docker hub](https://hub.docker.com/r/enumivo/enumivo/).
Create a new `docker-compose.yaml` file with the content below

```bash
version: "3"

services:
  enunoded:
    image: enumivo/enumivo:latest
    command: /opt/enumivo/bin/enunoded.sh -e
    hostname: enunoded
    ports:
      - 8888:8888
      - 9876:9876
    expose:
      - "8888"
    volumes:
      - enunode-data-volume:/opt/enumivo/bin/data-dir

  enuwallet:
    image: enumivo/enumivo:latest
    command: /opt/enumivo/bin/enuwallet
    hostname: enuwallet
    links:
      - enunoded
    volumes:
      - enuwallet-data-volume:/opt/enumivo/bin/data-dir

volumes:
  enunode-data-volume:
  enuwallet-data-volume:

```

*NOTE:* the default version is the latest, you can change it to what you want

run `docker pull enumivo/enumivo:latest`

run `docker-compose up`

### Enumivo 1.0 Testnet

We can easily set up a Enumivo 1.0 local testnet using docker images. Just run the following commands:

Note: if you want to use the mongo db plugin, you have to enable it in your `data-dir/config.ini` first.

```
# pull images
docker pull enumivo/enumivo:v1.0.0

# create volume
docker volume create --name=enunode-data-volume
docker volume create --name=enuwallet-data-volume
# start containers
docker-compose -f docker-compose-enumivo1.0.yaml up -d
# get chain info
curl http://127.0.0.1:8888/v1/chain/get_info
# get logs
docker-compose logs -f enunoded
# stop containers
docker-compose -f docker-compose-enumivo1.0.yaml down
```

The `blocks` data are stored under `--data-dir` by default, and the wallet files are stored under `--wallet-dir` by default, of course you can change these as you want.

### About MongoDB Plugin

Currently, the mongodb plugin is disabled in `config.ini` by default, you have to change it manually in `config.ini` or you can mount a `config.ini` file to `/opt/enumivo/bin/data-dir/config.ini` in the docker-compose file.
