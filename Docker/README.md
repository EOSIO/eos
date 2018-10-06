# Run in docker

Simple and fast setup of EOS.IO on Docker is also available.

## Install Dependencies

- [Docker](https://docs.docker.com) Docker 17.05 or higher is required
- [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Docker Requirement

- At least 7GB RAM (Docker -> Preferences -> Advanced -> Memory -> 7GB or above)
- If the build below fails, make sure you've adjusted Docker Memory settings and try again.

## Build eos image

```bash
git clone https://github.com/ARISENIO/arisen.git --recursive  --depth 1
cd eos/Docker
docker build . -t arisen/arisen
```

The above will build off the most recent commit to the master branch by default. If you would like to target a specific branch/tag, you may use a build argument. For example, if you wished to generate a docker image based off of the v1.3.1 tag, you could do the following:

```bash
docker build -t arisen/arisen:v1.3.1 --build-arg branch=v1.3.1 .
```

By default, the symbol in eosio.system is set to SYS. You can override this using the symbol argument while building the docker image.

```bash
docker build -t arisen/arisen --build-arg symbol=<symbol> .
```

## Start aOS docker container only

```bash
docker run --name aos -p 8888:8888 -p 9876:9876 -t arisen/arisen aosd.sh -e --http-alias=aos:8888 --http-alias=127.0.0.1:8888 --http-alias=localhost:8888 arg1 arg2
```

By default, all data is persisted in a docker volume. It can be deleted if the data is outdated or corrupted:

```bash
$ docker inspect --format '{{ range .Mounts }}{{ .Name }} {{ end }}' aos
fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
$ docker volume rm fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
```

Alternately, you can directly mount host directory into the container

```bash
docker run --name aos -v /path-to-data-dir:/opt/arisen/bin/data-dir -p 8888:8888 -p 9876:9876 -t arisen/arisen aosd.sh -e --http-alias=aos:8888 --http-alias=127.0.0.1:8888 --http-alias=localhost:8888 arg1 arg2
```

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Start both aos and awallet containers

```bash
docker volume create --name=aos-data-volume
docker volume create --name=awallet-data-volume
docker-compose up -d
```

After `docker-compose up -d`, two services named `aosd` and `awallet` will be started. aos service would expose ports 8888 and 9876 to the host. awallet service does not expose any port to the host, it is only accessible to arisecli when running arisecli is running inside the awallet container as described in "Execute arisecli commands" section.

### Execute arisecli commands

You can run the `arisecli` commands via a bash alias.

```bash
alias arisecli='docker-compose exec awallet /opt/arisen/bin/arisecli -u http://aosd:8888 --wallet-url http://localhost:8900'
arisecli get info
arisecli get account inita
```

Upload sample exchange contract

```bash
arisecli set contract exchange contracts/exchange/
```

If you don't need awallet afterwards, you can stop the awallet service using

```bash
docker-compose stop awallet
```

### Develop/Build custom contracts

Due to the fact that the arisen/arisen image does not contain the required dependencies for contract development (this is by design, to keep the image size small), you will need to utilize the arisen/arisen-dev image. This image contains both the required binaries and dependencies to build contracts using eosiocpp.

You can either use the image available on [Docker Hub](https://hub.docker.com/r/arisen/arisen-dev/) or navigate into the dev folder and build the image manually.

```bash
cd dev
docker build -t arisen/arisen-dev .
```

### Change default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  aos:
    volumes:
      - aos-data-volume:/opt/arisen/bin/data-dir
      - ./config2.ini:/opt/arisen/bin/data-dir/config.ini
```

Then restart your docker containers as follows:

```bash
docker-compose down
docker-compose up
```

### Clear data-dir

The data volume created by docker-compose can be deleted as follows:

```bash
docker volume rm aos-data-volume
docker volume rm awallet-data-volume
```

### Docker Hub

Docker Hub image available from [docker hub](https://hub.docker.com/r/arisen/arisen/).
Create a new `docker-compose.yaml` file with the content below

```bash
version: "3"

services:
  aosd:
    image: arisen/arisen:latest
    command: /opt/arisen/bin/aosd.sh --data-dir /opt/arisen/bin/data-dir -e --http-alias=aosd:8888 --http-alias=127.0.0.1:8888 --http-alias=localhost:8888
    hostname: aosd
    ports:
      - 8888:8888
      - 9876:9876
    expose:
      - "8888"
    volumes:
      - aos-data-volume:/opt/arisen/bin/data-dir

  awallet:
    image: arisen/arisen:latest
    command: /opt/arisen/bin/awallet --wallet-dir /opt/arisen/bin/data-dir --http-server-address=127.0.0.1:8900 --http-alias=localhost:8900 --http-alias=awallet:8900
    hostname: awallet
    links:
      - aosd
    volumes:
      - awallet-data-volume:/opt/arisen/bin/data-dir

volumes:
  aos-data-volume:
  awallet-data-volume:

```

*NOTE:* the default version is the latest, you can change it to what you want

run `docker pull arisen/arisen:latest`

run `docker-compose up`

### EOSIO Testnet

We can easily set up a EOSIO local testnet using docker images. Just run the following commands:

Note: if you want to use the mongo db plugin, you have to enable it in your `data-dir/config.ini` first.

```
# create volume
docker volume create --name=aos-data-volume
docker volume create --name=awallet-data-volume
# pull images and start containers
docker-compose -f docker-compose-eosio-latest.yaml up -d
# get chain info
curl http://127.0.0.1:8888/v1/chain/get_info
# get logs
docker-compose logs -f aosd
# stop containers
docker-compose -f docker-compose-eosio-latest.yaml down
```

The `blocks` data are stored under `--data-dir` by default, and the wallet files are stored under `--wallet-dir` by default, of course you can change these as you want.

### About MongoDB Plugin

Currently, the mongodb plugin is disabled in `config.ini` by default, you have to change it manually in `config.ini` or you can mount a `config.ini` file to `/opt/arisen/bin/data-dir/config.ini` in the docker-compose file.
