# Run in docker

Simple and fast setup of EOS.IO on Docker is also available.

## Install Dependencies

- [Docker](https://docs.docker.com) Docker 17.05 or higher is required
- [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Docker Requirement

- At least 8GB RAM (Docker -> Preferences -> Advanced -> Memory -> 8GB or above)

## Build eos image

```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos/Docker
docker build . -t eosio/eos
```

## Start nodeos docker container only

```bash
docker run --name nodeos -p 8888:8888 -p 9876:9876 -t eosio/eos nodeosd.sh arg1 arg2
```

By default, all data is persisted in a docker volume. It can be deleted if the data is outdated or corrupted:

```bash
$ docker inspect --format '{{ range .Mounts }}{{ .Name }} {{ end }}' nodeos
fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
$ docker volume rm fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
```

Alternately, you can directly mount host directory into the container

```bash
docker run --name nodeos -v /path-to-data-dir:/opt/eosio/bin/data-dir -p 8888:8888 -p 9876:9876 -t eosio/eos nodeosd.sh arg1 arg2
```

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Start both nodeos and keosd containers

```bash
docker volume create --name=nodeos-data-volume
docker volume create --name=keosd-data-volume
docker-compose up -d
```

After `docker-compose up -d`, two services named `nodeosd` and `keosd` will be started. nodeos service would expose ports 8888 and 9876 to the host. keosd service does not expose any port to the host, it is only accessible to cleos when runing cleos is running inside the keosd container as described in "Execute cleos commands" section.

### Execute cleos commands

You can run the `cleos` commands via a bash alias.

```bash
alias cleos='docker-compose exec keosd /opt/eosio/bin/cleos -H nodeosd'
cleos get info
cleos get account inita
```

Upload sample exchange contract

```bash
cleos set contract exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi
```

If you don't need keosd afterwards, you can stop the keosd service using

```bash
docker-compose stop keosd
```

### Change default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  nodeos:
    volumes:
      - nodeos-data-volume:/opt/eosio/bin/data-dir
      - ./config2.ini:/opt/eosio/bin/data-dir/config.ini
```

Then restart your docker containers as follows:

```bash
docker-compose down
docker-compose up
```

### Clear data-dir

The data volume created by docker-compose can be deleted as follows:

```bash
docker volume rm nodeos-data-volume
docker volume rm keosd-data-volume
```

### Docker Hub

Docker Hub image available from [docker hub](https://hub.docker.com/r/eosio/eos/).
Replace the `docker-compose.yaml` file with the content below

```bash
version: "3"

services:
  nodeosd:
    image: eosio/eos:latest
    command: /opt/eosio/bin/nodeosd.sh
    hostname: nodeosd
    ports:
      - 8888:8888
      - 9876:9876
    expose:
      - "8888"
    volumes:
      - nodeos-data-volume:/opt/eosio/bin/data-dir

  keosd:
    image: eosio/eos:latest
    command: /opt/eosio/bin/keosd
    hostname: keosd
    links:
      - nodeosd
    volumes:
      - keosd-data-volume:/opt/eosio/bin/data-dir

volumes:
  nodeos-data-volume:
  keosd-data-volume:

```

*NOTE:* the defalut version is the latest, you can change it to what you want

run `docker pull eosio/eos:latest` 

run `docker-compose up`

### Dawn3.0 Testnet

We can easliy set up a dawn3.0 local testnet using docker images. Just run the following commands:

Note: if you want to use the mongo db plugin, you have to enable it in your `data-dir/config.ini` first.

```
# pull images
docker pull eosio/eos:latest
docker pull mongo:latest
# create volume
docker volume create --name=nodeos-data-volume
docker volume create --name=keosd-data-volume
docker volume create --name=mongo-data-volume
# start containers
docker-compose -f docker-compose-dawn3.0.yaml up -d
# get chain info
curl http://127.0.0.1:8888/v1/chain/get_info
# get logs
docker-compose logs nodeosd
# stop containers
docker-compose -f docker-compose-dawn3.0.yaml down
```

The `blocks` data are stored under `--data-dir` by default, and the wallet files are stored under `--wallet-dir` by default, of course you can change these as you want.

### About MongoDB Plugin

Currently, the mongodb plugin is disabled in `config.ini` by default, you have to change it manually in `config.ini` or you can mount a `config.ini` file to `/opt/eosio/bin/data-dir/config.ini` in the docker-compose file.